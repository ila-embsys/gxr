#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>

#include <openvr-glib.h>

#include "openvr-system.h"
#include "openvr-overlay.h"

#include <GLFW/glfw3.h>

static GdkPixbuf *pixbuf;
static OpenVROverlay *overlay;


gboolean
timeout_callback (gpointer data)
{
  openvr_overlay_query_events (overlay, pixbuf);
  return TRUE;
}

void
print_pixbuf_info (GdkPixbuf * pixbuf)
{
  gint n_channels = gdk_pixbuf_get_n_channels (pixbuf);

  GdkColorspace colorspace = gdk_pixbuf_get_colorspace (pixbuf);

  gint bits_per_sample = gdk_pixbuf_get_bits_per_sample (pixbuf);
  gboolean has_alpha = gdk_pixbuf_get_has_alpha (pixbuf);

  gint width = gdk_pixbuf_get_width (pixbuf);
  gint height = gdk_pixbuf_get_height (pixbuf);

  gint rowstride = gdk_pixbuf_get_rowstride (pixbuf);

  g_print ("We loaded a pixbuf.\n");
  g_print ("channels %d\n", n_channels);
  g_print ("colorspace %d\n", colorspace);
  g_print ("bits_per_sample %d\n", bits_per_sample);
  g_print ("has_alpha %d\n", has_alpha);
  g_print ("pixel dimensions %dx%d\n", width, height);
  g_print ("rowstride %d\n", rowstride);
}

void
print_gl_context_info ()
{
  const GLubyte *version = glGetString (GL_VERSION);
  const GLubyte *vendor = glGetString (GL_VENDOR);
  const GLubyte *renderer = glGetString (GL_RENDERER);

  GLint major;
  GLint minor;
  glGetIntegerv (GL_MAJOR_VERSION, &major);
  glGetIntegerv (GL_MINOR_VERSION, &minor);

  g_print ("%s %s %s (%d.%d)\n", vendor, renderer, version, major, minor);
}

GdkPixbuf *
load_gdk_pixbuf ()
{
  GError *error = NULL;
  GdkPixbuf * pixbuf_unflipped =
    gdk_pixbuf_new_from_resource ("/res/cat.jpg", &error);

  if (error != NULL) {
    fprintf (stderr, "Unable to read file: %s\n", error->message);
    g_error_free (error);
    return NULL;
  } else {
    GdkPixbuf * pixbuf = gdk_pixbuf_flip (pixbuf_unflipped, FALSE);
    g_object_unref (pixbuf_unflipped);
    return pixbuf;
  }
}

static void
glfw_error_callback (int error, const char* description)
{
  fprintf (stderr, "GLFW Error: %s\n", description);
}

gboolean
init_offscreen_gl ()
{
  glfwSetErrorCallback (glfw_error_callback);

  if (!glfwInit ())
    return FALSE;

  glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 5);

  glfwWindowHint (GLFW_VISIBLE, GLFW_FALSE);
  GLFWwindow* offscreen_context = glfwCreateWindow (640, 480, "", NULL, NULL);

  if (!offscreen_context)
  {
    glfwTerminate ();
    return FALSE;
  }

  glfwMakeContextCurrent (offscreen_context);

  return TRUE;
}

static void
_move_cb (OpenVROverlay  *overlay,
          GdkEventMotion *event,
          gpointer        data)
{
  g_print ("move: %f %f (%d)\n", event->x, event->y, event->time);
}

static void
_press_cb (OpenVROverlay  *overlay,
           GdkEventButton *event,
           gpointer        data)
{
  g_print ("press: %d %f %f (%d)\n",
           event->button, event->x, event->y, event->time);
  GMainLoop *loop = (GMainLoop*) data;
  g_main_loop_quit (loop);
}

static void
_release_cb (OpenVROverlay  *overlay,
             GdkEventButton *event,
             gpointer        data)
{
  g_print ("release: %d %f %f (%d)\n",
           event->button, event->x, event->y, event->time);
}

static void
_show_cb (OpenVROverlay *ovl,
          gpointer       data)
{
  g_print ("show\n");
  //openvr_overlay_upload_gdk_pixbuf (overlay, pixbuf);
}

static void
_destroy_cb (OpenVROverlay *overlay,
             gpointer       data)
{
  g_print ("destroy\n");
  GMainLoop *loop = (GMainLoop*) data;
  g_main_loop_quit (loop);
}

int
test_cat_overlay ()
{
  GMainLoop *loop;

  pixbuf = load_gdk_pixbuf ();
  print_pixbuf_info (pixbuf);

  if (pixbuf == NULL)
    return -1;

  loop = g_main_loop_new (NULL, FALSE);

  if (!init_offscreen_gl ())
  {
    fprintf (stderr, "Error: Could not create GLFW context.\n");
    return FALSE;
  }

  print_gl_context_info ();

  OpenVRSystem * system = openvr_system_new ();
  gboolean ret = openvr_system_init_overlay (system);

  g_assert (ret);
  g_assert (openvr_system_is_available ());
  g_assert (openvr_system_is_compositor_available ());

  overlay = openvr_overlay_new ();
  openvr_overlay_create (overlay, "test.cat", "Cat");
  openvr_overlay_set_mouse_scale (overlay, pixbuf);

  g_signal_connect (overlay, "motion-notify-event", (GCallback) _move_cb, NULL);
  g_signal_connect (overlay, "button-press-event", (GCallback) _press_cb, loop);
  g_signal_connect (
    overlay, "button-release-event", (GCallback) _release_cb, NULL);
  g_signal_connect (overlay, "show", (GCallback) _show_cb, NULL);
  g_signal_connect (overlay, "destroy", (GCallback) _destroy_cb, loop);

  g_timeout_add (20, timeout_callback, NULL);
  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_print ("bye\n");

  g_object_unref (overlay);
  g_object_unref (pixbuf);

  return 0;
}

int main (int argc, char *argv[]) {
  return test_cat_overlay ();
}
