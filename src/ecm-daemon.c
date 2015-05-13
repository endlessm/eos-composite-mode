/* Copyright 2015 Endless Mobile, Inc. */

#include "ecm-daemon.h"

#include <stdlib.h>

#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>

#include "ecm-generated.h"

struct _EcmDaemon
{
  GtkApplication parent;

  EcmManager *skeleton;

  GdkScreen *gdk_screen;
  Display *xdpy;
  Window root_win;
};

G_DEFINE_TYPE (EcmDaemon, ecm_daemon, GTK_TYPE_APPLICATION);

static void
set_composite_mode (EcmDaemon *daemon, gboolean enabled)
{
  g_object_set (daemon->skeleton,
                "is-in-composite-mode", enabled,
                NULL);
}

static gboolean
output_is_composite (Display *dpy, RROutput output_id)
{
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *buffer;
  gboolean ret = FALSE;

  Atom connector_type = XInternAtom (dpy, RR_PROPERTY_CONNECTOR_TYPE, False);
  XRRGetOutputProperty (dpy, output_id,
                        connector_type,
                        0, G_MAXLONG, False, False, XA_ATOM,
                        &actual_type, &actual_format,
                        &nitems, &bytes_after, &buffer);

  if (actual_type != XA_ATOM || actual_format != 32 || nitems < 1)
    goto out;

  Atom res = ((Atom *) buffer)[0];
  Atom composite = (XInternAtom (dpy, "TV-Composite", True));
  ret = (res == composite);

 out:
  if (buffer) XFree (buffer);
  return ret;
}

static RROutput
find_composite_output (Display *dpy, XRRScreenResources *resources)
{
  int i;
  for (i = 0; i < resources->noutput; ++i) {
    RROutput output_id = resources->outputs[i];
    if (output_is_composite (dpy, output_id))
      return output_id;
  }
  return None;
}

static void
check_outputs (EcmDaemon *daemon)
{
  XRRScreenResources *resources = XRRGetScreenResourcesCurrent (daemon->xdpy, daemon->root_win);
  XRROutputInfo *output_info = NULL;

  if (!resources)
    goto out;

  RROutput output_id = find_composite_output (daemon->xdpy, resources);

  if (output_id == None)
    goto out;

  output_info = XRRGetOutputInfo (daemon->xdpy, resources, output_id);
  set_composite_mode (daemon, output_info->connection == RR_Connected);

 out:
  if (resources) XRRFreeScreenResources (resources);
  if (output_info) XRRFreeOutputInfo (output_info);
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
ecm_daemon_dbus_register (GApplication    *app,
                          GDBusConnection *connection,
                          const char      *object_path,
                          GError         **error)
{
  EcmDaemon *daemon = ECM_DAEMON (app);

  daemon->skeleton = ecm_manager_skeleton_new ();

  if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (daemon->skeleton), connection, object_path, error))
    return FALSE;

  return TRUE;
}

static void
ecm_daemon_class_init (EcmDaemonClass *klass)
{
  GApplicationClass *application_class = G_APPLICATION_CLASS (klass);

  application_class->startup = ecm_daemon_startup;
  application_class->dbus_register = ecm_daemon_dbus_register;
}

static void
ecm_daemon_init (EcmDaemon *daemon)
{
}
