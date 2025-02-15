/*
 * gulkan
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#version 460
#extension GL_ARB_separate_shader_objects : enable

#extension GL_EXT_multiview : enable

layout (binding = 0) uniform Transformation { mat4 mvp[2]; }
ubo;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;

layout (location = 0) out vec4 out_color;

out gl_PerVertex { vec4 gl_Position; };

void
main ()
{

  gl_Position = ubo.mvp[gl_ViewIndex] * vec4 (position, 1.0);
  out_color = vec4 (color, 1.0);
}
