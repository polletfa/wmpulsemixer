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

// Includes - standard
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include <poll.h>
#include <errno.h>

// X-Windows includes
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/xpm.h>
#include <X11/extensions/shape.h>

// Includes - PulseAudio
#include "pulse.h"

// Defines
#define BOOST       1
#define WINDOWMAKER false
#define USESHAPE    false
#define AFTERSTEP   false
#define NORMSIZE    64
#define ASTEPSIZE   56
#define NAME        "wmpulsemixer"
#define CLASS       "WmPulseMixer"
#define BACKCOLOR   "#202020"
#define LEDCOLOR    "#00c9c1"

#define VERSION "0.1.4"

// Pixmaps
Pixmap pm_main;
Pixmap pm_tile;
Pixmap pm_disp;
Pixmap pm_mask;
Pixmap pm_icon;
Pixmap pm_digits;
Pixmap pm_chars;
Pixmap pm_muted;
Pixmap pm_muted_mask;

// Xpm images
#include "XPM/wmpulsemixer.xpm"
#include "XPM/tile.xpm"
#include "XPM/icons.xpm"
#include "XPM/digits.xpm"
#include "XPM/chars.xpm"
#include "XPM/muted.xpm"

// Variables for command-line arguments
double boost=BOOST;
bool wmaker=WINDOWMAKER;
bool ushape=USESHAPE;
bool astep=AFTERSTEP;
char display[256]="";
char position[256]="";
int winsize;
char backcolor[256]=BACKCOLOR;
char ledcolor[256]=LEDCOLOR;

// X-Windows basics
Atom _XA_GNUSTEP_WM_FUNC;
Atom deleteWin;
Display *d_display;
Window w_icon;
Window w_main;
Window w_root;
Window w_activewin;
GC gc_gc;
unsigned long color[4];

int text_counter = 0;

// Current state information
int curchannel=0;
int curvol;
bool curmuted;

// For buttons
int btnstate=0;
#define BTNNEXT  1
#define BTNPREV  2

// For repeating next and prev buttons
#define RPTINTERVAL   5
int rpttimer=0;

// For draggable volume control
bool dragging=false;

// Channels
#define CHANNELS 2
int icon[CHANNELS]={0, 1};
char *small_labels[CHANNELS] = {"vol", "mic"};

// Event loop
struct pollfd* pollFDs = NULL;
int pollFDsSize = 0;

// Functions
// ---------

// Functions - command line parsing
void scanArgs(int argc, char **argv);

// Functions - interface
void initXWin(int argc, char **argv);
void freeXWin();
void createWin(Window *win, int x, int y);
unsigned long getColor(char *colorname);
unsigned long mixColor(char *colorname1, int prop1, char *colorname2, int prop2);
void drawMono();
void drawVolLevel();
void drawText(char *text);
void drawBtns(int btns);
void drawBtn(int x, int y, int w, int h, bool down);
void repaint(bool force);
void pressEvent(XButtonEvent *xev);
void releaseEvent(XButtonEvent *xev);
void motionEvent(XMotionEvent *xev);

// Functions - event loop
int eventLoopPoll(struct pollfd* ufds, unsigned long nfds, int timeout, void* userdata);

// Implementation
// --------------

int main(int argc, char **argv)
{
  scanArgs(argc, argv);
  initXWin(argc, argv);

  XGCValues gcv;
  unsigned long gcm;
  gcm=GCGraphicsExposures;
  gcv.graphics_exposures=false;
  gc_gc=XCreateGC(d_display, w_root, gcm, &gcv);

  color[0]=mixColor(ledcolor, 0, backcolor, 100);
  color[1]=mixColor(ledcolor, 100, backcolor, 0);
  color[2]=mixColor(ledcolor, 60, backcolor, 40);
  color[3]=mixColor(ledcolor, 25, backcolor, 75);

  XpmAttributes xpmattr;
  XpmColorSymbol xpmcsym[4]={{"back_color",     NULL, color[0]},
                             {"led_color_high", NULL, color[1]},
                             {"led_color_med",  NULL, color[2]},
                             {"led_color_low",  NULL, color[3]}};
  xpmattr.numsymbols=4;
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


  if(wmaker || ushape || astep)
    XShapeCombineMask(d_display, w_activewin, ShapeBounding, winsize/2-32, winsize/2-32, pm_mask, ShapeSet);
  else
    XCopyArea(d_display, pm_tile, pm_disp, gc_gc, 0, 0, 64, 64, 0, 0);

  XSetClipMask(d_display, gc_gc, pm_mask);
  XCopyArea(d_display, pm_main, pm_disp, gc_gc, 0, 0, 64, 64, 0, 0);
  XSetClipMask(d_display, gc_gc, None);

  // Connect to PulseAudio
  pulseInit(NAME, boost);

  XEvent xev;
  XSelectInput(d_display, w_activewin, ExposureMask | ButtonPressMask | ButtonReleaseMask | ButtonMotionMask);
  XMapWindow(d_display, w_main);
  repaint(true);

  bool done=false;
  while(!done){
    // PulseAudio events
    struct pulseState_t state = pulseIterate();
    if(!state.running) {
      done = true;
      break;
    }
    int newvol = curchannel == 0 ? (state.sink.muted ? 0 : state.sink.volume) : (state.source.muted ? 0 : state.source.volume);
    int newmuted = curchannel == 0 ? state.sink.muted : state.source.muted;
    if(newvol != curvol || newmuted != curmuted) {
      curvol = newvol;
      curmuted = newmuted;
      repaint(false);
    }

    // X11 events
    while(XPending(d_display)){
      XNextEvent(d_display, &xev);
      switch(xev.type){
      case Expose:
        repaint(false);
        break;
      case ButtonPress:
        pressEvent(&xev.xbutton);
        break;
      case ButtonRelease:
        releaseEvent(&xev.xbutton);
        break;
      case MotionNotify:
        motionEvent(&xev.xmotion);
        break;
      case ClientMessage:
        if(xev.xclient.data.l[0]==deleteWin)
          done=true;
        break;
      }
    }

    if(btnstate & (BTNPREV | BTNNEXT)){
      rpttimer++;
      if(rpttimer>=RPTINTERVAL){
        if(btnstate & BTNNEXT)
          curchannel++;
        else
          curchannel--;
        if(curchannel<0)
          curchannel=CHANNELS-1;
        if(curchannel>=CHANNELS)
          curchannel=0;
        rpttimer=0;
      }
      repaint(true);
    }

    if(text_counter) {
      text_counter--;
      if(!text_counter) {
        repaint(false);
      }
    }

    XFlush(d_display);

    if(!done) {
#ifdef DEBUG
      printf("Poll...\n");
#endif
      struct pollfd x11fd = { ConnectionNumber(d_display), POLLIN, 0 };
      if(pulseWait(eventLoopPoll, &x11fd) < 0) {
        break;
      }
#ifdef DEBUG
      printf("Waking up!\n");
#endif
    }
}
  XFreeGC(d_display, gc_gc);
  XFreePixmap(d_display, pm_main);
  XFreePixmap(d_display, pm_tile);
  XFreePixmap(d_display, pm_disp);
  XFreePixmap(d_display, pm_mask);
  XFreePixmap(d_display, pm_icon);
  XFreePixmap(d_display, pm_digits);
  XFreePixmap(d_display, pm_chars);
  freeXWin();

  pulseCleanup();

  if(pollFDs != NULL) {
    free(pollFDs);
    pollFDs = NULL;
    pollFDsSize = 0;
  }
  return 0;
}

void initXWin(int argc, char **argv)
{
  winsize=astep ? ASTEPSIZE : NORMSIZE;

  if((d_display=XOpenDisplay(display))==NULL){
    fprintf(stderr,"%s : Unable to open X display '%s'.\n", NAME, XDisplayName(display));
    exit(1);
  }
  _XA_GNUSTEP_WM_FUNC=XInternAtom(d_display, "_GNUSTEP_WM_FUNCTION", false);
  deleteWin=XInternAtom(d_display, "WM_DELETE_WINDOW", false);

  w_root=DefaultRootWindow(d_display);

  XWMHints wmhints;
  XSizeHints shints;
  shints.x=0;
  shints.y=0;
  shints.flags=0;
  bool pos=(XWMGeometry(d_display, DefaultScreen(d_display), position, NULL, 0, &shints, &shints.x, &shints.y,
                        &shints.width, &shints.height, &shints.win_gravity) & (XValue | YValue));
  shints.min_width=winsize;
  shints.min_height=winsize;
  shints.max_width=winsize;
  shints.max_height=winsize;
  shints.base_width=winsize;
  shints.base_height=winsize;
  shints.flags=PMinSize | PMaxSize | PBaseSize;

  createWin(&w_main, shints.x, shints.y);

  if(wmaker || astep || pos)
    shints.flags |= USPosition;
  if(wmaker){
    wmhints.initial_state=WithdrawnState;
    wmhints.flags=WindowGroupHint | StateHint | IconWindowHint;
    createWin(&w_icon, shints.x, shints.y);
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
}

void freeXWin()
{
  XDestroyWindow(d_display, w_main);
  if(wmaker)
    XDestroyWindow(d_display, w_icon);
  XCloseDisplay(d_display);
}

void createWin(Window *win, int x, int y)
{
  XClassHint classHint;
  *win=XCreateSimpleWindow(d_display, w_root, x, y, winsize, winsize, 0, 0, 0);
  classHint.res_name=NAME;
  classHint.res_class=CLASS;
  XSetClassHint(d_display, *win, &classHint);
}

unsigned long getColor(char *colorname)
{
  XColor color;
  XWindowAttributes winattr;
  XGetWindowAttributes(d_display, w_root, &winattr);
  color.pixel=0;
  XParseColor(d_display, winattr.colormap, colorname, &color);
  color.flags=DoRed | DoGreen | DoBlue;
  XAllocColor(d_display, winattr.colormap, &color);
  return color.pixel;
}

unsigned long mixColor(char *colorname1, int prop1, char *colorname2, int prop2)
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

void scanArgs(int argc, char **argv)
{
  for(int i=1;i<argc;i++){
    if(strcmp(argv[i], "-h")==0 || strcmp(argv[i], "--help")==0) {
      fprintf(stderr, NAME " - A simple PulseAudio mixer for WindowMaker\n");
      fprintf(stderr, "Copyright (C) 2026  Fabien Pollet <mail@frmpollet.me> (wmpulsemixer)\n");
      fprintf(stderr, "Copyright (C) 2003  Damian Kramer <psiren@hibernaculum.net> (wmsmixer)\n");
      fprintf(stderr, "Copyright (C) 1998  Sam Hawker <shawkie@geocities.com> (wmmixer)\n");
      fprintf(stderr, "This software comes with ABSOLUTELY NO WARRANTY\n");
      fprintf(stderr, "This software is free software, and you are welcome to redistribute it\n");
      fprintf(stderr, "under certain conditions\n");
      fprintf(stderr, "See the README file for a more complete notice.\n\n");
      fprintf(stderr, "usage:\n\n   %s [options]\n\noptions:\n\n",argv[0]);
      fprintf(stderr, "   -h | --help            display this help screen\n");
      fprintf(stderr, "   -v | --version         display the version\n");
      fprintf(stderr, "   -boost factor          Boost factor, e.g. 1.5 to allow volume up to 150%%\n");
      fprintf(stderr, "   -w                     use WithdrawnState    (for WindowMaker)\n");
      fprintf(stderr, "   -s                     shaped window\n");
      fprintf(stderr, "   -a                     use smaller window    (for AfterStep Wharf)\n");
      fprintf(stderr, "   -l led_color           use the specified color for led display\n");
      fprintf(stderr, "   -b back_color          use the specified color for backgrounds\n");
      fprintf(stderr, "   -position position     set window position   (see X manual pages)\n");
      fprintf(stderr, "   -display display       select target display (see X manual pages)\n\n");
      exit(0);
    }
    if(strcmp(argv[i], "-v")==0 || strcmp(argv[i], "--version")==0) {
      fprintf(stderr, NAME " version %s\n", VERSION);
      exit(0);
    }
    if(strcmp(argv[i], "-boost")==0){
      if(i<argc-1){
        i++;
        boost = atof(argv[i]);
      }
      continue;
    }
    if(strcmp(argv[i], "-w")==0)
      wmaker=!wmaker;
    if(strcmp(argv[i], "-s")==0)
      ushape=!ushape;
    if(strcmp(argv[i], "-a")==0)
      astep=!astep;
    if(strcmp(argv[i], "-l")==0){
      if(i<argc-1){
        i++;
        sprintf(ledcolor, "%s", argv[i]);
      }
      continue;
    }
    if(strcmp(argv[i], "-b")==0){
      if(i<argc-1){
        i++;
        sprintf(backcolor, "%s", argv[i]);
      }
      continue;
    }
    if(strcmp(argv[i], "-position")==0){
      if(i<argc-1){
        i++;
        sprintf(position, "%s", argv[i]);
      }
      continue;
    }
    if(strcmp(argv[i], "-display")==0){
      if(i<argc-1){
        i++;
        sprintf(display, "%s", argv[i]);
      }
      continue;
    }
  }
}

void pressEvent(XButtonEvent *xev)
{
  int x=xev->x-(winsize/2-32);
  int y=xev->y-(winsize/2-32);

  if(!curmuted && (xev->button == Button4 || xev->button == Button5)) {
    int inc;
    if(xev->button == Button4) inc = 4;
    else inc = -4;

    pulseSetVolume(curchannel, curvol + inc > 100 ? 100 : curvol + inc); // we could set higher values but we limit to what we can display
  } else if(x>=5 && y>=47 && x<=17 && y<=57){
    curchannel--;
    if(curchannel<0)
      curchannel=CHANNELS-1;
    btnstate |= BTNPREV;
    rpttimer=0;
    drawBtns(BTNPREV);
  } else if(x>=18 && y>=47 && x<=30 && y<=57){
    curchannel++;
    if(curchannel>=CHANNELS)
      curchannel=0;
    btnstate|=BTNNEXT;
    rpttimer=0;
    drawBtns(BTNNEXT);
  } else if(!curmuted && (x>=37 && x<=56 && y>=8 && y<=56)){
    int v=((60-y)*100)/(2*25);
    dragging=true;
    pulseSetVolume(curchannel, v);
  } else if(x>=5 && y>=21 && x<=30 && y<=42) {
    repaint(true);
    pulseToggleMute(curchannel);
    return;
  }

}

void releaseEvent(XButtonEvent *xev)
{
  dragging=false;
  btnstate &= ~(BTNPREV | BTNNEXT);
  drawBtns(BTNPREV | BTNNEXT);
  repaint(false);
}

void motionEvent(XMotionEvent *xev)
{
  int x=xev->x-(winsize/2-32);
  int y=xev->y-(winsize/2-32);
  if(x>=37 && x<=56 && y>=8 && dragging){
    int v=((60-y)*100)/(2*25);
    if(v<0)
      v=0;
    pulseSetVolume(curchannel, v);
  }
}

void repaint(bool force)
{
  drawMono();
  if(force || text_counter) {
    drawText(small_labels[curchannel]);
  } else {
    drawVolLevel();
  }
  XCopyArea(d_display, pm_icon, pm_disp, gc_gc, icon[curchannel]*26, 0, 26, 24, 5, 19);
  if(curmuted) {
    XSetClipOrigin(d_display, gc_gc, 5, 19);
    XSetClipMask(d_display, gc_gc, pm_muted_mask);
    XCopyArea(d_display, pm_muted, pm_disp, gc_gc, 0, 0, 26, 24, 5, 19);
    XSetClipMask(d_display, gc_gc, None);
  }
  XCopyArea(d_display, pm_disp, w_activewin, gc_gc, 0, 0, 64, 64, winsize/2-32, winsize/2-32);
  XEvent xev;
  while(XCheckTypedEvent(d_display, Expose, &xev));
}

void drawText(char *text)
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
  text_counter = 10;
}

void drawVolLevel()
{
  int digits[4];
  int vol = (int)(curvol * boost);
  digits[0] = vol < 100 ? 10 : (vol / 100) % 10; // 10 = empty
  digits[1] = vol < 10 ? 10 : (vol / 10) % 10; // 10 = empty
  digits[2] = vol % 10;
  digits[3] = 10; // clears the last place, in case something else was written before
                  // (would not happend with 3-letters channel names, but could happen if we use longer names later)

  for(int i=0; i<4; i++) {
    XCopyArea(d_display, pm_digits, pm_disp, gc_gc, 6*digits[i], 0, 6, 9, 5+(i*6), 5);
  }
}

void drawMono()
{
  XSetForeground(d_display, gc_gc, color[1]);
  for(int i=0;i<25;i++){
    if(i==(curvol*25)/100)
      XSetForeground(d_display, gc_gc, color[3]);
    XFillRectangle(d_display, pm_disp, gc_gc, 37, 55-2*i, 20, 1);
  }
}


void drawBtns(int btns)
{
  if(btns & BTNPREV)
    drawBtn(5, 47, 13, 11, (btnstate & BTNPREV));
  if(btns & BTNNEXT)
    drawBtn(18, 47, 13, 11, (btnstate & BTNNEXT));
}

void drawBtn(int x, int y, int w, int h, bool down)
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

int eventLoopPoll(struct pollfd* ufds, unsigned long nfds, int timeout, void* userdata) {
  if(pollFDs == NULL) {
    pollFDs = (struct pollfd*)malloc(sizeof(struct pollfd) * (nfds + 1));
    pollFDsSize = nfds + 1;
  } else if(pollFDsSize < nfds + 1) {
    pollFDs = (struct pollfd*)realloc(pollFDs, sizeof(struct pollfd) * (nfds + 1));
    pollFDsSize = nfds + 1;
  }

  memcpy(pollFDs, ufds, sizeof(struct pollfd) * nfds);
  memcpy(&(pollFDs[nfds]), userdata, sizeof(struct pollfd));

  // if the channel name is being displayed, we don't wait indefinitely, because
  // we want to draw the volume level after a few iterations of the loop.
  // If the volume level is being displayed (text_counter == 0), then we can wait forever.
  int pollRes = poll(pollFDs, nfds + 1, text_counter ? 50 : -1);

  if(pollRes < 0) {
    fprintf(stderr, "Poll failed: %s\n", strerror(errno));
  }
  return pollRes;
}
