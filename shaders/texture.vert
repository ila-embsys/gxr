/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#version 460
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 0) uniform Transformation {
  mat4 mvp;
} ubo;

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 uv;

layout (location = 0) out vec2 out_uv;

out gl_PerVertex {
  vec4 gl_Position;
};

void main() {
  gl_Position = ubo.mvp * vec4 (position, 0.0, 1.0);
  out_uv = uv;
}

