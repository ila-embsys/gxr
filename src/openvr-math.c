#include <glib.h>

#include "openvr-math.h"

void
openvr_math_print_matrix34 (HmdMatrix34_t mat)
{
  for (int i = 0; i < 4; i++)
    g_print ("| %+.6f %+.6f %+.6f |\n", mat.m[0][i], mat.m[1][i], mat.m[2][i]);
}

void
openvr_math_graphene_to_matrix34 (graphene_matrix_t *mat, HmdMatrix34_t *mat34)
{
  for (guint i = 0; i < 3; i++)
    for (guint j = 0; j < 4; j++)
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

void
openvr_math_matrix_get_translation (graphene_matrix_t *matrix,
                                    graphene_vec3_t   *vec)
{
  graphene_vec3_init (vec,
                      graphene_matrix_get_value (matrix, 3, 0),
                      graphene_matrix_get_value (matrix, 3, 1),
                      graphene_matrix_get_value (matrix, 3, 2));
}





