/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib.h>
#include <gdk/gdk.h>

#include <fcntl.h>

#include "gxr.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>

static GdkPixbuf *pixbuf;
static GulkanClient *uploader;

static GulkanTexture *texture;
static OpenVROverlay *overlay;
static GLuint gl_texture;

static void
create_overlay ()
{
  guint width = (guint)gdk_pixbuf_get_width (pixbuf);
  guint height = (guint) gdk_pixbuf_get_height (pixbuf);

  guchar* rgba = gdk_pixbuf_get_pixels (pixbuf);

  GulkanClient *client = GULKAN_CLIENT (uploader);
  GulkanDevice *device = gulkan_client_get_device (client);

  glGenTextures (1, &gl_texture);
  glActiveTexture (GL_TEXTURE0);
  glBindTexture (GL_TEXTURE_2D, gl_texture);

  /* TODO: investigate why this does not make the buffer storage linear.
   * Workaround: R600_DEBUG=notiling */
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_TILING_EXT, GL_LINEAR_TILING_EXT);

  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)width, (GLsizei)height, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)rgba);

  PFNEGLEXPORTDRMIMAGEMESAPROC _eglExportDMABUFImageMESA =
      (PFNEGLEXPORTDMABUFIMAGEMESAPROC)
          eglGetProcAddress ("eglExportDMABUFImageMESA");
  if (!_eglExportDMABUFImageMESA)
    {
      g_printerr ("Unable to load function eglExportDMABUFImageMESA.");
      return;
    }

  EGLDisplay eglDisplay = eglGetCurrentDisplay ();
  EGLContext eglContext = eglGetCurrentContext ();

  EGLImage egl_image = eglCreateImage (eglDisplay,
                                       eglContext,
                                       EGL_GL_TEXTURE_2D,
                                       (EGLClientBuffer)(ulong)gl_texture,
                                       NULL);

  int fd;
  if (!_eglExportDMABUFImageMESA (eglDisplay, egl_image, &fd, 0, 0)) {
    g_printerr ("Unable to export DMABUF\n");
    return;
  }

  eglDestroyImage (eglDisplay, egl_image);

  texture = gulkan_texture_new_from_dmabuf (device,
                                            fd, width, height,
                                            VK_FORMAT_R8G8B8A8_UNORM);
  if (texture == NULL)
    {
      g_printerr ("Unable to initialize vulkan dmabuf texture.\n");
      return;
    }

  if (!gulkan_client_transfer_layout (client,
                                      texture,
                                      VK_IMAGE_LAYOUT_UNDEFINED,
                                      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL))
    {
      g_printerr ("Unable to transfer layout.\n");
    }

  overlay = openvr_overlay_new ();
  openvr_overlay_create_width (overlay,
                               "vulkan.dmabuf",
                               "Vulkan DMABUF",
                               2.0);

  if (!openvr_overlay_is_valid (overlay))
    {
      g_printerr ("Overlay unavailable.\n");
      return;
    }

  graphene_matrix_t transform;
  graphene_point3d_t pos =
  {
    .x = 0,
    .y = 1,
    .z = -2
  };
  graphene_matrix_init_translate (&transform, &pos);
  openvr_overlay_set_transform_absolute (overlay, &transform);

  gint64 start = g_get_monotonic_time ();
  openvr_overlay_submit_texture (overlay, uploader, texture);
  gint64 end = g_get_monotonic_time ();
  g_print ("Submit frame took %f ms\n",
           (end - start) / (1000.));

  openvr_overlay_show (overlay);
}

static void
destroy_overlay ()
{
  g_object_unref (overlay);
  g_object_unref (texture);
  glDeleteTextures (1, &gl_texture);
  overlay = NULL;
}

static gboolean
overlay_change_callback ()
{
  if (!overlay)
    return FALSE;

  openvr_overlay_submit_texture (overlay, uploader, texture);

  return TRUE;
}

static gboolean
timeout_callback ()
{
  if (overlay)
    {
      gint64 start = g_get_monotonic_time ();
      destroy_overlay ();
      gint64 end = g_get_monotonic_time ();
      g_print ("Destroy overlay took %f ms\n",
               (end - start) / (1000.));
    }
  else
    {
      gint64 start = g_get_monotonic_time ();
      create_overlay ();
      /* submit the same gulkan texture periodically every 100 ms
       * possibly after having written some different data into its memory */
      g_timeout_add (100, overlay_change_callback, NULL);
      gint64 end = g_get_monotonic_time ();
      g_print ("Create overlay took %f ms\n",
               (end - start) / (1000.));
    }

  return TRUE;
}

static bool
_init_openvr ()
{
  OpenVRContext *context = openvr_context_get_instance ();
  if (!openvr_context_initialize (context, GXR_APP_OVERLAY))
    {
      g_printerr ("Could not init OpenVR.\n");
      return false;
    }

  return true;
}

static GdkPixbuf *
load_gdk_pixbuf ()
{
  GError *error = NULL;
  GdkPixbuf * pixbuf_unflipped =
    gdk_pixbuf_new_from_resource ("/res/cat.jpg", &error);

  if (error != NULL) {
    g_printerr ("Unable to read file: %s\n", error->message);
    g_error_free (error);
    return NULL;
  } else {
    GdkPixbuf *pb = gdk_pixbuf_add_alpha (pixbuf_unflipped, false, 0, 0, 0);
    g_object_unref (pixbuf_unflipped);
    return pb;
  }
}

int
main ()
{
  /* TODO: radeon specific workaround for enforcing no tiling */
  putenv ("R600_DEBUG=notiling");

  GMainLoop *loop = g_main_loop_new (NULL, FALSE);

  /* init openvr */
  if (!_init_openvr ())
    return -1;

  uploader = openvr_compositor_gulkan_client_new ();
  if (!uploader)
  {
    g_printerr ("Unable to initialize Vulkan!\n");
    return false;
  }

  pixbuf = load_gdk_pixbuf ();
  if (pixbuf == NULL)
    return -1;

  if (!glfwInit ()) {
    g_printerr ("Unable to initialize GLFW3\n");
    return 1;
  }
  glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  /* eglExportDMABUFImageMESA requires EGL GL context */
  glfwWindowHint (GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);

  /* window doesn't do anything, just for OpenGL context */
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

  GLFWwindow *window = glfwCreateWindow (640, 480, "EGL window", NULL, NULL);
  if (!window) {
    g_printerr ("Unable to create window with GLFW3\n");
    glfwTerminate ();
    return 1;
  }
  glfwMakeContextCurrent (window);

  glewInit ();

  const GLubyte* renderer = glGetString (GL_RENDERER);
  const GLubyte* version = glGetString (GL_VERSION);
  g_print ("OpengL Renderer: %s\n", renderer);
  g_print ("OpenGL version : %s\n", version);

  create_overlay ();

  g_timeout_add (1000, timeout_callback, NULL);

  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_print ("bye\n");

  g_object_unref (overlay);
  g_object_unref (texture);
  g_object_unref (uploader);

  OpenVRContext *context = openvr_context_get_instance ();
  g_object_unref (context);

  return 0;
}
