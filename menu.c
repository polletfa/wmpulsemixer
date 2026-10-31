// wmpulsemixer - A simple PulseAudio mixer for WindowMaker
// Copyright (C) 2026  Fabien Pollet <mail@frmpollet.me> (wmpulsemixer)
// Copyright (C) 2003  Damian Kramer <psiren@hibernaculum.net> (wmsmixer)
// Copyright (C) 1998  Sam Hawker <shawkie@geocities.com> (wmmixer)
// This software comes with ABSOLUTELY NO WARRANTY
// This software is free software, and you are welcome to redistribute it
// under certain conditions
// See the COPYING file for a more complete notice.

#include "menu.h"
#include "pulse.h"

#include <gtk/gtk.h>

static GtkWidget *menu = NULL;

// forward declarations for private functions
static void menuOnSelect(GtkMenuItem* menuitem, gpointer userdata);
static void menuCreate(int curchannel);

static void menuOnSelect(GtkMenuItem* menuitem, gpointer userdata) {
  pulseSelectDevice(userdata);
}

static void menuCreate(int curchannel) {
  menu = gtk_menu_new();
  g_object_add_weak_pointer(G_OBJECT(menu), (gpointer*)&menu); // set NULL on destroy
  g_signal_connect(menu, "selection-done", G_CALLBACK(gtk_widget_destroy), NULL);

  const struct pulseDeviceList_t* devices = pulseGetDevices(curchannel);

  for(int i = 0; i < devices->count; ++i) {
    GtkWidget *item;
    if(devices->list[i].isDefault) {
      item = gtk_menu_item_new();
      GtkWidget* label = gtk_label_new("");
      gtk_label_set_xalign(GTK_LABEL(label), 0.0);
      gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
      char* markup = g_strdup_printf("<b>%s</b>", devices->list[i].description);
      gtk_label_set_markup(GTK_LABEL(label), markup);
      g_free(markup);
      gtk_container_add(GTK_CONTAINER(item), label);
    } else {
      item = gtk_menu_item_new_with_label(devices->list[i].description);
    }
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    g_signal_connect(item, "activate", G_CALLBACK(menuOnSelect), devices->list + i);
  }

  gtk_widget_show_all(menu);
}

void menuOpen(int curchannel, GdkEvent* event) {
  if(menu) {
    // menu is open: just close, don't open a new menu
    gtk_widget_destroy(menu);
  } else {
    menuCreate(curchannel);
    gtk_menu_popup_at_pointer(GTK_MENU(menu), event);
  }
}
