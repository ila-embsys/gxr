/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENVR_GLIB_IO_H_
#define OPENVR_GLIB_IO_H_

#include <glib-object.h>

gboolean
openvr_io_create_directory_if_needed (gchar *path);

gboolean
openvr_io_write_resource_to_file (gchar *res_base_path, gchar *cache_path,
                                  const gchar *file_name,
                                  GString *file_path);

GString*
openvr_io_get_cache_path (const gchar* dir_name);

#endif /* OPENVR_GLIB_IO_H_ */
