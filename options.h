// wmpulsemixer - A simple PulseAudio mixer for WindowMaker
// Copyright (C) 2026  Fabien Pollet <mail@frmpollet.me> (wmpulsemixer)
// Copyright (C) 2003  Damian Kramer <psiren@hibernaculum.net> (wmsmixer)
// Copyright (C) 1998  Sam Hawker <shawkie@geocities.com> (wmmixer)
// This software comes with ABSOLUTELY NO WARRANTY
// This software is free software, and you are welcome to redistribute it
// under certain conditions
// See the COPYING file for a more complete notice.

#ifndef WMPULSEMIXER_OPTIONS_H
#define WMPULSEMIXER_OPTIONS_H

#include <stdbool.h>

// internal definitions
#define CHANNELS 2
#define SINK 0
#define SOURCE 1

#define REPEAT_INTERVAL 500
#define SHOW_TEXT_INTERVAL 2000

#define BTNNEXT  1
#define BTNPREV  2

#define NAME        "wmpulsemixer"
#define CLASS       "WmPulseMixer"

// command line options
#define BOOST       1
#define WINDOWMAKER false
#define USESHAPE    false
#define AFTERSTEP   false
#define VERBOSE     false
#define NORMSIZE    64
#define ASTEPSIZE   56
#define BACKCOLOR   "#202020"
#define LEDCOLOR    "#00c9c1"
#define MUTECOLOR   "#ff3333"

struct options_t {
  double boost;
  bool wmaker;
  bool ushape;
  bool astep;
  bool verbose;
  char display[256];
  char position[256];
  char backcolor[256];
  char ledcolor[256];
  char mutecolor[256];

  int winsize;
};

extern struct options_t options;

void optionsScanCommandLine(int argc, char **argv);

#endif
