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
  uniform mat4 mv_matrix[2];
  uniform mat4 mvp_matrix[2];
  uniform mat3 normal_matrix[2];
};

layout (location = 0) in vec4 in_position;
layout (location = 1) in vec4 in_color;
layout (location = 2) in vec3 in_normal;

layout (location = 0) out vec3 positon_view;
layout (location = 1) out vec3 normal_view;
layout (location = 2) out vec4 diffuse_color;

void
main ()
{
  normal_view = normal_matrix[gl_ViewIndex] * in_normal;
  vec4 positon_view_vec4 = mv_matrix[gl_ViewIndex] * in_position;
  positon_view = positon_view_vec4.xyz / positon_view_vec4.w;

  diffuse_color = in_color;

  gl_Position = mvp_matrix[gl_ViewIndex] * in_position;
}
