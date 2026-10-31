// wmpulsemixer - A simple PulseAudio mixer for WindowMaker
// Copyright (C) 2026  Fabien Pollet <mail@frmpollet.me> (wmpulsemixer)
// Copyright (C) 2003  Damian Kramer <psiren@hibernaculum.net> (wmsmixer)
// Copyright (C) 1998  Sam Hawker <shawkie@geocities.com> (wmmixer)
// This software comes with ABSOLUTELY NO WARRANTY
// This software is free software, and you are welcome to redistribute it
// under certain conditions
// See the COPYING file for a more complete notice.

#ifndef WMPULSEMIXER_WINDOW_H
#define WMPULSEMIXER_WINDOW_H

#include <stdbool.h>
#include <gtk/gtk.h>
#include <X11/Xlib.h>

struct windowState_t {
  int curchannel;
  int curvol;
  bool curmuted;
  int btnstate;
};

extern struct windowState_t windowState;

void windowInit(int argc, char **argv, GdkEventFunc gdkEventHandler, GdkFilterFunc xEventHandler);
void windowCleanup();

void windowDrawBtns(int btns);
void windowRepaint(bool force);

gboolean windowRepaintEvent(gpointer data);

#endif
