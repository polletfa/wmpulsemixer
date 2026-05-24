// wmpulsemixer - A simple PulseAudio mixer for WindowMaker
// Copyright (C) 2026  Fabien Pollet <mail@frmpollet.me> (wmpulsemixer)
// Copyright (C) 2003  Damian Kramer <psiren@hibernaculum.net> (wmsmixer)
// Copyright (C) 1998  Sam Hawker <shawkie@geocities.com> (wmmixer)
// This software comes with ABSOLUTELY NO WARRANTY
// This software is free software, and you are welcome to redistribute it
// under certain conditions
// See the COPYING file for a more complete notice.

#ifndef WMPULSEMIXER_PULSE_H
#define WMPULSEMIXER_PULSE_H

#include <stdbool.h>
#include <poll.h>

struct pulseVolume_t {
  bool muted;
  int volume; // between 0 and 100 (boost factor is handled internally)
};

struct pulseState_t {
  bool running;
  struct pulseVolume_t sink;
  struct pulseVolume_t source;
};

typedef int(*pollFunction_t)(struct pollfd* ufds, unsigned long nfds, int timeout, void* userdata);

/**
 * Initialise connection to PulseAudio server
 */
void pulseInit(const char* name, double boost);

/**
 * Handle all pending events
 * Return the current state
 */
struct pulseState_t pulseIterate();

/**
 * Wait using the provided function to poll.
 * Returns < 0 on error
 */
int pulseWait(pollFunction_t pollFn, void* userData);

/**
 * Cleanup
 */
void pulseCleanup();

/**
 * Set volume
 */
void pulseSetVolume(int channel, int volume);

/**
 * Mute/unmute
 */
void pulseToggleMute(int channel);

#endif
