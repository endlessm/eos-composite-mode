/* Copyright 2015 Endless Mobile, Inc. */

#include "ecm-daemon.h"

int main (int argc, char *argv[])
{
  g_autoptr(EcmDaemon) daemon = g_object_new (ECM_TYPE_DAEMON,
                                              "application-id", "com.endlessm.CompositeMode",
                                              "flags", G_APPLICATION_IS_SERVICE,
                                              NULL);
  return g_application_run (G_APPLICATION (daemon), argc, argv);
}
