// wmpulsemixer - A simple PulseAudio mixer for WindowMaker
// Copyright (C) 2026  Fabien Pollet <mail@frmpollet.me> (wmpulsemixer)
// Copyright (C) 2003  Damian Kramer <psiren@hibernaculum.net> (wmsmixer)
// Copyright (C) 1998  Sam Hawker <shawkie@geocities.com> (wmmixer)
// This software comes with ABSOLUTELY NO WARRANTY
// This software is free software, and you are welcome to redistribute it
// under certain conditions
// See the COPYING file for a more complete notice.

#include "ui.h"
#include "options.h"
#include "window.h"
#include "pulse.h"
#include "menu.h"

#include <stdbool.h>

#include <gtk/gtk.h>
#include <X11/Xlib.h>

// For repeating next and prev buttons
static guint repeatTimeoutId = 0;

// For draggable volume control
static bool dragging=false;

// forward declarations for private functions
static void uiPressEvent(GdkEvent* event);
static void uiReleaseEvent();
static void uiMotionEvent(GdkEvent* event);
static void uiScrollEvent(GdkXEvent* event);
static gboolean uiRepeatTimeoutEvent(gpointer data);

void uiGDKEventHandler(GdkEvent *event, gpointer data) {
  switch(event->type) {
  case GDK_DELETE:
    if (options.verbose) {
      printf("GDK_DELETE empfangen. Beende GTK-Mainloop sauber...\n");
    }
    gtk_main_quit();
    return;
  case GDK_BUTTON_PRESS:
    uiPressEvent(event);
    break;
  case GDK_BUTTON_RELEASE:
    uiReleaseEvent();
    break;
  case GDK_MOTION_NOTIFY:
    uiMotionEvent(event);
    break;
  default:
    break;
  }

  gtk_main_do_event(event);
}

GdkFilterReturn uiXEventHandler(GdkXEvent* xevent, GdkEvent* event, gpointer data) {
  XEvent* xev = (XEvent*)xevent;

  // Some events are not handled correctly if I try to handle them in gdkEventHandler.
  // - Calling repaint on GDK_EXPOSE, GDK_CONFIGURE and GDK_VISIBILITY_NOTIFY doesn't work as well as
  //   just Expose here. It still leads to some weird effects when moving the window.
  // - gdkEventHandler is not called for scroll events.
  // I'm not sure why that is but I think it's because the window was not created with GTK and GTK/GDK
  // events are not 1-to-1 the same as Xlib events.

  switch(xev->type) {
  case ButtonPress:
    if(xev->xbutton.button == Button4 || xev->xbutton.button == Button5) {
      uiScrollEvent(xevent);
    }
    break;
  case Expose:
    windowRepaint(true);
    break;
  default:
    break;
  }

  return GDK_FILTER_CONTINUE;
}


static void uiPressEvent(GdkEvent* event)
{
  if(options.verbose) printf("uiPressEvent\n");
  int x=event->button.x-(options.winsize/2-32);
  int y=event->button.y-(options.winsize/2-32);

  if(x>=5 && y>=47 && x<=17 && y<=57){
    windowState.curchannel--;
    if(windowState.curchannel<0)
      windowState.curchannel=CHANNELS-1;
    windowState.btnstate |= BTNPREV;
    if(repeatTimeoutId > 0) {
      g_source_remove(repeatTimeoutId);
      repeatTimeoutId = 0;
    }
    repeatTimeoutId = g_timeout_add(REPEAT_INTERVAL, uiRepeatTimeoutEvent, NULL);
    windowDrawBtns(BTNPREV);
    windowRepaint(true);
  } else if(x>=18 && y>=47 && x<=30 && y<=57){
    windowState.curchannel++;
    if(windowState.curchannel>=CHANNELS)
      windowState.curchannel=0;
    windowState.btnstate|=BTNNEXT;
    repeatTimeoutId = g_timeout_add(REPEAT_INTERVAL, uiRepeatTimeoutEvent, NULL);
    windowDrawBtns(BTNNEXT);
    windowRepaint(true);
  } else if(!windowState.curmuted && (x>=37 && x<=56 && y>=8 && y<=56)){
    int v=((60-y)*100)/(2*25);
    dragging=true;
    pulseSetVolume(windowState.curchannel, v);
  } else if(x>=5 && y>=21 && x<=30 && y<=42) {
    if(event->button.button == Button1) {
      pulseToggleMute(windowState.curchannel);
      windowRepaint(true);
    } else if(event->button.button == Button3) {
      menuOpen(windowState.curchannel, event);
    }
  }
}

static void uiReleaseEvent()
{
  if(options.verbose) printf("uiReleaseEvent\n");
  dragging=false;
  windowState.btnstate &= ~(BTNPREV | BTNNEXT);
  windowDrawBtns(BTNPREV | BTNNEXT);
  if(repeatTimeoutId > 0) {
    g_source_remove(repeatTimeoutId);
    repeatTimeoutId = 0;
  }
  windowRepaint(false);
}

static void uiMotionEvent(GdkEvent* event)
{
  if(options.verbose) printf("uiMotionEvent\n");
  int x=event->motion.x-(options.winsize/2-32);
  int y=event->motion.y-(options.winsize/2-32);
  if(x>=37 && x<=56 && y>=8 && dragging){
    int v=((60-y)*100)/(2*25);
    if(v<0)
      v=0;
    pulseSetVolume(windowState.curchannel, v);
  }
}

static void uiScrollEvent(GdkXEvent* event)
{
  if(options.verbose) printf("uiScrollEvent\n");
  XEvent* xev = (XEvent*)event;
  int target = windowState.curvol;
  if(xev->xbutton.button == Button4) target +=4;
  else target -= 4;

  target = target < 0 ? 0 : target;
  target = target > 100 ? 100 : target;

  pulseSetVolume(windowState.curchannel, target); // we could set higher values but we limit to what we can display
}

static gboolean uiRepeatTimeoutEvent(gpointer data) {
  if(windowState.btnstate & (BTNPREV | BTNNEXT)){
    if(options.verbose) printf("uiRepeatTimeoutEvent: still pressed, repeat\n");
    if(windowState.btnstate & BTNNEXT)
      windowState.curchannel++;
    else
      windowState.curchannel--;
    if(windowState.curchannel<0)
      windowState.curchannel=CHANNELS-1;
    if(windowState.curchannel>=CHANNELS)
      windowState.curchannel=0;
    windowRepaint(true);
    return TRUE;
  } else {
    if(options.verbose) printf("uiRepeatTimeoutEvent: released, no repeat\n");
    repeatTimeoutId = 0;
    return FALSE;
  }
}
