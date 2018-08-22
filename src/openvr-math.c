#include <glib.h>

#include "openvr-math.h"

void
openvr_math_print_matrix34 (HmdMatrix34_t mat)
{
  g_print ("%f %f %f\n", mat.m[0][0], mat.m[1][0], mat.m[2][0]);
  g_print ("%f %f %f\n", mat.m[0][1], mat.m[1][1], mat.m[2][1]);
  g_print ("%f %f %f\n", mat.m[0][2], mat.m[1][2], mat.m[2][2]);
  g_print ("%f %f %f\n", mat.m[0][3], mat.m[1][3], mat.m[2][3]);
}

void
openvr_math_graphene_to_matrix34 (graphene_matrix_t *mat, HmdMatrix34_t *mat34)
{
  mat34->m[0][0] = graphene_matrix_get_value (mat, 0, 0);
  mat34->m[0][1] = graphene_matrix_get_value (mat, 1, 0);
  mat34->m[0][2] = graphene_matrix_get_value (mat, 2, 0);
  mat34->m[0][3] = graphene_matrix_get_value (mat, 3, 0);

  mat34->m[1][0] = graphene_matrix_get_value (mat, 0, 1);
  mat34->m[1][1] = graphene_matrix_get_value (mat, 1, 1);
  mat34->m[1][2] = graphene_matrix_get_value (mat, 2, 1);
  mat34->m[1][3] = graphene_matrix_get_value (mat, 3, 1);

  mat34->m[2][0] = graphene_matrix_get_value (mat, 0, 2);
  mat34->m[2][1] = graphene_matrix_get_value (mat, 1, 2);
  mat34->m[2][2] = graphene_matrix_get_value (mat, 2, 2);
  mat34->m[2][3] = graphene_matrix_get_value (mat, 3, 2);
}

// translation vector
gboolean
openvr_math_vec3_init_from_matrix (graphene_vec3_t   *vec,
                                   graphene_matrix_t *matrix)
{
  graphene_vec3_init (vec,
                      graphene_matrix_get_value (matrix, 3, 0),
                      graphene_matrix_get_value (matrix, 3, 1),
                      graphene_matrix_get_value (matrix, 3, 2));
  return TRUE;
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
      float mat4x4[16] = {
        pose->mDeviceToAbsoluteTracking.m[0][0],
        pose->mDeviceToAbsoluteTracking.m[1][0],
        pose->mDeviceToAbsoluteTracking.m[2][0],
        0,
        pose->mDeviceToAbsoluteTracking.m[0][1],
        pose->mDeviceToAbsoluteTracking.m[1][1],
        pose->mDeviceToAbsoluteTracking.m[2][1],
        0,
        pose->mDeviceToAbsoluteTracking.m[0][2],
        pose->mDeviceToAbsoluteTracking.m[1][2],
        pose->mDeviceToAbsoluteTracking.m[2][2],
        0,
        pose->mDeviceToAbsoluteTracking.m[0][3],
        pose->mDeviceToAbsoluteTracking.m[1][3],
        pose->mDeviceToAbsoluteTracking.m[2][3],
        1
      };

      graphene_matrix_init_from_float (transform, mat4x4);

      return TRUE;
    }
  else
    {
      // g_print("controller: Pose invalid\n");
      return FALSE;
    }
}
