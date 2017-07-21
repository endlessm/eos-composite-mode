/* Copyright 2015 Endless Mobile, Inc. */

#include "ecm-daemon.h"

#include <stdlib.h>

#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>

#include "ecm-generated.h"

#define COMPOSITE_MODE_SCHEMA "com.endlessm.CompositeMode"
#define COMPOSITE_MODE_HDMI_SCHEMA "com.endlessm.CompositeMode.hdmi"
#define COMPOSITE_MODE_COMPOSITE_SCHEMA "com.endlessm.CompositeMode.composite"

#define TEXT_SCALING_FACTOR_KEY "text-scaling-factor"
#define BROWSER_SCALING_FACTOR_KEY "browser-scaling-factor"

typedef enum {
  ECM_STATE_INITIALIZED,
  ECM_STATE_HDMI,
  ECM_STATE_COMPOSITE,
} EcmState;

struct _EcmDaemon
{
  GtkApplication parent;

  EcmManager *skeleton;

  GdkScreen *gdk_screen;
  Display *xdpy;
  Window root_win;

  GSettings *current_settings;

  EcmState state;
};

G_DEFINE_TYPE (EcmDaemon, ecm_daemon, GTK_TYPE_APPLICATION);

static GSettings *
get_state_settings (EcmState state)
{
  switch (state) {
  case ECM_STATE_HDMI: return g_settings_new (COMPOSITE_MODE_HDMI_SCHEMA);
  case ECM_STATE_COMPOSITE: return g_settings_new (COMPOSITE_MODE_COMPOSITE_SCHEMA);
  default: g_assert_not_reached ();
  }
}

static void
load_settings (EcmDaemon *daemon)
{
  g_assert (ECM_IS_DAEMON (daemon));
  g_assert (G_IS_SETTINGS (daemon->current_settings));

  g_autoptr (GSettings) interface_settings = g_settings_new (COMPOSITE_MODE_SCHEMA);

  // Only set the scaling values if they are different from the currently set ones
  // otherwise it may end up spawning events related to the screen for no reason...
  // See https://phabricator.endlessm.com/T18078
  gdouble old_text_scaling = g_settings_get_double (interface_settings, TEXT_SCALING_FACTOR_KEY);
  gdouble old_browser_scaling = g_settings_get_double (interface_settings, BROWSER_SCALING_FACTOR_KEY);
  gdouble new_text_scaling = g_settings_get_double (daemon->current_settings, TEXT_SCALING_FACTOR_KEY);
  gdouble new_browser_scaling = g_settings_get_double (daemon->current_settings, BROWSER_SCALING_FACTOR_KEY);

  if (old_text_scaling != new_text_scaling)
    g_settings_set_double (interface_settings, TEXT_SCALING_FACTOR_KEY, new_text_scaling);

  if (old_browser_scaling != new_browser_scaling)
    g_settings_set_double (interface_settings, BROWSER_SCALING_FACTOR_KEY, new_browser_scaling);
}

static void
settings_changed (GSettings *settings, const char *key, EcmDaemon *daemon)
{
  load_settings (daemon);
}

static void
set_state (EcmDaemon *daemon, EcmState state)
{
  if (state == ECM_STATE_INITIALIZED)
    g_assert_not_reached ();

  if (daemon->state == state)
    return;

  daemon->state = state;

  /* We will be monitoring a different base schema now, update the GSettings object. */
  g_clear_object (&daemon->current_settings);
  daemon->current_settings = get_state_settings (state);

  /* Due to BGO bug 733791, we need to make sure that we connect to the "changed"
   * signal before any call to g_settings_get() or the callback won't ever be,
   * executed, so let's do it right before calling load_settings().
   * See https://bugzilla.gnome.org/show_bug.cgi?id=733791 */
  g_signal_connect_object (daemon->current_settings, "changed",
                           G_CALLBACK (settings_changed), daemon, 0);

  /* GSettings updated and state set. All ready to load the values. */
  load_settings (daemon);

  ecm_manager_set_is_in_composite_mode (daemon->skeleton, state == ECM_STATE_COMPOSITE);
}

static void
set_composite_mode (EcmDaemon *daemon, gboolean enabled)
{
  set_state (daemon, enabled ? ECM_STATE_COMPOSITE : ECM_STATE_HDMI);
}

static gboolean
output_is_composite (Display *dpy, RROutput output_id)
{
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  g_autofree unsigned char *buffer = NULL;

  Atom connector_type = XInternAtom (dpy, RR_PROPERTY_CONNECTOR_TYPE, False);
  XRRGetOutputProperty (dpy, output_id,
                        connector_type,
                        0, G_MAXLONG, False, False, XA_ATOM,
                        &actual_type, &actual_format,
                        &nitems, &bytes_after, &buffer);

  if (actual_type != XA_ATOM || actual_format != 32 || nitems < 1)
    return FALSE;

  Atom res = ((Atom *) buffer)[0];
  Atom composite = (XInternAtom (dpy, "TV-Composite", True));
  return (res == composite);
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC(XRROutputInfo, XRRFreeOutputInfo)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(XRRScreenResources, XRRFreeScreenResources)

static void
check_outputs (EcmDaemon *daemon)
{
  int i;
  gboolean has_composite_output = FALSE;
  gboolean has_connected_composite_output = FALSE;

  g_autoptr(XRRScreenResources) resources = XRRGetScreenResourcesCurrent (daemon->xdpy, daemon->root_win);

  for (i = 0; i < resources->noutput; ++i) {
    RROutput output_id = resources->outputs[i];

    if (output_is_composite (daemon->xdpy, output_id)) {
      has_composite_output = TRUE;

      g_autoptr(XRROutputInfo) output_info = XRRGetOutputInfo (daemon->xdpy, resources, output_id);
      if (output_info->connection == RR_Connected) {
        has_connected_composite_output = TRUE;
        break;
      }
    }
  }

  if (!has_composite_output && !getenv ("EOS_COMPOSITE_MODE_DEBUG"))
    exit (1);

  set_composite_mode (daemon, has_connected_composite_output);
}

static void
monitors_changed (GdkScreen *screen,
                  gpointer user_data)
{
  EcmDaemon *daemon = user_data;
  check_outputs (daemon);
}

static void
ecm_daemon_startup (GApplication *app)
{
  EcmDaemon *daemon = ECM_DAEMON (app);

  G_APPLICATION_CLASS (ecm_daemon_parent_class)->startup (app);

  daemon->gdk_screen = gdk_screen_get_default ();
  daemon->xdpy = GDK_DISPLAY_XDISPLAY (gdk_screen_get_display (daemon->gdk_screen));
  daemon->root_win = GDK_WINDOW_XID (gdk_screen_get_root_window (daemon->gdk_screen));

  g_signal_connect (daemon->gdk_screen, "monitors-changed",
                    G_CALLBACK (monitors_changed), daemon);
  check_outputs (daemon);

  /* hold so it runs in the session indefinitely */
  g_application_hold (app);
}

static gboolean
handle_debug_set_composite_mode (EcmManager *manager,
                                 GDBusMethodInvocation *invocation,
                                 gboolean enabled,
                                 gpointer user_data)
{
  EcmDaemon *daemon = ECM_DAEMON (user_data);
  set_composite_mode (daemon, enabled);
  ecm_manager_complete_debug_set_composite_mode (manager, invocation);
  return TRUE;
}

static void
reset_keys_for_schema (const gchar *schema_id)
{
  g_autoptr (GSettings) interface_settings = g_settings_new (schema_id);
  g_settings_reset (interface_settings, TEXT_SCALING_FACTOR_KEY);
  g_settings_reset (interface_settings, BROWSER_SCALING_FACTOR_KEY);
}

static void
reset_settings (void)
{
  reset_keys_for_schema (COMPOSITE_MODE_SCHEMA);
  reset_keys_for_schema (COMPOSITE_MODE_HDMI_SCHEMA);
  reset_keys_for_schema (COMPOSITE_MODE_COMPOSITE_SCHEMA);
}

static gboolean
handle_debug_reset_settings (EcmManager *manager,
                             GDBusMethodInvocation *invocation,
                             gboolean enabled,
                             gpointer user_data)
{
  reset_settings ();
  ecm_manager_complete_debug_reset_settings (manager, invocation);
  return TRUE;
}

static gboolean
ecm_daemon_dbus_register (GApplication    *app,
                          GDBusConnection *connection,
                          const char      *object_path,
                          GError         **error)
{
  EcmDaemon *daemon = ECM_DAEMON (app);

  daemon->skeleton = ecm_manager_skeleton_new ();
  g_signal_connect (daemon->skeleton, "handle-debug-set-composite-mode",
                    G_CALLBACK (handle_debug_set_composite_mode), daemon);
  g_signal_connect (daemon->skeleton, "handle-debug-reset-settings",
                    G_CALLBACK (handle_debug_reset_settings), daemon);

  if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (daemon->skeleton), connection, object_path, error))
    return FALSE;

  return TRUE;
}

static void
ecm_daemon_dbus_unregister (GApplication    *app,
                            GDBusConnection *connection,
                            const char      *object_path)
{
  EcmDaemon *daemon = ECM_DAEMON (app);

  g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (daemon->skeleton));

  /* If the session goes away, then turn off composite mode
   * so that if the next boot is with HDMI, it works fine. */
  set_composite_mode (daemon, FALSE);
}

static void
ecm_daemon_dispose (GObject *object)
{
  EcmDaemon *daemon = ECM_DAEMON (object);

  g_clear_object (&daemon->skeleton);
  g_clear_object (&daemon->current_settings);

  G_OBJECT_CLASS (ecm_daemon_parent_class)->dispose (object);
}

static void
ecm_daemon_class_init (EcmDaemonClass *klass)
{
  GApplicationClass *application_class = G_APPLICATION_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  application_class->startup = ecm_daemon_startup;
  application_class->dbus_register = ecm_daemon_dbus_register;
  application_class->dbus_unregister = ecm_daemon_dbus_unregister;
  object_class->dispose = ecm_daemon_dispose;
}

static void
ecm_daemon_init (EcmDaemon *daemon)
{
}
