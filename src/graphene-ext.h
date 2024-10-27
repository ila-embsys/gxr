/*
 * Graphene Extensions
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_GRAPHENE_EXT_H_
#define XRD_GRAPHENE_EXT_H_

#include <glib.h>
#include <graphene.h>

/**
 * graphene_ext_quaternion_to_float:
 * @q: The quaternion.
 * @dest: (out): A pointer to store the float array.
 *
 * Converts a quaternion to a float array.
 */
void
graphene_ext_quaternion_to_float (const graphene_quaternion_t *q, float *dest);

/**
 * graphene_ext_quaternion_print:
 * @q: The quaternion.
 *
 * Prints the quaternion to stdout.
 */
void
graphene_ext_quaternion_print (const graphene_quaternion_t *q);

/**
 * graphene_ext_matrix_get_translation_vec3:
 * @m: The matrix.
 * @res: (out): A pointer to store the translation vector.
 *
 * Gets the translation vector of the matrix.
 */
void
graphene_ext_matrix_get_translation_vec3 (const graphene_matrix_t *m,
                                          graphene_vec3_t         *res);

/**
 * graphene_ext_matrix_get_translation_point3d:
 * @m: The matrix.
 * @res: (out): A pointer to store the translation point.
 *
 * Gets the translation point of the matrix.
 */
void
graphene_ext_matrix_get_translation_point3d (const graphene_matrix_t *m,
                                             graphene_point3d_t      *res);

/**
 * graphene_ext_matrix_set_translation_vec3:
 * @m: The matrix.
 * @t: The translation vector.
 *
 * Sets the translation vector of the matrix.
 */
void
graphene_ext_matrix_set_translation_vec3 (graphene_matrix_t     *m,
                                          const graphene_vec3_t *t);

/**
 * graphene_ext_matrix_set_translation_point3d:
 * @m: The matrix.
 * @t: The translation point.
 *
 * Sets the translation point of the matrix.
 */
void
graphene_ext_matrix_set_translation_point3d (graphene_matrix_t        *m,
                                             const graphene_point3d_t *t);

/**
 * graphene_ext_matrix_get_scale:
 * @m: The matrix.
 * @res: (out): A pointer to store the scale.
 *
 * Gets the scale of the matrix.
 */
void
graphene_ext_matrix_get_scale (const graphene_matrix_t *m,
                               graphene_point3d_t      *res);

/**
 * graphene_ext_matrix_get_rotation_matrix:
 * @m: The matrix.
 * @scale: (out): A pointer to store the scale.
 * @rotation: (out): A pointer to store the rotation matrix.
 *
 * Gets the rotation matrix of the matrix.
 */
void
graphene_ext_matrix_get_rotation_matrix (const graphene_matrix_t *m,
                                         graphene_point3d_t      *scale,
                                         graphene_matrix_t       *rotation);

/**
 * graphene_ext_matrix_get_rotation_quaternion:
 * @m: The matrix.
 * @scale: (out): A pointer to store the scale.
 * @res: (out): A pointer to store the rotation quaternion.
 *
 * Gets the rotation quaternion of the matrix.
 */
void
graphene_ext_matrix_get_rotation_quaternion (const graphene_matrix_t *m,
                                             graphene_point3d_t      *scale,
                                             graphene_quaternion_t   *res);

/**
 * graphene_ext_matrix_get_rotation_angles:
 * @m: The matrix.
 * @deg_x: (out): A pointer to store the rotation angle around the x-axis in degrees.
 * @deg_y: (out): A pointer to store the rotation angle around the y-axis in degrees.
 * @deg_z: (out): A pointer to store the rotation angle around the z-axis in degrees.
 *
 * Gets the rotation angles of the matrix.
 */
void
graphene_ext_matrix_get_rotation_angles (const graphene_matrix_t *m,
                                         float                   *deg_x,
                                         float                   *deg_y,
                                         float                   *deg_z);

/**
 * graphene_ext_point_scale:
 * @p: The point.
 * @factor: The scale factor.
 * @res: (out): A pointer to store the scaled point.
 *
 * Scales the point.
 */
void
graphene_ext_point_scale (const graphene_point_t *p,
                          float                   factor,
                          graphene_point_t       *res);

/**
 * graphene_ext_ray_get_origin_vec4:
 * @r: The ray.
 * @w: The w component of the vector.
 * @res: (out): A pointer to store the origin vector.
 *
 * Gets the origin vector of the ray.
 */
void
graphene_ext_ray_get_origin_vec4 (const graphene_ray_t *r,
                                  float                 w,
                                  graphene_vec4_t      *res);

/**
 * graphene_ext_ray_get_origin_vec3:
 * @r: The ray.
 * @res: (out): A pointer to store the origin vector.
 *
 * Gets the origin vector of the ray.
 */
void
graphene_ext_ray_get_origin_vec3 (const graphene_ray_t *r,
                                  graphene_vec3_t      *res);

/**
 * graphene_ext_ray_get_direction_vec4:
 * @r: The ray.
 * @w: The w component of the vector.
 * @res: (out): A pointer to store the direction vector.
 *
 * Gets the direction vector of the ray.
 */
void
graphene_ext_ray_get_direction_vec4 (const graphene_ray_t *r,
                                     float                 w,
                                     graphene_vec4_t      *res);

/**
 * graphene_ext_vec4_print:
 * @v: The vector.
 *
 * Prints the vector to stdout.
 */
void
graphene_ext_vec4_print (const graphene_vec4_t *v);

/**
 * graphene_ext_vec3_print:
 * @v: The vector.
 *
 * Prints the vector to stdout.
 */
void
graphene_ext_vec3_print (const graphene_vec3_t *v);

/**
 * graphene_ext_matrix_interpolate_simple:
 * @from: The starting matrix.
 * @to: The ending matrix.
 * @factor: The interpolation factor.
 * @result: (out): A pointer to store the interpolated matrix.
 *
 * Interpolates between two matrices.
 */
void
graphene_ext_matrix_interpolate_simple (const graphene_matrix_t *from,
                                        const graphene_matrix_t *to,
                                        float                    factor,
                                        graphene_matrix_t       *result);

/**
 * graphene_ext_matrix_decompose:
 * @m: The matrix.
 * @scale: (out): A pointer to store the scale.
 * @rotation: (out): A pointer to store the rotation quaternion.
 * @translation: (out): A pointer to store the translation.
 * @returns: `TRUE` if the matrix was decomposed successfully, `FALSE` otherwise.
 *
 * Decomposes the matrix into its scale, rotation, and translation components.
 */
gboolean
graphene_ext_matrix_decompose (const graphene_matrix_t *m,
                               graphene_point3d_t      *scale,
                               graphene_quaternion_t   *rotation,
                               graphene_point3d_t      *translation);

/**
 * graphene_ext_matrix_validate:
 * @m: The matrix.
 * @returns: `TRUE` if the matrix is valid, `FALSE` otherwise.
 *
 * Validates the matrix.
 */
gboolean
graphene_ext_matrix_validate (const graphene_matrix_t *m);

/**
 * graphene_ext_quaternion_validate:
 * @q: The quaternion.
 * @returns: `TRUE` if the quaternion is valid, `FALSE` otherwise.
 *
 * Validates the quaternion.
 */
gboolean
graphene_ext_quaternion_validate (const graphene_quaternion_t *q);

/**
 * graphene_ext_point3d_validate:
 * @p: The point.
 * @returns: `TRUE` if the point is valid, `FALSE` otherwise.
 *
 * Validates the point.
 */
gboolean
graphene_ext_point3d_validate (const graphene_point3d_t *p);

/**
 * graphene_ext_point3d_validate_all_nonzero:
 * @p: The point.
 * @returns: `TRUE` if all components of the point are non-zero, `FALSE` otherwise.
 *
 * Validates that all components of the point are non-zero.
 */
gboolean
graphene_ext_point3d_validate_all_nonzero (const graphene_point3d_t *p);

/**
 * graphene_ext_vec3_validate:
 * @v: The vector.
 * @returns: `TRUE` if the vector is valid, `FALSE` otherwise.
 *
 * Validates the vector.
 */
gboolean
graphene_ext_vec3_validate (const graphene_vec3_t *v);

/**
 * graphene_ext_quaternion_near:
 * @a: The first quaternion.
 * @b: The second quaternion.
 * @epsilon: The tolerance.
 * @returns: `TRUE` if the quaternions are near each other, `FALSE` otherwise.
 *
 * Checks if two quaternions are near each other.
 */
bool
graphene_ext_quaternion_near (const graphene_quaternion_t *a,
                              const graphene_quaternion_t *b,
                              float                        epsilon);

/**
 * graphene_ext_point3d_near:
 * @a: The first point.
 * @b: The second point.
 * @epsilon: The tolerance.
 * @returns: `TRUE` if the points are near each other, `FALSE` otherwise.
 *
 * Checks if two points are near each other.
 */
bool
graphene_ext_point3d_near (const graphene_point3d_t *a,
                           const graphene_point3d_t *b,
                           float                     epsilon);

#endif /* XRD_GRAPHENE_QUATERNION_H_ */
