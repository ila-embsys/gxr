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
    mat34->m[0][0],
    mat34->m[1][0],
    mat34->m[2][0],
    0,
    mat34->m[0][1],
    mat34->m[1][1],
    mat34->m[2][1],
    0,
    mat34->m[0][2],
    mat34->m[1][2],
    mat34->m[2][2],
    0,
    mat34->m[0][3],
    mat34->m[1][3],
    mat34->m[2][3],
    1
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

