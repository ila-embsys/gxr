/*
 * gulkan
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#version 460 core

layout (location = 0) in vec4 in_positon;

layout (location = 0) out vec4 out_color;

void
main ()
{
  out_color = vec4 (in_positon);
}
