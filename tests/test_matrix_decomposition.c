/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib.h>

#include "graphene-ext.h"

static void
_print_translation (graphene_matrix_t *m)
{
  graphene_vec3_t t;
  graphene_ext_matrix_get_translation_vec3 (m, &t);

  float f[3];
  graphene_vec3_to_float (&t, f);

  g_print ("Translation: [%f %f %f]\n", (double) f[0], (double) f[1],
           (double) f[2]);
}

static void
_print_scale (graphene_matrix_t *m)
{
  graphene_point3d_t s;
  graphene_ext_matrix_get_scale (m, &s);

  g_print ("Scale: [%f %f %f]\n", (double) s.x, (double) s.y, (double) s.z);
}

static void
_print_rotation (graphene_matrix_t *m)
{
  float x, y, z;
  graphene_ext_matrix_get_rotation_angles (m, &x, &y, &z);

  g_print ("Angles: [%f %f %f]\n", (double) x, (double) y, (double) z);
}

static void
_test_translation_rotation_scale ()
{
  graphene_matrix_t mat;
  graphene_matrix_init_identity (&mat);

  g_print ("Identity:\n");
  graphene_matrix_print (&mat);
  _print_translation (&mat);
  _print_scale (&mat);
  _print_rotation (&mat);

  graphene_matrix_init_scale (&mat, 1, 2, 3);
  g_print ("Scale:\n");
  graphene_matrix_print (&mat);
  _print_translation (&mat);
  _print_scale (&mat);
  _print_rotation (&mat);

  graphene_point3d_t point = {.x = 1, .y = 2, .z = 3};
  graphene_matrix_init_translate (&mat, &point);
  g_print ("Translation:\n");
  graphene_matrix_print (&mat);
  _print_translation (&mat);
  _print_scale (&mat);
  _print_rotation (&mat);

  graphene_quaternion_t orientation;

  graphene_quaternion_init_from_angles (&orientation, 1, 2, 3);
  graphene_matrix_init_identity (&mat);
  graphene_matrix_rotate_quaternion (&mat, &orientation);
  g_print ("Rotation:\n");

  graphene_matrix_print (&mat);
  _print_translation (&mat);
  _print_scale (&mat);
  _print_rotation (&mat);

  g_print ("Rotation quat: ");
  graphene_ext_quaternion_print (&orientation);
  graphene_quaternion_t q;

  graphene_point3d_t unused_scale;
  graphene_ext_matrix_get_rotation_quaternion (&mat, &unused_scale, &q);
  g_print ("Result quat: ");
  graphene_ext_quaternion_print (&q);

  graphene_quaternion_init_from_angles (&orientation, 1, 2, 3);
  graphene_matrix_init_identity (&mat);
  graphene_matrix_init_scale (&mat, 1, 2, 4);
  graphene_matrix_rotate_quaternion (&mat, &orientation);
  g_print ("Scaled rotation:\n");

  graphene_matrix_print (&mat);
  _print_translation (&mat);
  _print_scale (&mat);
  _print_rotation (&mat);

  g_print ("Rotation quat: ");
  graphene_ext_quaternion_print (&orientation);

  graphene_ext_matrix_get_rotation_quaternion (&mat, &unused_scale, &q);
  g_print ("Result quat: ");
  graphene_ext_quaternion_print (&q);
}

static bool
_does_recompose (graphene_matrix_t *m)
{
  graphene_point3d_t    pos;
  graphene_quaternion_t rot;
  graphene_point3d_t    scale;

  graphene_ext_matrix_decompose (m, &scale, &rot, &pos);

  graphene_matrix_t recomposed;
  graphene_matrix_init_scale (&recomposed, scale.x, scale.y, scale.z);
  graphene_matrix_rotate_quaternion (&recomposed, &rot);
  graphene_matrix_translate (&recomposed, &pos);

  graphene_point3d_t    scale_r;
  graphene_quaternion_t rot_r;
  graphene_point3d_t    pos_r;
  graphene_ext_matrix_decompose (&recomposed, &scale_r, &rot_r, &pos_r);

  const float epsilon = 0.1f;

  if (!graphene_ext_point3d_near (&pos, &pos_r, epsilon))
    {
      return false;
    }

  if (!graphene_ext_quaternion_near (&rot, &rot_r, epsilon))
    {
      return false;
    }

  if (!graphene_ext_point3d_near (&scale, &scale_r, epsilon))
    {
      return false;
    }

  return true;
}

static void
_test_recompose ()
{
  // clang-format off
  float m[16] = {
    +0.727574f, -0.046446f, +0.680384f, +0.000000f,
    -0.220853f, +0.812468f, +0.241867f, +0.000000f,
    -0.573449f, -0.324470f, +0.573523f, +0.000000f,
    +2.769740f, +2.492541f, -2.140265f, +1.000000f,
  };

  float m2[16] = {
    +0.527033f, +0.218246f, -0.696080f, +0.000000f,
    -0.163887f, +0.926114f, +0.042852f, +0.000000f,
    +0.710689f, +0.196215f, +0.518015f, +0.000000f,
    -4.600538f, -0.035784f, -3.095682f, +1.000000f,
  };

  // Flips one sign in quaternion
  float m3[16] = {
    -0.710718f, -0.121415f, +0.692921f, +0.000000f,
    -0.121415f, -0.949041f, -0.290825f, +0.000000f,
    +0.692921f, -0.290825f, +0.659759f, +0.000000f,
    -7.132993f, +4.231433f, -7.415701f, +1.000000f,
  };

  // Flips two signs in quaternion
  float m4[16] = {
    +0.883787f, +0.421399f, -0.203328f, +0.000000f,
    +0.421399f, -0.905734f, -0.045484f, +0.000000f,
    -0.203328f, -0.045484f, -0.978054f, +0.000000f,
    +0.495907f, +1.366194f, +0.923377f, +1.000000f,
  };
  // clang-format on

  graphene_matrix_t mat;
  graphene_matrix_init_from_float (&mat, m);
  g_assert (_does_recompose (&mat));

  graphene_matrix_init_from_float (&mat, m2);
  g_assert (_does_recompose (&mat));

  graphene_matrix_init_from_float (&mat, m3);
  g_assert (_does_recompose (&mat));

  graphene_matrix_init_from_float (&mat, m4);
  g_assert (_does_recompose (&mat));
}

int
main ()
{
  _test_translation_rotation_scale ();
  _test_recompose ();
  return 0;
}
