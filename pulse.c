// wmpulsemixer - A simple PulseAudio mixer for WindowMaker
// Copyright (C) 2026  Fabien Pollet <mail@frmpollet.me> (wmpulsemixer)
// Copyright (C) 2003  Damian Kramer <psiren@hibernaculum.net> (wmsmixer)
// Copyright (C) 1998  Sam Hawker <shawkie@geocities.com> (wmmixer)
// This software comes with ABSOLUTELY NO WARRANTY
// This software is free software, and you are welcome to redistribute it
// under certain conditions
// See the COPYING file for a more complete notice.

#include "pulse.h"
#include "options.h"
#include "window.h"

#include <stdio.h>
#include <math.h>
#include <string.h>

#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>

#include <gtk/gtk.h>

// data
static double multiply = 100.0;

static pa_glib_mainloop* mainloop;
static pa_context* context;

static int defaultSinkIndex = -1;
static int defaultSinkChannels = 1;
static struct pulseDeviceList_t sinkList = { NULL, 0 };

static int defaultSourceIndex = -1;
static int defaultSourceChannels = 1;
static struct pulseDeviceList_t sourceList = { NULL, 0 };

static struct pulseState_t state = { {false, 0}, {false, 0} };

// forward declarations for private functions
static void callbackContextState(pa_context* c, void* userdata);
static void callbackContextSubscribe(pa_context* c, pa_subscription_event_type_t t, uint32_t idx, void* userdata);
static void callbackSinkInfo(pa_context* c, const pa_sink_info* info, int eol, void* userdata);
static void callbackSourceInfo(pa_context* c, const pa_source_info* info, int eol, void* userdata);
static void callbackServerInfo(pa_context* c, const pa_server_info* info, void* userdata);
static void callbackSetDevice(pa_context* c, int success, void* userdata);

/**
 * Initialise connection to PulseAudio server
 */
void pulseInit(const char* name) {
  bool success;
  multiply = 100.0 / (options.boost * PA_VOLUME_NORM);

  mainloop = pa_glib_mainloop_new(g_main_context_default());
  success = mainloop != NULL;
  if(success) {
    context = pa_context_new(pa_glib_mainloop_get_api(mainloop), name);
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
 * Cleanup
 */
void pulseCleanup() {
  pa_context_disconnect(context);
  pa_context_unref(context);
  pa_glib_mainloop_free(mainloop);
}

/**
 * Get state
 */
struct pulseState_t pulseGetState()
{
  return state;
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

  if(options.verbose) {
  printf("Set %d to %d\n", channel, volume);
  }

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
 * Get list of devices for a channel (0 = sinks, 1 = sources)
 */
const struct pulseDeviceList_t* pulseGetDevices(int channel) {
  return channel == 0 ? &sinkList : &sourceList;
}

/**
 * Select a device as new default source or sink
 */
void pulseSelectDevice(struct pulseDevice_t* device) {
  if(device->deviceType == SINK && !device->isDefault) {
    if(options.verbose) printf("Change sink to: %s (%s)\n", device->name, device->description);
    pa_context_set_default_sink(context, device->name, callbackSetDevice, NULL);
  } else if(device->deviceType == SOURCE && !device->isDefault) {
    if(options.verbose) printf("Change source to: %s (%s)\n", device->name, device->description);
    pa_context_set_default_source(context, device->name, callbackSetDevice, NULL);
  }
}

/**
 * Callback when the state of the context changes.
 * Used to react on connect/disconnect
 */
static void callbackContextState(pa_context* c, void* userdata) {
  switch (pa_context_get_state(c)) {
  case PA_CONTEXT_READY: {
    if(options.verbose) {
      printf("PulseAudio connection ready.\n");
    }

    pa_operation *op = pa_context_get_server_info(context, callbackServerInfo, NULL);
    if (op) pa_operation_unref(op);

    pa_context_set_subscribe_callback(c, callbackContextSubscribe, NULL);
    op = pa_context_subscribe(c,PA_SUBSCRIPTION_MASK_SINK | PA_SUBSCRIPTION_MASK_SOURCE | PA_SUBSCRIPTION_MASK_SERVER, NULL, NULL);
    if (op) pa_operation_unref(op);

    break;
  }

  case PA_CONTEXT_FAILED:
  case PA_CONTEXT_TERMINATED:
    if (options.verbose) {
      fprintf(stderr, "PulseAudio connection lost.\n");
    }
    gtk_main_quit();
    break;

  default:
    break;
  }
}

/**
 * Callback when a subscription event occurs (volume change or server event)
 */
static void callbackContextSubscribe(pa_context *c, pa_subscription_event_type_t t, uint32_t idx, void *userdata) {
  bool isServer = (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) == PA_SUBSCRIPTION_EVENT_SERVER;
  bool isSink = (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) == PA_SUBSCRIPTION_EVENT_SINK;
  bool isSource = (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) == PA_SUBSCRIPTION_EVENT_SOURCE;
  bool isChange = (t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_CHANGE;

  if(isServer) {
    if(options.verbose) {
      printf("Server event\n");
    }
    pa_operation *op = pa_context_get_server_info(context, callbackServerInfo, NULL);
    if (op) pa_operation_unref(op);
  } else if(isChange && isSink && defaultSinkIndex == idx) {
    if(options.verbose) {
      printf("Default sink volume changed\n");
    }
    pa_operation *op = pa_context_get_sink_info_by_index(c, idx, callbackSinkInfo, NULL);
    if (op) pa_operation_unref(op);
  } else if(isChange && isSource && defaultSourceIndex == idx) {
    if(options.verbose) {
      printf("Default source volume changed\n");
    }
    pa_operation *op = pa_context_get_source_info_by_index(c, idx, callbackSourceInfo, NULL);
    if (op) pa_operation_unref(op);
  }
}

/**
 * Callback when sink info is received (requested when server info is received or the volume of the sink changes)
 */
static void callbackSinkInfo(pa_context *c, const pa_sink_info *info, int eol, void *userdata) {
  if(eol != 0 || info == NULL || (info->active_port && info->active_port->available == PA_PORT_AVAILABLE_NO)) return;

  bool isDefault = userdata != NULL ? strcmp(userdata, info->name) == 0 : true;
  pa_volume_t rawVol = pa_cvolume_avg(&info->volume);
  int volume = (int)(round(rawVol * multiply));

  if(options.verbose) {
    if(isDefault) printf("\033[32;1m");
    printf("Sink #%u\n", info->index);
    printf("  Name:        %s\n", info->name);
    if(userdata) {
      printf("  Default:     %s\n", isDefault ? "Yes": "No");
    }
    printf("  Description: %s\n", info->description);
    printf("  Sample Rate: %u Hz\n", info->sample_spec.rate);
    printf("  Channels:    %u\n", info->sample_spec.channels);
    printf("  Muted:       %s\n", info->mute ? "Yes" : "No");
    printf("  Volume:      %d%%\n", volume);
    if(isDefault) printf("\033[0m");
  }

  if(userdata) {
    sinkList.list = (struct pulseDevice_t*)realloc(sinkList.list, sizeof(struct pulseDevice_t) * (sinkList.count + 1));
    sinkList.list[sinkList.count].deviceType = SINK;
    sinkList.list[sinkList.count].name = malloc(strlen(info->name) + 1);
    strcpy(sinkList.list[sinkList.count].name, info->name);
    sinkList.list[sinkList.count].description = malloc(strlen(info->description) + 1);
    strcpy(sinkList.list[sinkList.count].description, info->description);
    sinkList.list[sinkList.count].isDefault = isDefault;
    sinkList.count ++;
  }

  if(isDefault) {
    defaultSinkIndex = info->index;
    defaultSinkChannels = info->channel_map.channels;

    state.sink.muted = info->mute;
    state.sink.volume = volume;
  }

  if(options.verbose) {
    printf("\n");
  }

  g_idle_add(windowRepaintEvent, NULL);
}

/**
 * Callback when source info is received (requested when server info is received or the volume of the source changes)
 */
static void callbackSourceInfo(pa_context *c, const pa_source_info *info, int eol, void *userdata) {
  if (eol < 0 || info == NULL) return;

  if(info->monitor_of_sink != PA_INVALID_INDEX) {
    if(options.verbose) {
      printf("Source #%u ignored (%s)\n\n", info->index, info->description);
    }
    return;
  }

  bool isDefault = userdata != NULL ? strcmp(userdata, info->name) == 0 : true;
  pa_volume_t rawVol = pa_cvolume_avg(&info->volume);
  int volume = (int)(round(rawVol * multiply));

  if(options.verbose) {
    if(isDefault) printf("\033[32;1m");
    printf("Source #%u\n", info->index);
    printf("  Name:        %s\n", info->name);
    if(userdata) {
      printf("  Default:     %s\n", isDefault ? "Yes": "No");
    }
    printf("  Description: %s\n", info->description);
    printf("  Sample Rate: %u Hz\n", info->sample_spec.rate);
    printf("  Channels:    %u\n", info->sample_spec.channels);
    printf("  Muted:       %s\n", info->mute ? "Yes" : "No");
    printf("  Volume:      %d%%\n", volume);
    if(isDefault) printf("\033[0m");
  }

  if(userdata) {
    sourceList.list = (struct pulseDevice_t*)realloc(sourceList.list, sizeof(struct pulseDevice_t) * (sourceList.count + 1));
    sourceList.list[sourceList.count].deviceType = SOURCE;
    sourceList.list[sourceList.count].name = malloc(strlen(info->name) + 1);
    strcpy(sourceList.list[sourceList.count].name, info->name);
    sourceList.list[sourceList.count].description = malloc(strlen(info->description) + 1);
    strcpy(sourceList.list[sourceList.count].description, info->description);
    sourceList.list[sourceList.count].isDefault = isDefault;
    sourceList.count ++;
  }

  if(isDefault) {
    defaultSourceIndex = info->index;
    defaultSourceChannels = info->channel_map.channels;

    state.source.muted = info->mute;
    state.source.volume = volume;
  }

  if(options.verbose) {
    printf("\n");
  }

  g_idle_add(windowRepaintEvent, NULL);
}

/**
 * Callback when server info is received (requested when a server event occurs, on on start)
 */
static void callbackServerInfo(pa_context *c, const pa_server_info *info, void *userdata) {
  if (!info) return;

  if(options.verbose) {
    printf("Received server info\n");
  }

  if(sinkList.list != NULL) {
    for(int i = 0; i < sinkList.count; ++i) {
      free(sinkList.list[i].name);
      free(sinkList.list[i].description);
    }
    free(sinkList.list);
    sinkList.list = NULL;
  }
  sinkList.count = 0;
  if(sourceList.list != NULL) {
    for(int i = 0; i < sourceList.count; ++i) {
      free(sourceList.list[i].name);
      free(sourceList.list[i].description);
    }
    free(sourceList.list);
    sourceList.list = NULL;
  }
  sourceList.count = 0;

  pa_operation *op = pa_context_get_source_info_list(c, callbackSourceInfo, (void*)info->default_source_name);
  if (op) pa_operation_unref(op);

  op = pa_context_get_sink_info_list(c, callbackSinkInfo, (void*)info->default_sink_name);
  if (op) pa_operation_unref(op);
}

/**
 * Callback for set_default_sink and set_default_source
 */
static void callbackSetDevice(pa_context* c, int success, void* userdata) {
  if(options.verbose) {
    printf("Change device: %s\n", success ? "success" : "failed");
  }
}
