/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <X11/Xlib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char** argv)
{
  int window_id = -1;
  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "-window") == 0 && (i < argc - 1)) {
      window_id = strtol(argv[i+1], NULL, 10);
    }
  }
  if (window_id == -1) {
    printf("Use -window <WindowId>. Use xwininfo -int\n");
    return 0;
  }

  Display *display = XOpenDisplay(NULL);
  Window root_window = RootWindow(display, DefaultScreen(display));


  XEvent event = {0};
  event.type = ButtonPress;
  event.xbutton.display = display;
  event.xbutton.button = Button3;
  event.xbutton.same_screen = True;
  event.xbutton.root = root_window;
  event.xbutton.window = window_id;
  event.xbutton.subwindow = event.xbutton.window;
  event.xbutton.x = 100;
  event.xbutton.y = 100;

  printf(
    "Sending events to window %d: Button %d at %d,%d relative to the window\n",
    window_id, event.xbutton.button, event.xbutton.x, event.xbutton.y);
  int res = XSendEvent(display, window_id, True, 0xfff, &event);
  if (res == Success) {
  } else {
    //printf("Sending the event failed!\n");
  }
  XFlush(display);
}
