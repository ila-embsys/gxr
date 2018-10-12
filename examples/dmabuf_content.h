/*
 * OpenVR GLib
 * Copyright 2014 Rob Clark <robdclark@gmail.com>
 * Copyright 2018 Collabora Ltd.
 * Authors: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

/*
 * From egl_dma_buf.c demo
 */

#ifndef OPENVR_GLIB_DMABUF_CONTENT_H_
#define OPENVR_GLIB_DMABUF_CONTENT_H_

#include <stdint.h>

void
dma_buf_fill (char *pixels, uint32_t width, uint32_t height, int stride)
{
  uint32_t i, j;
  /* paint the buffer with a colored gradient */
  for (j = 0; j < height; j++)
    {
      uint32_t *fb_ptr = (uint32_t*)(pixels + j * stride);
      for (i = 0; i < width; i++)
        fb_ptr[i] = 0xff000000 + 0x00000001 * (i * 255 / width)
                               + 0x00000100 * (j * 255 / height);
    }
}

#endif /* OPENVR_GLIB_DMABUF_CONTENT_H_ */
