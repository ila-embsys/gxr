/*
 * gulkan
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#version 460 core

#extension GL_EXT_multiview : enable

layout (std140, set = 0, binding = 0) uniform block
{
  uniform mat4 mvp[2];
};

layout (location = 0) in vec4 in_position;

layout (location = 0) out vec4 color;

void
main ()
{
  color = in_position;
  gl_Position = mvp[gl_ViewIndex] * in_position;
}
