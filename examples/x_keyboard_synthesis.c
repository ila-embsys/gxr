/*
 * gxr
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

#include "gxr.h"

#include <graphene.h>

#define XK_XKB_KEYS
#define XK_MISCELLANY
#define XK_LATIN1
#include <X11/keysymdef.h>

#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

static Display *dpy;
static graphene_matrix_t keyboard_position;

typedef struct KeySymTable {
  KeySym *table;
  int min_keycode;
  //int max_keycode;
  int keysyms_per_keycode;
  int num_elements;

  XModifierKeymap *modmap;
} KeySymTable;

typedef struct ModifierKeyCodes {
  // there are only up to 8 modifier keys
  KeyCode key_codes[8];
  int length;
} ModifierKeyCodes;

static gboolean
timeout_callback (gpointer data)
{
  gxr_context_poll_event (GXR_CONTEXT (data));
  return TRUE;
}

static bool
_init_openvr (GxrContext *context)
{
  if (!gxr_context_init_runtime (context, GXR_APP_OVERLAY))
    {
      g_printerr ("Could not init OpenVR.\n");
      return false;
    }

  return true;
}

static void
_keyboard_closed (GxrContext  *context,
                  GdkEventKey *event,
                  gpointer     data)
{
  (void) event;
  (void) data;
  g_print ("Closed keyboard... Let's open it again!\n");
  gxr_context_show_keyboard (context);
  gxr_context_set_keyboard_transform (context, &keyboard_position);
}

ModifierKeyCodes
_modifier_keycode_for (KeySym sym, KeySymTable *keysym_table);

static void
_keyboard_input (GxrContext  *context,
                 GdkEventKey *event,
                 KeySymTable *keysym_table)
{
  (void) context;
  for (int char_index = 0; char_index < event->length; char_index++)
    {
      char c = event->string[char_index];
      //g_print ("Got char %d\n", c);

      // TODO: many openvr key codes match Latin 1 keysyms from X11/keysymdef.h
      // lucky coincidence or subject to change?
      KeySym key_sym;
      // OpenVR Backspace = 8 and XK_Backspace = 0xff08 etc.
      if (c >= 8 && c <= 17)
        key_sym = (KeySym) (0xff00 + c);
      else
        key_sym = (KeySym) c;

      // character 10 on the open vr keyboard (Line Feed)
      // should be return for us. There is no return on the openvr keyboard?!
      if (c == 10)
          key_sym = XK_Return;

      ModifierKeyCodes mod_key_codes =
        _modifier_keycode_for (key_sym, keysym_table);

      if (mod_key_codes.length == -1)
        {
          g_print ("There was an error, not synthing %c keysym %lu %s\n",
                   event->string[char_index], key_sym,
                   XKeysymToString (key_sym));
          return;
        }

      for (int i = 0; i < mod_key_codes.length; i++)
        {
          KeySym key_code = mod_key_codes.key_codes[i];
          //g_print ("%c: modkey %lu\n", c, key_code);
          XTestFakeKeyEvent (dpy, (guint) key_code, true, 0);
        }

      unsigned int key_code = XKeysymToKeycode (dpy, key_sym);
      //g_print ("%c (%d): keycode %d (keysym %d)\n", c, c, key_code, key_sym);
      XTestFakeKeyEvent (dpy, key_code, true, 0);
      XTestFakeKeyEvent (dpy, key_code, false, 0);

      for (int i = 0; i < mod_key_codes.length; i++)
        {
          KeySym key_code_to_send = mod_key_codes.key_codes[i];
          //g_print ("%c: modkey %lu\n", c, key_code);
          XTestFakeKeyEvent (dpy, (guint)key_code_to_send, false, 0);
        }

      XSync (dpy, false);
    }
}

static KeySymTable *
_init_keysym_table ()
{
  int min_keycode;
  int max_keycode;
  int keysyms_per_keycode;
  XDisplayKeycodes (dpy, &min_keycode, &max_keycode);


  int keycode_count = max_keycode - min_keycode + 1;
  KeySym *keysyms = XGetKeyboardMapping (dpy,
                                         (KeyCode) min_keycode,
                                         (KeyCode) keycode_count,
                                         &keysyms_per_keycode);

  int num_elements = keycode_count * keysyms_per_keycode;
/*
  g_print ("Keycodes: min %d, max %d\n", min_keycode, max_keycode);
  g_print ("Key Syms per key code: %d\n", keysyms_per_keycode);
  g_print ("Key code: syms\n");
  for (int keycode = min_keycode; keycode < num_elements / keysyms_per_keycode;
       keycode++)
    {
      g_print ("%3d ", keycode);
      for (int keysym_num = 0; keysym_num < keysyms_per_keycode; keysym_num++)
        {
          int index = (keycode - min_keycode) * keysyms_per_keycode +
            keysym_num;

          KeySym sym = keysyms[index];
          g_print ("| %-22s", sym == NoSymbol ? "" : XKeysymToString(sym));
        }
      g_print ("\n");
    }
*/

  XModifierKeymap *modmap = XGetModifierMapping(dpy);
/*
  g_print ("Modifiers (max %d keys per mod):\n", modmap->max_keypermod);
  for (int modifier = 0; modifier < 8; modifier++)
    {
      for (int mod_key = 0; mod_key < modmap->max_keypermod; mod_key++)
        {
          KeyCode keycode = modmap->modifiermap[modifier * modmap->max_keypermod + mod_key];
          int syms;
          if (keycode == 0) continue;
          KeySym *sym = XGetKeyboardMapping (dpy, keycode, 1, &syms);
          for (int i = 0; i < syms; i++)
            {
              if (sym[i] == NoSymbol) continue;
              g_print ("| %3d %-22s", keycode, XKeysymToString(sym[i]));
            }
        }
      g_print ("\n");
    }
*/
  KeySymTable *keysym_table = malloc (sizeof (KeySymTable));
  keysym_table->table = keysyms;
  keysym_table->min_keycode = min_keycode;
  //keysym_table->max_keycode = max_keycode;
  keysym_table->keysyms_per_keycode = keysyms_per_keycode;
  keysym_table->num_elements = num_elements;
  keysym_table->modmap = modmap;

  return keysym_table;
}

static int
_find_modifier_num (KeySymTable *keysym_table, KeySym sym)
{
  int min_keycode = keysym_table->min_keycode;
  int num_elements = keysym_table->num_elements;
  int keysyms_per_keycode = keysym_table->keysyms_per_keycode;
  KeySym *keysyms = keysym_table->table;
  for (int keycode = min_keycode; keycode < num_elements / keysyms_per_keycode;
     keycode++)
    {
      for (int keysym_num = 0; keysym_num < keysyms_per_keycode; keysym_num++)
        {

          // TODO: 2 (ctr+alt) or 3 (ctrl+alt+shift) are weird
          // XK_apostrophe (') on the german keyboard is Shift+#, but also the
          // weird Ctrl+Alt+ä combinatoin, which comes first, but only works in
          // some (?) apps, so skip the weird ones.
          if (keysym_num == 2 || keysym_num == 3) continue;

          int index = (keycode - min_keycode) * keysyms_per_keycode +
            keysym_num;
          if (keysyms[index] == sym)
            {
              int modifier_num = keysym_num;
              return modifier_num;
            }
        }
    }
  return -1;
}

ModifierKeyCodes
_modifier_keycode_for (KeySym sym, KeySymTable *keysym_table)
{
  /* each row of the key sym table is one key code
   * in each row there are up to "keysyms_per_keycode" key syms that are located
   * on the same physical key in the current layout
   *
   * for example in the german layout the row for the q key looks like this
   * key code 24; key syms: q, Q, q, Q, at, Greek_OMEGA, at
   * (note that XKeysymToString() omits the XK_ prefix that can be found in the
   * #define in keysymdef.h, e.g. the "at" key sym is defined as XK_at)
   *
   * _find_modifier_num() finds the index of the first applicable sym:
   * XK_q => 0
   * XK_Q => 1
   * XK_at => 4
   */
  int modifier_num = _find_modifier_num (keysym_table, sym);

  if (modifier_num == -1) {
    g_print ("ERROR: Did not find key sym!\n");
    return (ModifierKeyCodes) { .length = -1 };
  } else {
    //g_print ("Found key sym with mod number %d!\n", modifier_num);
  }

  /* There are up to 8 modifiers that can be enumerated (shift, ctrl, etc.)
   *
   * each of these 8 keys has up to "max_keypermod" "equivalent" modifiers
   * (e.g. left shift, right shift, etc.)
   *
   * TODO:
   * from XK_at appearing in column 4 of the table above we know that XK_at
   * can be achieved by q + modifier "4" and
   * I know from my keyboard that modifier 4 is the alt gr key (which has keysym
   * XK_ISO_Level3_Shift) but I do not know how to programatically find wich
   * entry of the XModifierKeymap entries below corresponds to e.g. column 4
   *
   * If modmap->modifiermap[modmap_index(4)] was XK_ISO_Level3_Shift we would
   * have solved the problem

  XModifierKeymap *modmap = keysym_table->modmap;
  // + 0: There are modmap->max_keypermod "equivalent" modifier keys.
  int modmap_index = modifier_num * modmap->max_keypermod + 0;
  KeyCode key_code = modmap->modifiermap[modmap_index];
  return key_code;
  */

  // for now hardcode the modifier key
  switch (modifier_num) {
  case 0:
    return (ModifierKeyCodes) {
      .key_codes[0] = XKeysymToKeycode (dpy, NoSymbol),
      .length = 1
    };
  case 1:
    return (ModifierKeyCodes) {
      .key_codes[0] = XKeysymToKeycode (dpy, XK_Shift_L),
      .length = 1
    };
  // TODO: are 2 and 3 correct? They are super weird!
  case 2:
    return (ModifierKeyCodes) {
      .key_codes[0] = XKeysymToKeycode (dpy, XK_Control_L),
      .key_codes[1] = XKeysymToKeycode (dpy, XK_Alt_L),
      .length = 2
    };
  case 3:
    return (ModifierKeyCodes) {
      .key_codes[0] = XKeysymToKeycode (dpy, XK_Shift_L),
      .key_codes[1] = XKeysymToKeycode (dpy, XK_Control_L),
      .key_codes[2] = XKeysymToKeycode (dpy, XK_Alt_L),
      .length = 3
    };
  case 4:
    return (ModifierKeyCodes) {
      .key_codes[0] = XKeysymToKeycode (dpy, XK_ISO_Level3_Shift),
      .length = 1
    };
  case 5:
    return (ModifierKeyCodes) {
      .key_codes[0] = XKeysymToKeycode (dpy, XK_Shift_L),
      .key_codes[1] = XKeysymToKeycode (dpy, XK_ISO_Level3_Shift),
      .length = 2
    };
  default:
    return (ModifierKeyCodes) { .length = 0 };
  }
}

int
main (int argc, char *argv[])
{
  (void) argc;
  (void) argv;
  GMainLoop *loop = g_main_loop_new (NULL, FALSE);

  dpy = XOpenDisplay (NULL);

  KeySymTable *keysym_table = _init_keysym_table ();
/*
  KeySym test_key = XK_at;
  ModifierKeyCodes modifier_key_code =
    _modifier_keycode_for (test_key, keysym_table);

  g_print ("Got %d modifier key codes: ", modifier_key_code.length);
  for (int i = 0; i < modifier_key_code.length; i++)
    {
      int syms;
      KeySym *sym =
        XGetKeyboardMapping (dpy, modifier_key_code.key_codes[i], 1, &syms);
      for (int i = 0; i < syms; i++)
        {
          if (sym[i] == NoSymbol) continue;
            g_print ("| %-22s", XKeysymToString(sym[i]));
        }
      g_print ("\n");
    }
*/

  GxrContext *context = gxr_context_new ();

  /* init openvr */
  if (!_init_openvr (context))
    return -1;

  gxr_context_show_keyboard (GXR_CONTEXT (context));

  graphene_point3d_t position = {
    .x = 0,
    .y = 1,
    .z = -1.5
  };
  graphene_matrix_init_translate (&keyboard_position, &position);
  gxr_context_set_keyboard_transform (context, &keyboard_position);

  g_signal_connect (context, "keyboard-press-event",
                    (GCallback) _keyboard_input, keysym_table);
  g_signal_connect (context, "keyboard-close-event",
                    (GCallback) _keyboard_closed, context);
  g_timeout_add (20, timeout_callback, context);

  g_main_loop_run (loop);
  g_main_loop_unref (loop);
  g_object_unref (context);

  return 0;
}
