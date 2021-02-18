/*
 * gulkan
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#version 460 core

layout (std140, set = 0, binding = 0) uniform block
{
  uniform mat4 mvp;
};

layout (location = 0) in vec4 in_position;

layout (location = 0) out vec4 positon;

void
main ()
{
  positon = in_position;
  gl_Position = mvp * in_position;
}
