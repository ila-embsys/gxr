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
openvr_io_write_resource_to_file (const gchar *res_base_path,
                                  gchar *cache_path,
                                  const gchar *file_name,
                                  GString *file_path);

GString*
openvr_io_get_cache_path (const gchar* dir_name);

gboolean
openvr_io_load_cached_action_manifest (const char* cache_name,
                                       const char* resource_path,
                                       const char* manifest_name,
                                       const char* first_binding,
                                       ...);

#endif /* OPENVR_GLIB_IO_H_ */
