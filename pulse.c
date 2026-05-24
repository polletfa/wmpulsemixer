// wmpulsemixer - A simple PulseAudio mixer for WindowMaker
// Copyright (C) 2026  Fabien Pollet <mail@frmpollet.me> (wmpulsemixer)
// Copyright (C) 2003  Damian Kramer <psiren@hibernaculum.net> (wmsmixer)
// Copyright (C) 1998  Sam Hawker <shawkie@geocities.com> (wmmixer)
// This software comes with ABSOLUTELY NO WARRANTY
// This software is free software, and you are welcome to redistribute it
// under certain conditions
// See the COPYING file for a more complete notice.

#include "pulse.h"

#include <pulse/pulseaudio.h>

#include <stdio.h>
#include <math.h>

// data
double multiply = 100.0;

pa_mainloop *mainloop;
pa_context* context;

int defaultSinkIndex = -1;
int defaultSinkChannels = 1;

int defaultSourceIndex = -1;
int defaultSourceChannels = 1;

struct pulseState_t state = { true, {false, 0}, {false, 0} };

// forward declarations for callbacks (not in the header file)
void callbackContextState(pa_context* c, void* userdata);
void callbackContextSubscribe(pa_context *c, pa_subscription_event_type_t t, uint32_t idx, void *userdata);
void callbackSinkInfo(pa_context *c, const pa_sink_info *info, int eol, void *userdata);
void callbackSourceInfo(pa_context *c, const pa_source_info *info, int eol, void *userdata);
void callbackServerInfo(pa_context *c, const pa_server_info *info, void *userdata);

/**
 * Initialise connection to PulseAudio server
 */
void pulseInit(const char* name, double boost) {
  bool success;
  multiply = 100.0 / (boost * PA_VOLUME_NORM);

  mainloop = pa_mainloop_new();
  success = mainloop != NULL;
  if(success) {
    context = pa_context_new(pa_mainloop_get_api(mainloop), name);
    success = context != NULL;
  }
  if(success) {
    pa_context_set_state_callback(context, callbackContextState, NULL);
    success = 0 == pa_context_connect(context, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL);
  }
  if(!success) {
    fprintf(stderr, "Unable to connect to PulseAudio.\n");
    if(context) {
      fprintf(stderr, "%s\n", pa_strerror(pa_context_errno(context)));
    } else {
      fprintf(stderr, "Context could not be created.\n");
    }
    exit(1);
  }
}

/**
 * Handle all pending events.
 * Return the current state
 */
struct pulseState_t pulseIterate() {
  while(pa_mainloop_iterate(mainloop, 0, NULL) > 0)
    ; // loop until no event has been handled or there was an error
  return state;
}

/**
 * Wait using the provided function.
 */
int pulseWait(pollFunction_t pollFn, void* userData) {
  pa_mainloop_set_poll_func(mainloop, pollFn, userData);
  int res = pa_mainloop_iterate(mainloop, 0, NULL);
  pa_mainloop_set_poll_func(mainloop, NULL, NULL);
  if(res < 0) {
    fprintf(stderr, "Error on pulseWait: %s\n", pa_strerror(pa_context_errno(context)));
    return res;
  }
  return 0;
}

/**
 * Cleanup
 */
void pulseCleanup() {
  pa_context_disconnect(context);
  pa_context_unref(context);
  pa_mainloop_free(mainloop);
}

/**
 * Set volume
 */
void pulseSetVolume(int channel, int volume) {
  if((channel == 0 && defaultSinkIndex < 0) || (channel == 1 && defaultSourceIndex < 0)) {
    return; // not yet initialized
  }

  pa_volume_t pa_volume = (pa_volume_t)(volume / multiply);
  pa_cvolume cvol;
  pa_cvolume_init(&cvol);
  pa_cvolume_set(&cvol, channel == 0 ? defaultSinkChannels : defaultSourceChannels, pa_volume);

#ifdef DEBUG
  printf("Set %d to %d\n", channel, volume);
#endif
  pa_operation* op = NULL;
  if(channel == 0) {
    op = pa_context_set_sink_volume_by_index(context, defaultSinkIndex, &cvol, NULL, NULL);
  } else {
    op = pa_context_set_source_volume_by_index(context, defaultSourceIndex, &cvol, NULL, NULL);
  }
  if (op) {
    pa_operation_unref(op);
  }
}

/**
 * Mute/Unmute
 */
void pulseToggleMute(int channel) {
  if((channel == 0 && defaultSinkIndex < 0) || (channel == 1 && defaultSourceIndex < 0)) {
    return; // not yet initialized
  }

  pa_operation* op = NULL;
  if(channel == 0) {
    op = pa_context_set_sink_mute_by_index(context, defaultSinkIndex, !state.sink.muted, NULL, NULL);
  } else {
    op = pa_context_set_source_mute_by_index(context, defaultSourceIndex, !state.source.muted, NULL, NULL);
  }
  if (op) {
    pa_operation_unref(op);
  }
}

/**
 * Callback when the state of the context changes.
 * Used to react on connect/disconnect
 */
void callbackContextState(pa_context* c, void* userdata) {
  switch (pa_context_get_state(c)) {
  case PA_CONTEXT_READY: {
#ifdef DEBUG
    printf("PulseAudio connection ready.\n");
#endif

    pa_operation *op = pa_context_get_server_info(context, callbackServerInfo, NULL);
    if (op) pa_operation_unref(op);

    pa_context_set_subscribe_callback(c, callbackContextSubscribe, NULL);
    op = pa_context_subscribe(c,PA_SUBSCRIPTION_MASK_SINK | PA_SUBSCRIPTION_MASK_SOURCE | PA_SUBSCRIPTION_MASK_SERVER, NULL, NULL);
    if (op) pa_operation_unref(op);
    break;
  }

  case PA_CONTEXT_FAILED:
  case PA_CONTEXT_TERMINATED:
#ifdef DEBUG
    fprintf(stderr, "PulseAudio connection lost.\n");
#endif
    state.running = false;
    break;

  default:
    break;
  }
}

/**
 * Callback when a subscription event occurs (volume change or server event)
 */
void callbackContextSubscribe(pa_context *c, pa_subscription_event_type_t t, uint32_t idx, void *userdata) {
  bool isServer = (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) == PA_SUBSCRIPTION_EVENT_SERVER;
  bool isSink = (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) == PA_SUBSCRIPTION_EVENT_SINK;
  bool isSource = (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) == PA_SUBSCRIPTION_EVENT_SOURCE;
  bool isChange = (t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_CHANGE;

  if(isServer) {
#ifdef DEBUG
    printf("Server event\n");
#endif
    pa_operation *op = pa_context_get_server_info(context, callbackServerInfo, NULL);
    if (op) pa_operation_unref(op);
  } else if(isChange && isSink && defaultSinkIndex == idx) {
#ifdef DEBUG
    printf("Default sink volume changed\n");
#endif
    pa_operation *op = pa_context_get_sink_info_by_index(c, idx, callbackSinkInfo, NULL);
    if (op) pa_operation_unref(op);
  } else if(isChange && isSource && defaultSourceIndex == idx) {
#ifdef DEBUG
    printf("Default source volume changed\n");
#endif
    pa_operation *op = pa_context_get_source_info_by_index(c, idx, callbackSourceInfo, NULL);
    if (op) pa_operation_unref(op);
  }
}

/**
 * Callback when sink info is received (requested when server info is received or the volume of the sink changes)
 */
void callbackSinkInfo(pa_context *c, const pa_sink_info *info, int eol, void *userdata) {
  if (eol < 0 || info == NULL) return;

#ifdef DEBUG
  printf("Received sink info\n");
#endif
  defaultSinkIndex = info->index;
  defaultSinkChannels = info->channel_map.channels;

  state.sink.muted = info->mute;
  pa_volume_t avg_vol = pa_cvolume_avg(&info->volume);
  state.sink.volume = (int)(round(avg_vol * multiply));
#ifdef DEBUG
  printf("Volume: %d (muted = %d) - PA_VOLUME_NORM: %d - normalized: %d\n", avg_vol, state.sink.muted, PA_VOLUME_NORM, state.sink.volume);
#endif
}

/**
 * Callback when source info is received (requested when server info is received or the volume of the source changes)
 */
void callbackSourceInfo(pa_context *c, const pa_source_info *info, int eol, void *userdata) {
  if (eol < 0 || info == NULL) return;

#ifdef DEBUG
  printf("Received source info\n");
#endif
  defaultSourceIndex = info->index;
  defaultSourceChannels = info->channel_map.channels;

  state.source.muted = info->mute;
  pa_volume_t avg_vol = pa_cvolume_avg(&info->volume);
  state.source.volume = (int)(round(avg_vol * multiply));
#ifdef DEBUG
  printf("Volume: %d (muted = %d) - PA_VOLUME_NORM: %d - normalized: %d\n", avg_vol, state.source.muted, PA_VOLUME_NORM, state.source.volume);
#endif
}

/**
 * Callback when server info is received (requested when a server event occurs, on on start)
 */
void callbackServerInfo(pa_context *c, const pa_server_info *info, void *userdata) {
  if (!info) return;

#ifdef DEBUG
  printf("Received server info\n");
#endif
  pa_operation *op = pa_context_get_sink_info_by_name(c, info->default_sink_name, callbackSinkInfo, NULL);
  if (op) pa_operation_unref(op);

  op = pa_context_get_source_info_by_name(c, info->default_source_name, callbackSourceInfo, NULL);
  if (op) pa_operation_unref(op);
}
