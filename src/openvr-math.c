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
