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

void
dma_buf_fill (char *pixels, uint32_t width, uint32_t height, int stride)
{
  uint32_t i, j;
  /* paint the buffer with colored tiles */
  for (j = 0; j < height; j++)
    {
      uint32_t *fb_ptr = (uint32_t*)(pixels + j * stride);
      for (i = 0; i < width; i++)
        {
          div_t d = div((int) (i + j), (int) width);
          fb_ptr[i] = 0x00130502 * ((uint32_t) d.quot >> 6) +
                      0x000a1120 * ((uint32_t) d.rem >> 6);
        }
    }
}

#endif /* OPENVR_GLIB_DMABUF_CONTENT_H_ */
