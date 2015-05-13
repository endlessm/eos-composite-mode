/* Copyright 2015 Endless Mobile, Inc. */

#pragma once

#include <gtk/gtk.h>

#define ECM_TYPE_DAEMON (ecm_daemon_get_type ())
G_DECLARE_FINAL_TYPE (EcmDaemon, ecm_daemon, ECM, DAEMON, GtkApplication)
