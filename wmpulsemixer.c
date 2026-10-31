// wmpulsemixer - A simple PulseAudio mixer for WindowMaker
// Copyright (C) 2026  Fabien Pollet <mail@frmpollet.me> (wmpulsemixer)
// Copyright (C) 2003  Damian Kramer <psiren@hibernaculum.net> (wmsmixer)
// Copyright (C) 1998  Sam Hawker <shawkie@geocities.com> (wmmixer)
// This software comes with ABSOLUTELY NO WARRANTY
// This software is free software, and you are welcome to redistribute it
// under certain conditions
// See the COPYING file for a more complete notice.

#include "window.h"
#include "pulse.h"
#include "options.h"
#include "ui.h"

int main(int argc, char **argv)
{
  g_set_prgname(NAME);
  setenv("GDK_CORE_DEVICE_EVENTS", "1", 1);
  setenv("GDK_BACKEND", "x11", 1);

  optionsScanCommandLine(argc, argv);
  windowInit(argc, argv, uiGDKEventHandler, uiXEventHandler);
  pulseInit(NAME);

  gtk_main();

  windowCleanup();
  pulseCleanup();
  return 0;
}
