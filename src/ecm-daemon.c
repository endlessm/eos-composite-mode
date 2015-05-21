/* Copyright 2015 Endless Mobile, Inc. */

#include "ecm-daemon.h"

#include <stdlib.h>

#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>

#include "ecm-generated.h"

#include "ecm-settings.c"

struct _EcmDaemon
{
  GtkApplication parent;

  EcmManager *skeleton;

  GdkScreen *gdk_screen;
  Display *xdpy;
  Window root_win;

  gboolean composite_enabled;
  EcmSettings saved_settings;
};

G_DEFINE_TYPE (EcmDaemon, ecm_daemon, GTK_TYPE_APPLICATION);

static void
ecm_daemon_dispose (GObject *object)
{
  EcmDaemon *daemon = ECM_DAEMON (object);

  g_clear_object (&daemon->skeleton);

  G_OBJECT_CLASS (ecm_daemon_parent_class)->dispose (object);
}

static void
set_composite_mode (EcmDaemon *daemon, gboolean enabled)
{
  if (daemon->composite_enabled == enabled)
    return;

  daemon->composite_enabled = enabled;
  ecm_manager_set_is_in_composite_mode (daemon->skeleton, enabled);

  if (enabled) {
    ecm_settings_load_from_gsettings (&daemon->saved_settings);
    ecm_settings_save_to_gsettings (&composite_settings);
  } else {
    ecm_settings_save_to_gsettings (&daemon->saved_settings);
  }
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
