/*
 * gxr
 * Copyright 2014 Rob Clark <robdclark@gmail.com>
 * Copyright 2018 Collabora Ltd.
 * Authors: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

/*
 * From egl_dma_buf.c demo
 */

#ifndef GXR_DMABUF_CONTENT_H_
#define GXR_DMABUF_CONTENT_H_

#include <stdint.h>

void
dma_buf_fill (char *pixels, uint32_t width, uint32_t height, uint32_t stride);

void
dma_buf_fill (char *pixels, uint32_t width, uint32_t height, uint32_t stride)
{
  uint32_t i, j;
  /* paint the buffer with a colored gradient */
  for (j = 0; j < height; j++)
    {
      /* pixel data is BGRA, each channel in a char. */
      char *fb_ptr = (char*)(pixels + j * stride);
      for (i = 0; i < width; i++)
        {
          fb_ptr[i * 4]     = 0;
          fb_ptr[i * 4 + 1] = (char) (i * 255 / width);
          fb_ptr[i * 4 + 2] = (char) (j * 255 / height);
          fb_ptr[i * 4 + 3] = (char) 255;
          //printf ("b %d\n", fb_ptr[i+3]);
        }
    }
}

#endif /* GXR_DMABUF_CONTENT_H_ */
