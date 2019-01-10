#include <glib.h>

#include "openvr-math.h"

void
openvr_math_print_matrix34 (HmdMatrix34_t mat)
{
  for (int i = 0; i < 4; i++)
    g_print ("| %+.6f %+.6f %+.6f |\n", mat.m[0][i], mat.m[1][i], mat.m[2][i]);
}

void
openvr_math_print_point3d (graphene_point3d_t *point)
{
  g_print ("| %+.6f %+.6f %+.6f |\n", point->x, point->y, point->z);
}

GString *
openvr_math_vec3_to_string (graphene_vec3_t *vec)
{
  GString *string = g_string_new ("");
  g_string_printf (string, "[%f, %f, %f]",
                   graphene_vec3_get_x (vec),
                   graphene_vec3_get_y (vec),
                   graphene_vec3_get_z (vec));
  return string;
}

void
openvr_math_graphene_to_matrix34 (graphene_matrix_t *mat, HmdMatrix34_t *mat34)
{
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 4; j++)
      mat34->m[i][j] = graphene_matrix_get_value (mat, j, i);
}

void
openvr_math_matrix34_to_graphene (HmdMatrix34_t *mat34, graphene_matrix_t *mat)
{
  float m[16] = {
    mat34->m[0][0], mat34->m[1][0], mat34->m[2][0], 0,
    mat34->m[0][1], mat34->m[1][1], mat34->m[2][1], 0,
    mat34->m[0][2], mat34->m[1][2], mat34->m[2][2], 0,
    mat34->m[0][3], mat34->m[1][3], mat34->m[2][3], 1
  };
  graphene_matrix_init_from_float (mat, m);
}

void
openvr_math_matrix44_to_graphene (HmdMatrix44_t *mat44, graphene_matrix_t *mat)
{
  float m[16] = {
    mat44->m[0][0], mat44->m[1][0], mat44->m[2][0], mat44->m[3][0],
    mat44->m[0][1], mat44->m[1][1], mat44->m[2][1], mat44->m[3][1],
    mat44->m[0][2], mat44->m[1][2], mat44->m[2][2], mat44->m[3][2],
    mat44->m[0][3], mat44->m[1][3], mat44->m[2][3], mat44->m[3][3]
  };
  graphene_matrix_init_from_float (mat, m);
}

gboolean
openvr_math_direction_from_matrix_vec3 (graphene_matrix_t *matrix,
                                        graphene_vec3_t   *start,
                                        graphene_vec3_t   *direction)
{
  graphene_quaternion_t orientation;
  graphene_quaternion_init_from_matrix (&orientation, matrix);

  // construct a matrix that only contains rotation instead of
  // rotation + translation like transform
  graphene_matrix_t rotation_matrix;
  graphene_matrix_init_identity (&rotation_matrix);
  graphene_matrix_rotate_quaternion (&rotation_matrix, &orientation);

  graphene_matrix_transform_vec3 (&rotation_matrix, start, direction);
  return TRUE;
}

gboolean
openvr_math_direction_from_matrix (graphene_matrix_t *matrix,
                                   graphene_vec3_t   *direction)
{
  // in openvr, the neutral forward direction is along the -z axis
  graphene_vec3_t start;
  graphene_vec3_init (&start, 0, 0, -1);

  return openvr_math_direction_from_matrix_vec3 (matrix, &start, direction);
}

gboolean
openvr_math_pose_to_matrix (TrackedDevicePose_t *pose,
                            graphene_matrix_t   *transform)
{
  if (pose->eTrackingResult != ETrackingResult_TrackingResult_Running_OK)
    {
      // g_print("Tracking result: Not running ok: %d!\n",
      //  pose->eTrackingResult);
      return FALSE;
    }
  else if (pose->bPoseIsValid)
    {
      openvr_math_matrix34_to_graphene (&pose->mDeviceToAbsoluteTracking,
                                        transform);
      return TRUE;
    }
  else
    {
      // g_print("controller: Pose invalid\n");
      return FALSE;
    }
}

void
openvr_math_matrix_set_translation (graphene_matrix_t  *matrix,
                                    graphene_point3d_t *point)
{
  float m[16];
  graphene_matrix_to_float (matrix, m);

  m[12] = point->x;
  m[13] = point->y;
  m[14] = point->z;

  graphene_matrix_init_from_float (matrix, m);
}

void
openvr_math_matrix_get_translation (graphene_matrix_t *matrix,
                                    graphene_vec3_t   *vec)
{
  graphene_vec3_init (vec,
                      graphene_matrix_get_value (matrix, 3, 0),
                      graphene_matrix_get_value (matrix, 3, 1),
                      graphene_matrix_get_value (matrix, 3, 2));
}

void
openvr_math_matrix_interpolate (graphene_matrix_t *from,
                                graphene_matrix_t *to,
                                float interpolation,
                                graphene_matrix_t *result)
{
  float from_f[16];
  float to_f[16];
  float interpolated_f[16];

  graphene_matrix_to_float (from, from_f);
  graphene_matrix_to_float (to, to_f);

  for (uint32_t i = 0; i < 16; i++)
    interpolated_f[i] = from_f[i] * (1.0f - interpolation) +
                        to_f[i] * interpolation;

  graphene_matrix_init_from_float (result, interpolated_f);
}

/** openvr_math_worldspace_to_screenspace:
 * Projects the worldspace point into screen space. Set input factor w = 1.0
 * for standard usage. The perspective scaling with w is performed by this
 * function, w is only returned in case the caller is interested what it was. */
void
openvr_math_worldspace_to_screenspace (graphene_point3d_t *worldspace_point,
                                       graphene_matrix_t  *camera_transform,
                                       graphene_matrix_t  *projection_matrix,
                                       graphene_point3d_t *screenspace_point,
                                       float              *w)
{
  /* transform intersection point into camera space: (camera transform)^-1 */
  graphene_matrix_t camera_transform_inv;
  graphene_matrix_inverse (camera_transform, &camera_transform_inv);
  graphene_matrix_transform_point3d (&camera_transform_inv, worldspace_point,
                                     screenspace_point);

  /* Project the HMD relative intersection point onto the HMD display.
   * To get the actual position on the HMD, divide by the w factor. */
  graphene_vec4_t screenspace_vec4;
  graphene_vec4_init (&screenspace_vec4,
                      screenspace_point->x,
                      screenspace_point->y,
                      screenspace_point->z, *w);
  graphene_matrix_transform_vec4 (projection_matrix, &screenspace_vec4,
                                  &screenspace_vec4);

  *w = graphene_vec4_get_w (&screenspace_vec4);

  graphene_vec4_scale (&screenspace_vec4, 1. / *w, &screenspace_vec4);

  graphene_point3d_init (screenspace_point,
                         graphene_vec4_get_x (&screenspace_vec4),
                         graphene_vec4_get_y (&screenspace_vec4),
                         graphene_vec4_get_z (&screenspace_vec4));
}

/** openvr_math_screenspace_to_worldspace:
 * Projects a screen space point into world space. Requires input factor w for
 * reversing the perspective projection.
 * After this function, w is usually 1.0. */
void
openvr_math_screenspace_to_worldspace (graphene_point3d_t *screenspace_point,
                                       graphene_matrix_t  *camera_transform,
                                       graphene_matrix_t  *projection_matrix,
                                       graphene_point3d_t *worldspace_point,
                                       float              *w)
{
  graphene_matrix_t projection_matrix_inv;
  graphene_matrix_inverse (projection_matrix, &projection_matrix_inv);

  graphene_vec4_t world_space_vec4;
  graphene_vec4_init (&world_space_vec4,
                      screenspace_point->x,
                      screenspace_point->y,
                      screenspace_point->z,
                      1.0);
  graphene_vec4_scale (&world_space_vec4, *w, &world_space_vec4);


  graphene_matrix_transform_vec4 (&projection_matrix_inv, &world_space_vec4,
                                  &world_space_vec4);
  graphene_matrix_transform_vec4 (camera_transform, &world_space_vec4,
                                  &world_space_vec4);

  graphene_point3d_init (worldspace_point,
                         graphene_vec4_get_x (&world_space_vec4),
                         graphene_vec4_get_y (&world_space_vec4),
                         graphene_vec4_get_z (&world_space_vec4));
  *w = graphene_vec4_get_w (&world_space_vec4);
}

bool
openvr_math_matrix_equals (graphene_matrix_t *a,
                           graphene_matrix_t *b)
{
  float a_f[16];
  float b_f[16];

  graphene_matrix_to_float (a, a_f);
  graphene_matrix_to_float (b, b_f);

  for (uint32_t i = 0; i < 16; i++)
    if (a_f[i] != b_f[i])
      return FALSE;

  return TRUE;
}

float
openvr_math_point_matrix_distance (graphene_point3d_t *intersection_point,
                                   graphene_matrix_t  *pose)
{
  graphene_vec3_t intersection_vec;
  graphene_point3d_to_vec3 (intersection_point, &intersection_vec);

  graphene_vec3_t pose_translation;
  openvr_math_matrix_get_translation (pose, &pose_translation);

  graphene_vec3_t distance_vec;
  graphene_vec3_subtract (&pose_translation,
                          &intersection_vec,
                          &distance_vec);

  return graphene_vec3_length (&distance_vec);
}
