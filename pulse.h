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
  struct pulseVolume_t sink;
  struct pulseVolume_t source;
};

typedef int(*pollFunction_t)(struct pollfd* ufds, unsigned long nfds, int timeout, void* userdata);

#define SINK 0
#define SOURCE 1

struct pulseDevice_t {
  int deviceType;
  bool isDefault;
  char* name;
  char* description;
};

struct pulseDeviceList_t {
  struct pulseDevice_t* list;
  int count;
};

/**
 * Initialise connection to PulseAudio server
 */
void pulseInit(const char* name);

/**
 * Cleanup
 */
void pulseCleanup();

/**
 * Get state
 */
struct pulseState_t pulseGetState();

/**
 * Set volume
 */
void pulseSetVolume(int channel, int volume);

/**
 * Mute/unmute
 */
void pulseToggleMute(int channel);

/**
 * Get list of devices for a channel (0 = sinks, 1 = sources)
 */
const struct pulseDeviceList_t* pulseGetDevices(int channel);

/**
 * Select a device as new default source or sink
 */
void pulseSelectDevice(struct pulseDevice_t* device);

#endif
