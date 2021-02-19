
/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef DEMO_POINTER_TIP_H_
#define DEMO_POINTER_TIP_H_

#include <glib-object.h>
#include <graphene.h>
#include <gulkan.h>
#include <gxr.h>

G_BEGIN_DECLS

#define DEMO_TYPE_POINTER_TIP demo_pointer_tip_get_type()
G_DECLARE_INTERFACE (DemoPointerTip, demo_pointer_tip, DEMO, POINTER_TIP, GObject)

/*
 * Since the pulse animation surrounds the tip and would
 * exceed the canvas size, we need to scale it to fit the pulse.
 */
#define GXR_TIP_VIEWPORT_SCALE 3

/*
 * The distance in meters for which apparent size and regular size are equal.
 */
#define GXR_TIP_APPARENT_SIZE_DISTANCE 3.0f

typedef struct {
  DemoPointerTip *tip;
  float progress;
  guint callback_id;
} DemoPointerTipAnimation;

typedef struct {
  gboolean keep_apparent_size;
  float width_meters;

  graphene_point3d_t active_color;
  graphene_point3d_t passive_color;

  double pulse_alpha;

  int texture_width;
  int texture_height;
} DemoPointerTipSettings;

typedef struct {
  DemoPointerTip *tip;

  gboolean active;

  VkImageLayout upload_layout;

  DemoPointerTipSettings settings;

  /* Pointer to the data of the currently running animation.
   * Must be freed when an animation callback is cancelled. */
  DemoPointerTipAnimation *animation;
} DemoPointerTipData;

struct _DemoPointerTipInterface
{
  GTypeInterface parent;

  void
  (*set_transformation) (DemoPointerTip     *self,
                         graphene_matrix_t *matrix);

  void
  (*get_transformation) (DemoPointerTip     *self,
                         graphene_matrix_t *matrix);

  void
  (*show) (DemoPointerTip *self);

  void
  (*hide) (DemoPointerTip *self);

  gboolean
  (*is_visible) (DemoPointerTip *self);

  void
  (*set_width_meters) (DemoPointerTip *self,
                       float          meters);

  void
  (*submit_texture) (DemoPointerTip *self);

  GulkanTexture *
  (*get_texture) (DemoPointerTip *self);

  void
  (*set_and_submit_texture) (DemoPointerTip *self,
                             GulkanTexture *texture);

  DemoPointerTipData*
  (*get_data) (DemoPointerTip *self);

  GulkanClient*
  (*get_gulkan_client) (DemoPointerTip *self);
};

void
demo_pointer_tip_update_apparent_size (DemoPointerTip *self,
                                      GxrContext    *context);

void
demo_pointer_tip_update (DemoPointerTip      *self,
                        GxrContext         *context,
                        graphene_matrix_t  *pose,
                        graphene_point3d_t *intersection_point);

void
demo_pointer_tip_set_active (DemoPointerTip *self,
                            gboolean       active);

void
demo_pointer_tip_animate_pulse (DemoPointerTip *self);

void
demo_pointer_tip_set_transformation (DemoPointerTip     *self,
                                    graphene_matrix_t *matrix);

void
demo_pointer_tip_get_transformation (DemoPointerTip     *self,
                                    graphene_matrix_t *matrix);

void
demo_pointer_tip_show (DemoPointerTip *self);

void
demo_pointer_tip_hide (DemoPointerTip *self);

gboolean
demo_pointer_tip_is_visible (DemoPointerTip *self);

void
demo_pointer_tip_set_width_meters (DemoPointerTip *self,
                                  float          meters);

void
demo_pointer_tip_submit_texture (DemoPointerTip *self);

void
demo_pointer_tip_set_and_submit_texture (DemoPointerTip *self,
                                        GulkanTexture *texture);

GulkanTexture *
demo_pointer_tip_get_texture (DemoPointerTip *self);

void
demo_pointer_tip_init_settings (DemoPointerTip     *self,
                               DemoPointerTipData *data);

GdkPixbuf*
demo_pointer_tip_render (DemoPointerTip *self,
                        float          progress);

DemoPointerTipData*
demo_pointer_tip_get_data (DemoPointerTip *self);

GulkanClient*
demo_pointer_tip_get_gulkan_client (DemoPointerTip *self);


void
demo_pointer_tip_update_texture_resolution (DemoPointerTip *self,
                                           int            width,
                                           int            height);

void
demo_pointer_tip_update_color (DemoPointerTip      *self,
                              gboolean            active_color,
                              graphene_point3d_t *color);

void
demo_pointer_tip_update_pulse_alpha (DemoPointerTip *self,
                                    double         alpha);

void
demo_pointer_tip_update_keep_apparent_size (DemoPointerTip *self,
                                           gboolean       keep_apparent_size);
void
demo_pointer_tip_update_width_meters (DemoPointerTip *self,
                                     float          width);

G_END_DECLS

#endif /* DEMO_POINTER_TIP_H_ */
