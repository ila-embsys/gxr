/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib.h>
#include <gdk/gdk.h>

#include <fcntl.h>

#include "openvr-glib.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <vulkan/vulkan.h>

GdkPixbuf *pixbuf;
GulkanClient *uploader;

GulkanTexture *gk_texture;
OpenVROverlay *overlay;
GLuint gl_texture;

void
create_overlay ()
{
  uint32_t width = gdk_pixbuf_get_width (pixbuf);
  uint32_t height = gdk_pixbuf_get_height (pixbuf);

  guchar* rgb = gdk_pixbuf_get_pixels (pixbuf);

  GulkanClient *client = GULKAN_CLIENT (uploader);
  GulkanDevice *device = gulkan_client_get_device (client);

  gsize size;
  int fd;
  gk_texture = gulkan_texture_new_export_fd (device, width, height,
                                             VK_FORMAT_R8G8B8A8_UNORM, &size,
                                             &fd);
  g_print ("Mem size: %lu\n", size);
  if (gk_texture == NULL)
    {
      g_printerr ("Unable to initialize vulkan dmabuf texture.\n");
      return;
    }

  GLint gl_dedicated_mem = GL_TRUE;
  GLuint gl_mem_object;
  glCreateMemoryObjectsEXT (1, &gl_mem_object);
  glMemoryObjectParameterivEXT (
      gl_mem_object, GL_DEDICATED_MEMORY_OBJECT_EXT, &gl_dedicated_mem);

  glMemoryObjectParameterivEXT (
      gl_mem_object, GL_DEDICATED_MEMORY_OBJECT_EXT, &gl_dedicated_mem);
  glGetMemoryObjectParameterivEXT (
      gl_mem_object, GL_DEDICATED_MEMORY_OBJECT_EXT, &gl_dedicated_mem);

  glImportMemoryFdEXT (gl_mem_object, size, GL_HANDLE_TYPE_OPAQUE_FD_EXT, fd);


  glGenTextures (1, &gl_texture);
  glActiveTexture (GL_TEXTURE0);
  glBindTexture (GL_TEXTURE_2D, gl_texture);
  glTexStorageMem2DEXT (
      GL_TEXTURE_2D, 1, GL_RGBA8, width, height, gl_mem_object, 0);

  /* don't need to keep this around */
  glDeleteMemoryObjectsEXT (1, &gl_mem_object);

  /* this does not seem to be necessary to get everything in linear memory but
   * for now we leave it here */
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_TILING_EXT, GL_LINEAR_TILING_EXT);

  glTexSubImage2D (GL_TEXTURE_2D, 0 ,0, 0, width, height, GL_RGBA,
                   GL_UNSIGNED_BYTE, (GLvoid*)rgb);


  if (!gulkan_client_transfer_layout (client,
                                      gk_texture,
                                      VK_IMAGE_LAYOUT_UNDEFINED,
                                      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL))
    {
      g_printerr ("Unable to transfer layout.\n");
    }


  overlay = openvr_overlay_new ();
  openvr_overlay_create_width (overlay, "vulkan.dmabuf", "Vulkan DMABUF", 2.0);

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

  guint64 start = g_get_monotonic_time ();
  openvr_overlay_submit_texture (overlay, uploader, gk_texture);
  guint64 end = g_get_monotonic_time ();
  g_print ("Submit frame took %f ms\n",
           (end - start) / (1000.));

  openvr_overlay_show (overlay);
}

void
destroy_overlay ()
{
  g_object_unref (overlay);
  g_object_unref (gk_texture);
  glDeleteTextures (1, &gl_texture);
  overlay = NULL;
}

gboolean change_callback()
{
  if (!overlay)
    return FALSE;

  uint32_t width = gdk_pixbuf_get_width (pixbuf);
  uint32_t height = gdk_pixbuf_get_height (pixbuf);

  GdkPixbuf *original = pixbuf;
  pixbuf = gdk_pixbuf_flip  (pixbuf, TRUE);
  g_object_unref (original);

  guchar *rgb = gdk_pixbuf_get_pixels (pixbuf);

  glTexSubImage2D (GL_TEXTURE_2D, 0 ,0, 0, width, height, GL_RGBA,
                   GL_UNSIGNED_BYTE, (GLvoid*)rgb);

  openvr_overlay_submit_texture (overlay, uploader, gk_texture);

  return TRUE;
}

gboolean
timeout_callback ()
{
  if (overlay)
    {
      guint64 start = g_get_monotonic_time ();
      destroy_overlay ();
      guint64 end = g_get_monotonic_time ();
      g_print ("Destroy overlay took %f ms\n",
               (end - start) / (1000.));
    }
  else
    {
      guint64 start = g_get_monotonic_time ();
      create_overlay ();
      g_timeout_add (250, change_callback, NULL);
      guint64 end = g_get_monotonic_time ();
      g_print ("Create overlay took %f ms\n",
               (end - start) / (1000.));
    }

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

GdkPixbuf *
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
    GdkPixbuf *pixbuf = gdk_pixbuf_add_alpha (pixbuf_unflipped, false, 0, 0, 0);
    g_object_unref (pixbuf_unflipped);
    return pixbuf;
  }
}

int
main ()
{
  GMainLoop *loop = g_main_loop_new (NULL, FALSE);

  /* init openvr */
  if (!_init_openvr ())
    return -1;

  uploader = openvr_compositor_gulkan_client_new (true);
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

  glfwWindowHint (GLFW_VISIBLE, GLFW_FALSE);

  GLFWwindow *window = glfwCreateWindow(
      640, 480, "GLFW OpenGL & Vulkan", NULL, NULL);

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
  g_print ("OpenGL Version : %s\n", version);

  create_overlay ();

  g_timeout_add (1000, timeout_callback, NULL);

  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_print ("bye\n");

  g_object_unref (overlay);
  g_object_unref (gk_texture);
  g_object_unref (uploader);

  glDeleteTextures (1, &gl_texture);

  OpenVRContext *context = openvr_context_get_instance ();
  g_object_unref (context);

  return 0;
}
