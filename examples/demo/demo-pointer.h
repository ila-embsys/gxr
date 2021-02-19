/*
 * xrdesktop
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef DEMO_POINTER_H_
#define DEMO_POINTER_H_

#include <glib-object.h>
#include <graphene.h>

G_BEGIN_DECLS

#define DEMO_TYPE_POINTER demo_pointer_get_type()
G_DECLARE_INTERFACE (DemoPointer, demo_pointer, DEMO, POINTER, GObject)

typedef struct {
  float start_offset;
  float length;
  float default_length;
  gboolean visible;
  gboolean render_ray;
} DemoPointerData;

struct _DemoPointerInterface
{
  GTypeInterface parent;

  void
  (*move) (DemoPointer        *self,
           graphene_matrix_t *transform);

  void
  (*set_length) (DemoPointer *self,
                 float       length);

  DemoPointerData*
  (*get_data) (DemoPointer *self);

  void
  (*set_transformation) (DemoPointer        *self,
                         graphene_matrix_t *matrix);

  void
  (*get_transformation) (DemoPointer        *self,
                         graphene_matrix_t *matrix);

  void
  (*set_selected_object) (DemoPointer *pointer,
                          gpointer    object);

  void
  (*show) (DemoPointer *self);

  void
  (*hide) (DemoPointer *self);
};

void
demo_pointer_move (DemoPointer *self,
                  graphene_matrix_t *transform);

void
demo_pointer_set_length (DemoPointer *self,
                        float       length);

float
demo_pointer_get_default_length (DemoPointer *self);

void
demo_pointer_reset_length (DemoPointer *self);

DemoPointerData*
demo_pointer_get_data (DemoPointer *self);

void
demo_pointer_init (DemoPointer *self);

void
demo_pointer_set_transformation (DemoPointer        *self,
                                graphene_matrix_t *matrix);

void
demo_pointer_get_transformation (DemoPointer        *self,
                                graphene_matrix_t *matrix);

void
demo_pointer_get_ray (DemoPointer     *self,
                     graphene_ray_t *res);

gboolean
demo_pointer_get_plane_intersection (DemoPointer        *self,
                                    graphene_plane_t  *plane,
                                    graphene_matrix_t *plane_transform,
                                    float              plane_aspect,
                                    float             *distance,
                                    graphene_vec3_t   *res);

void
demo_pointer_show (DemoPointer *self);

void
demo_pointer_hide (DemoPointer *self);

gboolean
demo_pointer_is_visible (DemoPointer *self);

void
demo_pointer_update_render_ray (DemoPointer *self, gboolean render_ray);

G_END_DECLS

#endif /* DEMO_POINTER_H_ */
