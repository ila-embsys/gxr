/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib.h>
#include <gdk/gdk.h>

#include <amdgpu_drm.h>
#include <amdgpu.h>

#include <fcntl.h>

#include <openvr-glib.h>

#include "openvr-system.h"
#include "openvr-overlay.h"
#include "openvr-compositor.h"
#include "openvr-vulkan-uploader.h"

#include "dmabuf_content.h"

OpenVRVulkanTexture *texture;

gboolean
timeout_callback (gpointer data)
{
  OpenVROverlay *overlay = (OpenVROverlay*) data;
  openvr_overlay_poll_event (overlay);
  return TRUE;
}

void*
allocate_dmabuf_amd (int size, int *fd)
{
  amdgpu_device_handle amd_dev;
  amdgpu_bo_handle amd_bo;

  /* use render node to avoid needing to authenticate: */
  int dev_fd = open ("/dev/dri/renderD128", 02, 0);

  uint32_t major_version;
  uint32_t minor_version;
  int ret = amdgpu_device_initialize (dev_fd,
                                     &major_version,
                                     &minor_version,
                                     &amd_dev);
  if (ret < 0)
    {
      g_printerr ("Could not create amdgpu device: %s\n", strerror (-ret));
      return NULL;
    }

  g_print ("Initialized amdgpu drm device with fd %d. Version %d.%d\n",
           dev_fd, major_version, minor_version);

  struct amdgpu_bo_alloc_request alloc_buffer =
  {
    .alloc_size = (uint64_t) size,
    .preferred_heap = AMDGPU_GEM_DOMAIN_GTT,
  };

  ret = amdgpu_bo_alloc (amd_dev, &alloc_buffer, &amd_bo);
  if (ret < 0)
    {
      g_printerr ("amdgpu_bo_alloc failed: %s\n", strerror(-ret));
      return NULL;
    }

  uint32_t shared_handle;
  ret = amdgpu_bo_export (amd_bo,
                          amdgpu_bo_handle_type_dma_buf_fd,
                         &shared_handle);

  if (ret < 0)
    {
      g_printerr ("amdgpu_bo_export failed: %s\n", strerror (-ret));
      return NULL;
    }

  *fd = (int) shared_handle;
  void *cpu_buffer;
  ret = amdgpu_bo_cpu_map (amd_bo, &cpu_buffer);

  if (ret < 0)
    {
      g_printerr ("amdgpu_bo_cpu_map failed: %s\n", strerror (-ret));
      return NULL;
    }

  return cpu_buffer;
}

static void
_move_cb (OpenVROverlay  *overlay,
          GdkEventMotion *event,
          gpointer        data)
{
  // g_print ("move: %f %f (%d)\n", event->x, event->y, event->time);
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
_show_cb (OpenVROverlay *overlay,
          gpointer       data)
{
  g_print ("show\n");

  /* skip rendering if the overlay isn't available or visible */
  gboolean is_unavailable = !openvr_overlay_is_valid (overlay) ||
                            !openvr_overlay_is_available (overlay);
  gboolean is_invisible = !openvr_overlay_is_visible (overlay) &&
                          !openvr_overlay_thumbnail_is_visible (overlay);

  if (is_unavailable || is_invisible)
    return;

  OpenVRVulkanUploader * uploader = (OpenVRVulkanUploader*) data;

  openvr_vulkan_uploader_submit_frame (uploader, overlay, texture);
}

static void
_destroy_cb (OpenVROverlay *overlay,
             gpointer       data)
{
  g_print ("destroy\n");
  GMainLoop *loop = (GMainLoop*) data;
  g_main_loop_quit (loop);
}

#define ALIGN(_v, _d) (((_v) + ((_d) - 1)) & ~((_d) - 1))

int main (int argc, char *argv[]) {
  GMainLoop *loop = g_main_loop_new (NULL, FALSE);

  /* create dmabuf */
  uint32_t width = 512;
  uint32_t height = 512;

  int fd;
  int stride = ALIGN ((int) width, 32) * 4;
  uint64_t size = (uint64_t) stride * height;
  char* map = (char*) allocate_dmabuf_amd (size, &fd);

  dma_buf_fill (map, width, height, stride);

  OpenVRSystem * system = openvr_system_new ();
  gboolean ret = openvr_system_init_overlay (system);

  OpenVRCompositor *compositor = openvr_compositor_new ();

  g_assert (ret);
  g_assert (openvr_system_is_available (system));
  g_assert (openvr_system_is_installed ());

  OpenVRVulkanUploader *uploader = openvr_vulkan_uploader_new ();
  if (!openvr_vulkan_uploader_init_vulkan (uploader, true, system, compositor))
  {
    g_printerr ("Unable to initialize Vulkan!\n");
    return false;
  }

  texture = openvr_vulkan_texture_new ();
  if (!openvr_vulkan_uploader_load_dmabuf (uploader, texture,
                                           fd, width, height,
                                           VK_FORMAT_B8G8R8A8_UNORM))
    {
      g_printerr ("Unable to initialize vulkan dmabuf texture.\n");
      return -1;
    }


  OpenVROverlay *overlay = openvr_overlay_new ();
  openvr_overlay_create_for_dashboard (overlay, "vulkan.dmabuf", "Vulkan DMABUF");

  if (!openvr_overlay_is_valid (overlay) ||
      !openvr_overlay_is_available (overlay))
    {
      g_printerr ("Overlay unavailable.\n");
      return -1;
    }

  /*
  openvr_overlay_set_mouse_scale (overlay,
                                  (float) gdk_pixbuf_get_width (pixbuf),
                                  (float) gdk_pixbuf_get_height (pixbuf));
  */

  g_signal_connect (overlay, "motion-notify-event", (GCallback) _move_cb, NULL);
  g_signal_connect (overlay, "button-press-event", (GCallback) _press_cb, loop);
  g_signal_connect (
    overlay, "button-release-event", (GCallback) _release_cb, NULL);
  g_signal_connect (overlay, "show", (GCallback) _show_cb, uploader);
  g_signal_connect (overlay, "destroy", (GCallback) _destroy_cb, loop);

  g_timeout_add (20, timeout_callback, overlay);
  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_print ("bye\n");

  g_object_unref (overlay);
  g_object_unref (texture);
  g_object_unref (uploader);

  return 0;
}
