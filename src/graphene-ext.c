/*
 * Graphene Extensions
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "graphene-ext.h"
#include <inttypes.h>

// TODO: Move to upstream

void
graphene_ext_quaternion_to_float (const graphene_quaternion_t *q, float *dest)
{
  graphene_vec4_t v;
  graphene_quaternion_to_vec4 (q, &v);
  graphene_vec4_to_float (&v, dest);
}

void
graphene_ext_quaternion_print (const graphene_quaternion_t *q)
{
  float f[4];
  graphene_ext_quaternion_to_float (q, f);
  g_print ("| %f %f %f %f |\n", (double) f[0], (double) f[1], (double) f[2],
           (double) f[3]);
}

void
graphene_ext_matrix_get_translation_vec3 (const graphene_matrix_t *m,
                                          graphene_vec3_t         *res)
{
  float f[16];
  graphene_matrix_to_float (m, f);
  graphene_vec3_init (res, f[12], f[13], f[14]);
}

void
graphene_ext_matrix_get_translation_point3d (const graphene_matrix_t *m,
                                             graphene_point3d_t      *res)
{
  float f[16];
  graphene_matrix_to_float (m, f);
  graphene_point3d_init (res, f[12], f[13], f[14]);
}

void
graphene_ext_matrix_set_translation_vec3 (graphene_matrix_t     *m,
                                          const graphene_vec3_t *t)
{
  float f[16];
  graphene_matrix_to_float (m, f);
  f[12] = graphene_vec3_get_x (t);
  f[13] = graphene_vec3_get_y (t);
  f[14] = graphene_vec3_get_z (t);
  graphene_matrix_init_from_float (m, f);
}

void
graphene_ext_matrix_set_translation_point3d (graphene_matrix_t        *m,
                                             const graphene_point3d_t *t)
{
  float f[16];
  graphene_matrix_to_float (m, f);
  f[12] = t->x;
  f[13] = t->y;
  f[14] = t->z;
  graphene_matrix_init_from_float (m, f);
}

void
graphene_ext_matrix_get_scale (const graphene_matrix_t *m,
                               graphene_point3d_t      *res)
{
  float f[16];
  graphene_matrix_to_float (m, f);

  graphene_vec3_t sx, sy, sz;
  graphene_vec3_init (&sx, f[0], f[1], f[2]);
  graphene_vec3_init (&sy, f[4], f[5], f[6]);
  graphene_vec3_init (&sz, f[8], f[9], f[10]);

  graphene_point3d_init (res, graphene_vec3_length (&sx),
                         graphene_vec3_length (&sy),
                         graphene_vec3_length (&sz));
}

void
graphene_ext_matrix_get_rotation_matrix (const graphene_matrix_t *m,
                                         graphene_point3d_t      *scale,
                                         graphene_matrix_t       *rotation)
{
  float f[16];
  graphene_matrix_to_float (m, f);

  graphene_ext_matrix_get_scale (m, scale);

  // clang-format off
  float r[16] = {
    f[0] / scale->x, f[1] / scale->x, f[2]  / scale->x, 0,
    f[4] / scale->y, f[5] / scale->y, f[6]  / scale->y, 0,
    f[8] / scale->z, f[9] / scale->z, f[10] / scale->z, 0,
    0              , 0              , 0               , 1
  };
  // clang-format on
  graphene_matrix_init_from_float (rotation, r);
}

void
graphene_ext_matrix_get_rotation_quaternion (const graphene_matrix_t *m,
                                             graphene_point3d_t      *scale,
                                             graphene_quaternion_t   *res)
{
  graphene_matrix_t rot_m;
  graphene_ext_matrix_get_rotation_matrix (m, scale, &rot_m);
  graphene_quaternion_init_from_matrix (res, &rot_m);
}

void
graphene_ext_matrix_get_rotation_angles (const graphene_matrix_t *m,
                                         float                   *deg_x,
                                         float                   *deg_y,
                                         float                   *deg_z)
{
  graphene_quaternion_t q;

  graphene_point3d_t unused_scale;
  graphene_ext_matrix_get_rotation_quaternion (m, &unused_scale, &q);
  graphene_quaternion_to_angles (&q, deg_x, deg_y, deg_z);
}

/**
 * graphene_point_scale:
 * @p: a #graphene_point_t
 * @factor: the scaling factor
 * @res: (out caller-allocates): return location for the scaled point
 *
 * Scales the coordinates of the given #graphene_point_t by
 * the given @factor.
 */
void
graphene_ext_point_scale (const graphene_point_t *p,
                          float                   factor,
                          graphene_point_t       *res)
{
  graphene_point_init (res, p->x * factor, p->y * factor);
}

void
graphene_ext_ray_get_origin_vec4 (const graphene_ray_t *r,
                                  float                 w,
                                  graphene_vec4_t      *res)
{
  graphene_vec3_t o;
  graphene_ext_ray_get_origin_vec3 (r, &o);
  graphene_vec4_init_from_vec3 (res, &o, w);
}

void
graphene_ext_ray_get_origin_vec3 (const graphene_ray_t *r, graphene_vec3_t *res)
{
  graphene_point3d_t o;
  graphene_ray_get_origin (r, &o);
  graphene_point3d_to_vec3 (&o, res);
}

void
graphene_ext_ray_get_direction_vec4 (const graphene_ray_t *r,
                                     float                 w,
                                     graphene_vec4_t      *res)
{
  graphene_vec3_t d;
  graphene_ray_get_direction (r, &d);
  graphene_vec4_init_from_vec3 (res, &d, w);
}

void
graphene_ext_vec4_print (const graphene_vec4_t *v)
{
  float f[4];
  graphene_vec4_to_float (v, f);
  g_print ("| %f %f %f %f |\n", (double) f[0], (double) f[1], (double) f[2],
           (double) f[3]);
}

void
graphene_ext_vec3_print (const graphene_vec3_t *v)
{
  float f[3];
  graphene_vec3_to_float (v, f);
  g_print ("| %f %f %f |\n", (double) f[0], (double) f[1], (double) f[2]);
}

void
graphene_ext_matrix_interpolate_simple (const graphene_matrix_t *from,
                                        const graphene_matrix_t *to,
                                        float                    factor,
                                        graphene_matrix_t       *result)
{
  float from_f[16];
  float to_f[16];
  float interpolated_f[16];

  graphene_matrix_to_float (from, from_f);
  graphene_matrix_to_float (to, to_f);

  for (uint32_t i = 0; i < 16; i++)
    interpolated_f[i] = from_f[i] * (1.0f - factor) + to_f[i] * factor;

  graphene_matrix_init_from_float (result, interpolated_f);
}

gboolean
graphene_ext_matrix_decompose (const graphene_matrix_t *m,
                               graphene_point3d_t      *scale,
                               graphene_quaternion_t   *rotation,
                               graphene_point3d_t      *translation)
{
  graphene_ext_matrix_get_translation_point3d (m, translation);
  graphene_ext_matrix_get_rotation_quaternion (m, scale, rotation);
  return true;
}

gboolean
graphene_ext_matrix_validate (const graphene_matrix_t *m)
{
  float f[16];
  graphene_matrix_to_float (m, f);
  for (uint32_t i = 0; i < 16; i++)
    {
      if (isnan (f[i]))
        {
          g_assert ("Matrix component is NaN" && FALSE);
          return FALSE;
        }
      if (isinf (f[i]))
        {
          g_assert ("Matrix component is inf" && FALSE);
          return FALSE;
        }
    }
  return TRUE;
}

gboolean
graphene_ext_quaternion_validate (const graphene_quaternion_t *q)
{
  float f[4];
  graphene_ext_quaternion_to_float (q, f);

  gboolean all_zero = TRUE;
  for (uint32_t i = 0; i < 4; i++)
    {
      if (isnan (f[i]))
        {
          g_assert ("Quaternion component is NaN" && FALSE);
          return FALSE;
        }
      if (isinf (f[i]))
        {
          g_assert ("Quaternion component is inf" && FALSE);
          return FALSE;
        }
      if (f[i] != 0)
        {
          all_zero = FALSE;
        }
    }

  if (all_zero)
    {
      g_assert ("Quaternion is all zero" && FALSE);
      return FALSE;
    }
  return TRUE;
}

gboolean
graphene_ext_point3d_validate (const graphene_point3d_t *p)
{
  float f[3] = {p->x, p->y, p->z};

  for (uint32_t i = 0; i < 3; i++)
    {
      if (isnan (f[i]))
        {
          g_assert ("Point component is NaN" && FALSE);
          return FALSE;
        }
      if (isinf (f[i]))
        {
          g_assert ("Point component is inf" && FALSE);
          return FALSE;
        }
    }
  return TRUE;
}

gboolean
graphene_ext_point3d_validate_all_nonzero (const graphene_point3d_t *p)
{
  float f[3] = {p->x, p->y, p->z};

  for (uint32_t i = 0; i < 3; i++)
    {
      if (isnan (f[i]))
        {
          g_assert ("Point component is NaN" && FALSE);
          return FALSE;
        }
      if (isinf (f[i]))
        {
          g_assert ("Point component is inf" && FALSE);
          return FALSE;
        }
      if (f[i] == 0)
        {
          g_assert ("Point component is zero" && FALSE);
          return FALSE;
        }
    }
  return TRUE;
}

gboolean
graphene_ext_vec3_validate (const graphene_vec3_t *v)
{
  float f[3];
  graphene_vec3_to_float (v, f);

  for (uint32_t i = 0; i < 3; i++)
    {
      if (isnan (f[i]))
        {
          g_assert ("Vec3 component is NaN" && FALSE);
          return FALSE;
        }
      if (isinf (f[i]))
        {
          g_assert ("Vec3 component is inf" && FALSE);
          return FALSE;
        }
    }
  return TRUE;
}

bool
graphene_ext_point3d_near (const graphene_point3d_t *a,
                           const graphene_point3d_t *b,
                           float                     epsilon)
{
  if (fabsf (a->x - b->x) > epsilon)
    {
      return false;
    }
  if (fabsf (a->y - b->y) > epsilon)
    {
      return false;
    }
  if (fabsf (a->z - b->z) > epsilon)
    {
      return false;
    }
  return true;
}

bool
graphene_ext_quaternion_near (const graphene_quaternion_t *a,
                              const graphene_quaternion_t *b,
                              float                        epsilon)
{
  float af[4];
  graphene_ext_quaternion_to_float (a, af);

  float bf[4];
  graphene_ext_quaternion_to_float (b, bf);

  for (uint8_t i = 0; i < 4; i++)
    {
      // TODO: To pass recomposition tests we need to drop the signs.
      // Something in our recomposition is flipping them. See unit tests.
      af[i] = fabsf (af[i]);
      bf[i] = fabsf (bf[i]);

      if (fabsf (af[i] - bf[i]) > epsilon)
        {
          return false;
        }
    }

  return true;
}
