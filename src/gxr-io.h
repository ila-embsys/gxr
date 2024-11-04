/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_IO_H_
#define GXR_IO_H_

#if !defined(GXR_INSIDE) && !defined(GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include <glib-object.h>

/**
 * gxr_io_create_directory_if_needed:
 * @path: The path of the directory to create.
 * @returns: `TRUE` if the directory was created successfully, `FALSE` otherwise.
 *
 * Creates a directory if it doesn't already exist.
 */
gboolean
gxr_io_create_directory_if_needed (gchar *path);

/**
 * gxr_io_write_resource_to_file:
 * @res_base_path: The base path of the resource.
 * @cache_path: The path of the cache directory.
 * @file_name: The name of the file to write.
 * @file_path: (out): A pointer to store the path of the written file.
 * @returns: `TRUE` if the resource was written successfully, `FALSE` otherwise.
 *
 * Writes a resource to a file in the cache directory.
 */
gboolean
gxr_io_write_resource_to_file (const gchar *res_base_path,
                               gchar       *cache_path,
                               const gchar *file_name,
                               GString     *file_path);

/**
 * gxr_io_get_cache_path:
 * @dir_name: The name of the directory.
 * @returns: A newly allocated #GString containing the path of the cache directory.
 *
 * Gets the path of the cache directory.
 */
GString *
gxr_io_get_cache_path (const gchar *dir_name);

#endif /* GXR_IO_H_ */
