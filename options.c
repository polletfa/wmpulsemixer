// wmpulsemixer - A simple PulseAudio mixer for WindowMaker
// Copyright (C) 2026  Fabien Pollet <mail@frmpollet.me> (wmpulsemixer)
// Copyright (C) 2003  Damian Kramer <psiren@hibernaculum.net> (wmsmixer)
// Copyright (C) 1998  Sam Hawker <shawkie@geocities.com> (wmmixer)
// This software comes with ABSOLUTELY NO WARRANTY
// This software is free software, and you are welcome to redistribute it
// under certain conditions
// See the COPYING file for a more complete notice.

#include "options.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct options_t options = {
  .boost=BOOST,
  .wmaker=WINDOWMAKER,
  .ushape=USESHAPE,
  .astep=AFTERSTEP,
  .verbose=VERBOSE,
  .display="",
  .position="",
  .backcolor=BACKCOLOR,
  .ledcolor=LEDCOLOR,
  .mutecolor=MUTECOLOR,

  .winsize=0
};

void optionsScanCommandLine(int argc, char **argv)
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
      fprintf(stderr, "   --boost factor         Boost factor, e.g. 1.5 to allow volume up to 150%%\n");
      fprintf(stderr, "   -w                     use WithdrawnState    (for WindowMaker)\n");
      fprintf(stderr, "   -s                     shaped window\n");
      fprintf(stderr, "   -a                     use smaller window    (for AfterStep Wharf)\n");
      fprintf(stderr, "   -l led_color           use the specified color for led display  (default: %s)\n", LEDCOLOR);
      fprintf(stderr, "   -b back_color          use the specified color for backgrounds  (default: %s)\n", BACKCOLOR);
      fprintf(stderr, "   -m mute_color          use the specified color for mute display (default: %s)\n", MUTECOLOR);
      fprintf(stderr, "   --position position    set window position   (see X manual pages)\n");
      fprintf(stderr, "   --display display      select target display (see X manual pages)\n");
      fprintf(stderr, "   --verbose              enable debug information\n");
      fprintf(stderr, "\n");
      fprintf(stderr, "Note: For backward compatibility, the old options -boost, -position, -display and -verbose\n");
      fprintf(stderr, "      (with a single dash) are still supported.\n");
      fprintf(stderr, "\n");
      exit(0);
    }
    if(strcmp(argv[i], "-v")==0 || strcmp(argv[i], "--version")==0) {
      fprintf(stderr, NAME " version %s\n", VERSION);
      exit(0);
    }
    if(strcmp(argv[i], "--boost")==0 || strcmp(argv[i], "-boost")==0){
      if(i<argc-1){
        i++;
        options.boost = atof(argv[i]);
      }
      continue;
    }
    if(strcmp(argv[i], "-w")==0)
      options.wmaker=!WINDOWMAKER;
    if(strcmp(argv[i], "-s")==0)
      options.ushape=!USESHAPE;
    if(strcmp(argv[i], "-a")==0) {
      options.astep=!AFTERSTEP;
    }
    if(strcmp(argv[i], "-l")==0){
      if(i<argc-1){
        i++;
        sprintf(options.ledcolor, "%s", argv[i]);
      }
      continue;
    }
    if(strcmp(argv[i], "-b")==0){
      if(i<argc-1){
        i++;
        sprintf(options.backcolor, "%s", argv[i]);
      }
      continue;
    }
    if(strcmp(argv[i], "-m")==0){
      if(i<argc-1){
        i++;
        sprintf(options.mutecolor, "%s", argv[i]);
      }
      continue;
    }
    if(strcmp(argv[i], "--position")==0 || strcmp(argv[i], "-position")==0){
      if(i<argc-1){
        i++;
        sprintf(options.position, "%s", argv[i]);
      }
      continue;
    }
    if(strcmp(argv[i], "--display")==0 || strcmp(argv[i], "-display")==0){
      if(i<argc-1){
        i++;
        sprintf(options.display, "%s", argv[i]);
      }
      continue;
    }
    if(strcmp(argv[i], "--verbose")==0 || strcmp(argv[i], "-verbose")==0){
      options.verbose=!VERBOSE;
    }
  }

  options.winsize=options.astep ? ASTEPSIZE : NORMSIZE;
}
