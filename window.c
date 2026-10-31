// wmpulsemixer - A simple PulseAudio mixer for WindowMaker
// Copyright (C) 2026  Fabien Pollet <mail@frmpollet.me> (wmpulsemixer)
// Copyright (C) 2003  Damian Kramer <psiren@hibernaculum.net> (wmsmixer)
// Copyright (C) 1998  Sam Hawker <shawkie@geocities.com> (wmmixer)
// This software comes with ABSOLUTELY NO WARRANTY
// This software is free software, and you are welcome to redistribute it
// under certain conditions
// See the COPYING file for a more complete notice.


// Defines, includes and global variables
// --------------------------------------

#include "window.h"
#include "pulse.h"
#include "options.h"

// Includes - standard
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>

// X-Windows includes
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/xpm.h>
#include <X11/extensions/shape.h>

// GTK
#include <gtk/gtk.h>
#include <gtk/gtkx.h>

// Pixmaps
static Pixmap pm_main;
static Pixmap pm_tile;
static Pixmap pm_disp;
static Pixmap pm_mask;
static Pixmap pm_icon;
static Pixmap pm_digits;
static Pixmap pm_chars;
static Pixmap pm_muted;
static Pixmap pm_muted_mask;

// Xpm images
#include "XPM/wmpulsemixer.xpm"
#include "XPM/tile.xpm"
#include "XPM/icons.xpm"
#include "XPM/digits.xpm"
#include "XPM/chars.xpm"
#include "XPM/muted.xpm"

// X-Windows basics
static Atom _XA_GNUSTEP_WM_FUNC;
static Atom deleteWin;
static Display *d_display;
static Window w_icon;
static Window w_main;
static Window w_root;
static Window w_activewin;
static GC gc_gc;
static unsigned long color[5];

// Show text interval
static bool showText = false;

// channels
static int icon[CHANNELS]={0, 1};
static char *small_labels[CHANNELS] = {"vol", "mic"};

// state
struct windowState_t windowState = {
  .curchannel = SINK,
  .curvol = 0,
  .curmuted = 0,
  .btnstate = 0
};

// forward declarations for private functions
static void windowDraw();
static void windowCreate(Window *win, int x, int y);
static unsigned long windowMixColor(char *colorname1, int prop1, char *colorname2, int prop2);
static void windowDrawMono();
static void windowDrawVolLevel();
static void windowDrawText(char *text);
static void windowDrawBtn(int x, int y, int w, int h, bool down);
static gboolean windowShowTextTimeoutEvent(gpointer data);

void windowInit(int argc, char **argv, GdkEventFunc gdkEventHandler, GdkFilterFunc xEventHandler)
{
  gtk_parse_args(&argc, &argv);

  GdkDisplayManager *manager = gdk_display_manager_get();
  GdkDisplay* gdk_disp = gdk_display_manager_open_display(manager, (options.display[0] != '\0') ? options.display : NULL);
  if (!gdk_disp) {
    const char *disp_name = (options.display[0] != '\0') ? options.display : getenv("DISPLAY");
    fprintf(stderr, "%s : Unable to open X display '%s'.\n", NAME, disp_name ? disp_name : "NULL");
    exit(1);
  }
  gdk_display_manager_set_default_display(manager, gdk_disp);

  gtk_init(&argc, &argv);

  d_display = GDK_DISPLAY_XDISPLAY(gdk_disp);

  _XA_GNUSTEP_WM_FUNC=XInternAtom(d_display, "_GNUSTEP_WM_FUNCTION", false);
  deleteWin=XInternAtom(d_display, "WM_DELETE_WINDOW", false);

  w_root=DefaultRootWindow(d_display);

  XWMHints wmhints;
  XSizeHints shints;
  shints.x=0;
  shints.y=0;
  shints.flags=0;
  bool pos=(XWMGeometry(d_display, DefaultScreen(d_display), options.position, NULL, 0, &shints, &shints.x, &shints.y,
                        &shints.width, &shints.height, &shints.win_gravity) & (XValue | YValue));
  shints.min_width=options.winsize;
  shints.min_height=options.winsize;
  shints.max_width=options.winsize;
  shints.max_height=options.winsize;
  shints.base_width=options.winsize;
  shints.base_height=options.winsize;
  shints.flags=PMinSize | PMaxSize | PBaseSize;

  windowCreate(&w_main, shints.x, shints.y);

  if(options.wmaker || options.astep || pos)
    shints.flags |= USPosition;
  if(options.wmaker){
    wmhints.initial_state=WithdrawnState;
    wmhints.flags=WindowGroupHint | StateHint | IconWindowHint;
    windowCreate(&w_icon, shints.x, shints.y);
    w_activewin=w_icon;
    wmhints.icon_window=w_icon;
  }
  else{
    wmhints.initial_state=NormalState;
    wmhints.flags=WindowGroupHint | StateHint;
    w_activewin=w_main;
  }
  wmhints.window_group=w_main;
  XSetWMHints(d_display, w_main, &wmhints);
  XSetWMNormalHints(d_display, w_main, &shints);
  XSetCommand(d_display, w_main, argv, argc);
  XStoreName(d_display, w_main, NAME);
  XSetIconName(d_display, w_main, NAME);
  XSetWMProtocols(d_display, w_activewin, &deleteWin, 1);

  windowDraw();

  GdkWindow* gdk_win = gdk_x11_window_foreign_new_for_display(gdk_disp, w_activewin);
  if (!gdk_win) {
    fprintf(stderr, "Fehler: Konnte das X11-Fenster nicht bei GDK registrieren.\n");
    exit(1);
  }
  gdk_window_add_filter(gdk_win, xEventHandler, NULL);
  gdk_event_handler_set(gdkEventHandler, NULL, NULL);
}

static void windowDraw()
{
  XGCValues gcv;
  unsigned long gcm;
  gcm=GCGraphicsExposures;
  gcv.graphics_exposures=true;
  gc_gc=XCreateGC(d_display, w_root, gcm, &gcv);

  color[0]=windowMixColor(options.ledcolor, 0, options.backcolor, 100);
  color[1]=windowMixColor(options.ledcolor, 100, options.backcolor, 0);
  color[2]=windowMixColor(options.ledcolor, 60, options.backcolor, 40);
  color[3]=windowMixColor(options.ledcolor, 25, options.backcolor, 75);
  color[4]=windowMixColor(options.mutecolor, 100, options.backcolor, 0);

  XpmAttributes xpmattr;
  XpmColorSymbol xpmcsym[5]={{"back_color",     NULL, color[0]},
                             {"led_color_high", NULL, color[1]},
                             {"led_color_med",  NULL, color[2]},
                             {"led_color_low",  NULL, color[3]},
                             {"mute_color",     NULL, color[4]}};
  xpmattr.numsymbols=5;
  xpmattr.colorsymbols=xpmcsym;
  xpmattr.exactColors=false;
  xpmattr.closeness=40000;
  xpmattr.valuemask=XpmColorSymbols | XpmExactColors | XpmCloseness;
  XpmCreatePixmapFromData(d_display, w_root, wmpulsemixer_xpm, &pm_main, &pm_mask, &xpmattr);
  XpmCreatePixmapFromData(d_display, w_root, muted_xpm, &pm_muted, &pm_muted_mask, &xpmattr);
  XpmCreatePixmapFromData(d_display, w_root, tile_xpm, &pm_tile, NULL, &xpmattr);
  XpmCreatePixmapFromData(d_display, w_root, icons_xpm, &pm_icon, NULL, &xpmattr);
  XpmCreatePixmapFromData(d_display, w_root, digits_xpm, &pm_digits, NULL, &xpmattr);
  XpmCreatePixmapFromData(d_display, w_root, chars_xpm, &pm_chars, NULL, &xpmattr);
  pm_disp=XCreatePixmap(d_display, w_root, 64, 64, DefaultDepth(d_display, DefaultScreen(d_display)));


  if(options.wmaker || options.ushape || options.astep)
    XShapeCombineMask(d_display, w_activewin, ShapeBounding, options.winsize/2-32, options.winsize/2-32, pm_mask, ShapeSet);
  else
    XCopyArea(d_display, pm_tile, pm_disp, gc_gc, 0, 0, 64, 64, 0, 0);

  XSetClipMask(d_display, gc_gc, pm_mask);
  XCopyArea(d_display, pm_main, pm_disp, gc_gc, 0, 0, 64, 64, 0, 0);
  XSetClipMask(d_display, gc_gc, None);

  XSelectInput(d_display, w_activewin, ExposureMask | ButtonPressMask | ButtonReleaseMask | ButtonMotionMask
                                       | StructureNotifyMask | VisibilityChangeMask);
  XMapWindow(d_display, w_main);

  g_idle_add(windowRepaintEvent, NULL);
}

static void windowCreate(Window *win, int x, int y)
{
  XClassHint classHint;
  *win=XCreateSimpleWindow(d_display, w_root, x, y, options.winsize, options.winsize, 0, 0, 0);
  classHint.res_name=NAME;
  classHint.res_class=CLASS;
  XSetClassHint(d_display, *win, &classHint);
}

void windowCleanup()
{
  XFreeGC(d_display, gc_gc);
  XFreePixmap(d_display, pm_main);
  XFreePixmap(d_display, pm_tile);
  XFreePixmap(d_display, pm_disp);
  XFreePixmap(d_display, pm_mask);
  XFreePixmap(d_display, pm_icon);
  XFreePixmap(d_display, pm_digits);
  XFreePixmap(d_display, pm_chars);
  XDestroyWindow(d_display, w_main);
  if(options.wmaker)
    XDestroyWindow(d_display, w_icon);
}

static unsigned long windowMixColor(char *colorname1, int prop1, char *colorname2, int prop2)
{
  XColor color, color1, color2;
  XWindowAttributes winattr;
  XGetWindowAttributes(d_display, w_root, &winattr);
  XParseColor(d_display, winattr.colormap, colorname1, &color1);
  XParseColor(d_display, winattr.colormap, colorname2, &color2);
  color.pixel=0;
  color.red=(color1.red*prop1+color2.red*prop2)/(prop1+prop2);
  color.green=(color1.green*prop1+color2.green*prop2)/(prop1+prop2);
  color.blue=(color1.blue*prop1+color2.blue*prop2)/(prop1+prop2);
  color.flags=DoRed | DoGreen | DoBlue;
  XAllocColor(d_display, winattr.colormap, &color);
  return color.pixel;
}

void windowRepaint(bool _showText)
{
  if(_showText && !showText) {
    showText = true;
    g_timeout_add(SHOW_TEXT_INTERVAL, windowShowTextTimeoutEvent, NULL);
  }
  struct pulseState_t state = pulseGetState();
  int newvol = windowState.curchannel == SINK ? (state.sink.muted ? 0 : state.sink.volume) : (state.source.muted ? 0 : state.source.volume);
  int newmuted = windowState.curchannel == SINK ? state.sink.muted : state.source.muted;
  windowState.curvol = newvol;
  windowState.curmuted = newmuted;
  windowDrawMono();
  if(showText) {
    windowDrawText(small_labels[windowState.curchannel]);
  } else {
    windowDrawVolLevel();
  }
  XCopyArea(d_display, pm_icon, pm_disp, gc_gc, icon[windowState.curchannel]*26, 0, 26, 24, 5, 19);
  if(windowState.curmuted) {
    XSetClipOrigin(d_display, gc_gc, 5, 19);
    XSetClipMask(d_display, gc_gc, pm_muted_mask);
    XCopyArea(d_display, pm_muted, pm_disp, gc_gc, 0, 0, 26, 24, 5, 19);
    XSetClipMask(d_display, gc_gc, None);
  }
  XCopyArea(d_display, pm_disp, w_activewin, gc_gc, 0, 0, 64, 64, options.winsize/2-32, options.winsize/2-32);
}

static void windowDrawText(char *text)
{
  char *p = text;
  char p2;

  for(int i=0; i<4; i++, p++) {
    p2 = toupper(*p);
    if(p2 >= 'A' && p2 <= 'Z') {
      XCopyArea(d_display, pm_chars, pm_disp, gc_gc, 6*((int)p2-65), 0, 6, 9, 5+(i*6), 5);
    }
    else if(p2 >= '0' && p2 <= '9') {
      XCopyArea(d_display, pm_digits, pm_disp, gc_gc, 6*((int)p2-48), 0, 6, 9, 5+(i*6), 5);
    }
    else {
      if(p2 == '\0')
        p--;
      XCopyArea(d_display, pm_digits, pm_disp, gc_gc, 60, 0, 6, 9, 5+(i*6), 5);
    }
  }
}

static void windowDrawVolLevel()
{
  int digits[4];
  int vol = (int)(windowState.curvol * options.boost);
  digits[0] = vol < 100 ? 10 : (vol / 100) % 10; // 10 = empty
  digits[1] = vol < 10 ? 10 : (vol / 10) % 10; // 10 = empty
  digits[2] = vol % 10;
  digits[3] = 10; // clears the last place, in case something else was written before
                  // (would not happend with 3-letters channel names, but could happen if we use longer names later)

  for(int i=0; i<4; i++) {
    XCopyArea(d_display, pm_digits, pm_disp, gc_gc, 6*digits[i], 0, 6, 9, 5+(i*6), 5);
  }
}

static void windowDrawMono()
{
  XSetForeground(d_display, gc_gc, color[1]);
  for(int i=0;i<25;i++){
    if(i==(windowState.curvol*25)/100)
      XSetForeground(d_display, gc_gc, color[3]);
    XFillRectangle(d_display, pm_disp, gc_gc, 37, 55-2*i, 20, 1);
  }
}


void windowDrawBtns(int btns)
{
  if(btns & BTNPREV)
    windowDrawBtn(5, 47, 13, 11, (windowState.btnstate & BTNPREV));
  if(btns & BTNNEXT)
    windowDrawBtn(18, 47, 13, 11, (windowState.btnstate & BTNNEXT));
}

static void windowDrawBtn(int x, int y, int w, int h, bool down)
{
  if(!down)
    XCopyArea(d_display, pm_main, pm_disp, gc_gc, x, y, w, h, x, y);
  else {
    XCopyArea(d_display, pm_main, pm_disp, gc_gc, x, y, 1, h-1, x+w-1, y+1);
    XCopyArea(d_display, pm_main, pm_disp, gc_gc, x+w-1, y+1, 1, h-1, x, y);
    XCopyArea(d_display, pm_main, pm_disp, gc_gc, x, y, w-1, 1, x+1, y+h-1);
    XCopyArea(d_display, pm_main, pm_disp, gc_gc, x+1, y+h-1, w-1, 1, x, y);
  }
}

gboolean windowRepaintEvent(gpointer data) {
  if(options.verbose) printf("windowRepaintEvent\n");
  windowRepaint(true);
  return FALSE;
}

static gboolean windowShowTextTimeoutEvent(gpointer data) {
  if(options.verbose) printf("windowShowTextTimeoutEvent\n");
  showText = false;
  windowRepaint(false);
  return FALSE;
}
