/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 *
 * This example shows the openvr keyboard and sends its input via XTest to the
 * currently focused window.
 */

#include <glib.h>
#include <glib/gprintf.h>
#include <gdk/gdk.h>

#include <openvr-glib.h>
#include "openvr-context.h"

#include <graphene.h>

#define XK_MISCELLANY
#include <X11/keysymdef.h>

#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

Display *dpy;
graphene_matrix_t keyboard_position;

gboolean
timeout_callback (OpenVRContext *context)
{
  openvr_context_poll_event (context);
  return TRUE;
}

bool
_init_openvr ()
{
  if (!openvr_context_is_installed ())
    {
      g_printerr ("VR Runtime not installed.\n");
      return false;
    }

  OpenVRContext *context = openvr_context_get_instance ();
  if (!openvr_context_init_overlay (context))
    {
      g_printerr ("Could not init OpenVR.\n");
      return false;
    }

  if (!openvr_context_is_valid (context))
    {
      g_printerr ("Could not load OpenVR function pointers.\n");
      return false;
    }

  return true;
}

static void
_keyboard_closed (OpenVRContext  *context,
                 GdkEventKey    *event,
                 gpointer        data)
{
  (void) event;
  (void) data;
  g_print ("Closed keyboard... Let's open it again!\n");
  openvr_context_show_system_keyboard (context);
  openvr_context_set_system_keyboard_transform (context, &keyboard_position);
}

static void
_keyboard_input (OpenVRContext  *context,
                 GdkEventKey    *event,
                 gpointer        data)
{
  (void) context;
  (void) data;
  for (int i = 0; i < event->length; i++)
    {
      char c = event->string[i];

      // TODO: many openvr key codes match Latin 1 keysyms from X11/keysymdef.h
      // lucky coincidence or subject to change?
      int key_sym;
      // OpenVR Backspace = 8 and XK_Backspace = 0xff08 etc.
      if (c >= 8 && c <= 17)
        key_sym = 0xff00 + c;
      else
        key_sym = c;

      // character 10 on the open vr keyboard (Line Feed)
      // should be return for us. There is no return on the openvr keyboard?!
      if (c == 10)
          key_sym = XK_Return;

      unsigned int key_code = XKeysymToKeycode (dpy, key_sym);

      g_print ("%c (%d): keycode %d (keysym %d)\n", c, c, key_code, key_sym);
      XTestFakeKeyEvent (dpy,key_code, true, 0);
      XTestFakeKeyEvent (dpy,key_code, false, 0);
      XSync (dpy, false);
    }
}

int
main (int argc, char *argv[])
{
  (void) argc;
  (void) argv;
  GMainLoop *loop = g_main_loop_new (NULL, FALSE);

  /* init openvr */
  if (!_init_openvr ())
    return -1;

  dpy = XOpenDisplay (NULL);

  OpenVRContext *context = openvr_context_get_instance ();
  openvr_context_show_system_keyboard (context);

  graphene_point3d_t position = {
    .x = 0,
    .y = 1,
    .z = -1.5
  };
  graphene_matrix_init_translate (&keyboard_position, &position);
  openvr_context_set_system_keyboard_transform (context, &keyboard_position);

  g_signal_connect (context, "keyboard-char-input-event",
                    (GCallback) _keyboard_input, NULL);
  g_signal_connect (context, "keyboard-closed-event",
                    (GCallback) _keyboard_closed, context);
  g_timeout_add (20, G_SOURCE_FUNC (timeout_callback), context);

  g_main_loop_run (loop);
  g_main_loop_unref (loop);
  g_object_unref (context);

  return 0;
}
