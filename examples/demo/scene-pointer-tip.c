/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "scene-pointer-tip.h"

#include "scene-object.h"

#include <stdalign.h>
#include <gdk/gdk.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
  alignas(16) float mvp[16];
  alignas(16) float mv[16];
  alignas(16) float m[16];
  alignas(4) bool receive_light;
} ScenePointerTipUniformBuffer;

typedef struct {
  alignas(16) float color[4];
  alignas(4) bool flip_y;
} WindowUniformBuffer;

typedef struct {
  ScenePointerTip *tip;
  float progress;
  guint callback_id;
} ScenePointerTipAnimation;

typedef struct {
  gboolean keep_apparent_size;
  float width_meters;

  graphene_point3d_t active_color;
  graphene_point3d_t passive_color;

  double pulse_alpha;

  int texture_width;
  int texture_height;
} ScenePointerTipSettings;

struct _ScenePointerTip
{
  GObject parent;

  GulkanClient *gulkan;
  VkBuffer lights;

  GulkanVertexBuffer *vertex_buffer;
  VkSampler sampler;
  float aspect_ratio;

  gboolean flip_y;
  graphene_vec3_t color;

  /* separate ubo used in fragment shader */
  GulkanUniformBuffer *shading_buffer;
  WindowUniformBuffer shading_buffer_data;

  GulkanTexture *texture;
  uint32_t texture_width;
  uint32_t texture_height;

  float initial_width_meter;
  float initial_height_meter;
  float scale;

  gboolean active;

  VkImageLayout upload_layout;

  ScenePointerTipSettings settings;

  /* Pointer to the data of the currently running animation.
   * Must be freed when an animation callback is cancelled. */
  ScenePointerTipAnimation *animation;
};

G_DEFINE_TYPE (ScenePointerTip, scene_pointer_tip, SCENE_TYPE_OBJECT)

static void
scene_pointer_tip_finalize (GObject *gobject);

static void
scene_pointer_tip_class_init (ScenePointerTipClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = scene_pointer_tip_finalize;
}

static void
scene_pointer_tip_init (ScenePointerTip *self)
{
  self->active = FALSE;
  self->animation = NULL;
  self->settings.width_meters = 1.0f;
  self->upload_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  self->vertex_buffer = gulkan_vertex_buffer_new ();
  self->sampler = VK_NULL_HANDLE;
  self->aspect_ratio = 1.0;
  self->shading_buffer_data.flip_y = FALSE;

  self->texture = NULL;
  self->texture_width = 0;
  self->texture_height = 0;
  self->initial_height_meter = 0;
  self->initial_width_meter = 0;
}

static void
_set_color (ScenePointerTip       *self,
            const graphene_vec3_t *color)
{
  graphene_vec3_init_from_vec3 (&self->color, color);

  graphene_vec4_t color_vec4;
  graphene_vec4_init_from_vec3 (&color_vec4, color, 1.0f);

  float color_arr[4];
  graphene_vec4_to_float (&color_vec4, color_arr);
  for (int i = 0; i < 4; i++)
    self->shading_buffer_data.color[i] = color_arr[i];

  gulkan_uniform_buffer_update (self->shading_buffer,
                                (gpointer) &self->shading_buffer_data);
}

static void
_append_plane (GulkanVertexBuffer *vbo, float aspect_ratio)
{
  graphene_matrix_t mat_scale;
  graphene_matrix_init_scale (&mat_scale, aspect_ratio, 1.0f, 1.0f);

  graphene_point_t from = { .x = -0.5, .y = -0.5 };
  graphene_point_t to = { .x = 0.5, .y = 0.5 };

  gulkan_geometry_append_plane (vbo, &from, &to, &mat_scale);
}

static gboolean
_initialize (ScenePointerTip* self, VkDescriptorSetLayout *layout)
{
  SceneObject *obj = SCENE_OBJECT (self);

  GulkanDevice *device = gulkan_client_get_device (self->gulkan);

  _append_plane (self->vertex_buffer, self->aspect_ratio);
  if (!gulkan_vertex_buffer_alloc_array (self->vertex_buffer, device))
    return FALSE;

  VkDeviceSize ubo_size = sizeof (ScenePointerTipUniformBuffer);
  if (!scene_object_initialize (obj, self->gulkan, layout, ubo_size))
    return FALSE;

  self->shading_buffer =
    gulkan_uniform_buffer_new (device, sizeof (WindowUniformBuffer));
  if (!self->shading_buffer)
    return FALSE;

  graphene_vec3_t white;
  graphene_vec3_init (&white, 1.0f, 1.0f, 1.0f);
  _set_color (self, &white);

  return TRUE;
}

void
scene_pointer_tip_set_width_meters (ScenePointerTip *self,
                                    float            width_meters)
{
  float height_meters = width_meters / self->aspect_ratio;

  self->initial_width_meter = width_meters;
  self->initial_height_meter = height_meters;
  self->scale = 1.0;

  scene_object_set_scale (SCENE_OBJECT (self), height_meters);
}

ScenePointerTip *
scene_pointer_tip_new (GulkanClient          *gulkan,
                       VkDescriptorSetLayout *layout,
                       VkBuffer               lights)
{
  ScenePointerTip* self =
    (ScenePointerTip*) g_object_new (SCENE_TYPE_POINTER_TIP, 0);

  self->gulkan = g_object_ref (gulkan);
  self->lights = lights;

  self->texture_width = 64;
  self->texture_height = 64;

  _initialize (self, layout);

  scene_pointer_tip_init_settings (SCENE_POINTER_TIP (self));

  return self;
}

static void
scene_pointer_tip_finalize (GObject *gobject)
{
  ScenePointerTip *self = SCENE_POINTER_TIP (gobject);

  /* cancels potentially running animation */
  scene_pointer_tip_set_active (SCENE_POINTER_TIP (self), FALSE);

  VkDevice device = gulkan_client_get_device_handle (self->gulkan);
  vkDestroySampler (device, self->sampler, NULL);

  g_object_unref (self->vertex_buffer);
  g_object_unref (self->shading_buffer);

  g_object_unref (self->texture);

  g_object_unref (self->gulkan);

  G_OBJECT_CLASS (scene_pointer_tip_parent_class)->finalize (gobject);
}


static void
_update_descriptors (ScenePointerTip *self)
{
  VkDevice device = gulkan_client_get_device_handle (self->gulkan);

  for (uint32_t eye = 0; eye < 2; eye++)
  {
    VkBuffer transformation_buffer =
    scene_object_get_transformation_buffer (SCENE_OBJECT (self),
                                                eye);

    VkDescriptorSet descriptor_set =
    scene_object_get_descriptor_set (SCENE_OBJECT (self), eye);

    VkWriteDescriptorSet *write_descriptor_sets = (VkWriteDescriptorSet []) {
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_set,
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &(VkDescriptorBufferInfo) {
          .buffer = transformation_buffer,
          .offset = 0,
          .range = VK_WHOLE_SIZE
        },
        .pTexelBufferView = NULL
      },
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_set,
        .dstBinding = 1,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &(VkDescriptorImageInfo) {
          .sampler = self->sampler,
          .imageView = gulkan_texture_get_image_view (self->texture),
          .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        },
        .pBufferInfo = NULL,
        .pTexelBufferView = NULL
      },
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_set,
        .dstBinding = 2,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &(VkDescriptorBufferInfo) {
          .buffer = gulkan_uniform_buffer_get_handle (self->shading_buffer),
          .offset = 0,
          .range = VK_WHOLE_SIZE
        },
        .pTexelBufferView = NULL
      },
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_set,
        .dstBinding = 3,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &(VkDescriptorBufferInfo) {
          .buffer = self->lights,
          .offset = 0,
          .range = VK_WHOLE_SIZE
        },
        .pTexelBufferView = NULL
      }
    };

    vkUpdateDescriptorSets (device, 4, write_descriptor_sets, 0, NULL);
  }
}

void
scene_pointer_tip_set_and_submit_texture (ScenePointerTip *tip,
                                          GulkanTexture *texture)
{
  ScenePointerTip *self = SCENE_POINTER_TIP (tip);
  if (texture == self->texture)
    {
      g_debug ("Texture %p was already set on tip %p.",
                (void*) texture, (void*) self);
      return;
    }


  uint32_t previous_texture_width = self->texture_width;
  uint32_t previous_Texture_height = self->texture_height;

  VkExtent2D extent = gulkan_texture_get_extent (texture);
  self->texture_width = extent.width;
  self->texture_height = extent.height;

  VkDevice device = gulkan_client_get_device_handle (self->gulkan);

  float aspect_ratio = (float) extent.width / (float) extent.height;

  if (self->aspect_ratio != aspect_ratio)
    {
      self->aspect_ratio = aspect_ratio;
      gulkan_vertex_buffer_reset (self->vertex_buffer);
      _append_plane (self->vertex_buffer, self->aspect_ratio);
      gulkan_vertex_buffer_map_array (self->vertex_buffer);
    }

  /* self->texture == texture must be handled above */
  if (self->texture)
    g_object_unref (self->texture);

  self->texture = texture;
  guint mip_levels = gulkan_texture_get_mip_levels (texture);

  VkSamplerCreateInfo sampler_info = {
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .magFilter = VK_FILTER_LINEAR,
    .minFilter = VK_FILTER_LINEAR,
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .anisotropyEnable = VK_TRUE,
    .maxAnisotropy = 16.0f,
    .minLod = 0.0f,
    .maxLod = (float) mip_levels
  };

  if (self->sampler != VK_NULL_HANDLE)
    vkDestroySampler (device, self->sampler, NULL);

  vkCreateSampler (device, &sampler_info, NULL, &self->sampler);

  _update_descriptors (self);

  if (previous_texture_width != extent.width ||
    previous_Texture_height != extent.height)
  {
    /* initial-dims are respective the texture size and ppm.
     * Now that the texture size changed, initial dims need to be
     * updated, using the original ppm used to create this window. */
    float initial_width_meter = self->initial_width_meter;
    /* float initial_height_meter = self->initial_height_meter; */

    float previous_ppm = (float)previous_texture_width / initial_width_meter;
    float new_initial_width_meter = (float) extent.width / previous_ppm;

    /* updates "initial-width-meters"  and "initial height-meters"! */
    scene_pointer_tip_set_width_meters (self, new_initial_width_meter);
  }

}

static void
_update_ubo (ScenePointerTip   *self,
             GxrEye             eye,
             graphene_matrix_t *vp)
{
  ScenePointerTipUniformBuffer ub  = {0};

  ub.receive_light = FALSE;

  graphene_matrix_t m_matrix;
  scene_object_get_transformation (SCENE_OBJECT (self), &m_matrix);

  graphene_matrix_t mvp_matrix;
  graphene_matrix_multiply (&m_matrix, vp, &mvp_matrix);

  float mvp[16];
  graphene_matrix_to_float (&mvp_matrix, mvp);
  for (int i = 0; i < 16; i++)
    ub.mvp[i] = mvp[i];

  scene_object_update_ubo (SCENE_OBJECT (self), eye, &ub);
}

void
scene_pointer_tip_render (ScenePointerTip   *self,
                          GxrEye             eye,
                          VkPipeline         pipeline,
                          VkPipelineLayout   pipeline_layout,
                          VkCommandBuffer    cmd_buffer,
                          graphene_matrix_t *vp)
{
  if (!self->texture)
  {
    /* g_warning ("Trying to draw window with no texture.\n"); */
    return;
  }

  SceneObject *obj = SCENE_OBJECT (self);
  if (!scene_object_is_visible (obj))
    return;

  vkCmdBindPipeline (cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  _update_ubo (self, eye, vp);

  scene_object_bind (obj, eye, cmd_buffer, pipeline_layout);
  gulkan_vertex_buffer_draw (self->vertex_buffer, cmd_buffer);
}



/* TODO */
static void
gxr_math_matrix_set_translation_point (graphene_matrix_t  *matrix,
                                       graphene_point3d_t *point)
{
  float m[16];
  graphene_matrix_to_float (matrix, m);

  m[12] = point->x;
  m[13] = point->y;
  m[14] = point->z;
  graphene_matrix_init_from_float (matrix, m);
}
static void
graphene_ext_matrix_get_translation_point3d (const graphene_matrix_t *m,
                                             graphene_point3d_t      *res)
{
  float f[16];
  graphene_matrix_to_float (m, f);
  graphene_point3d_init (res, f[12], f[13], f[14]);
}
/* TODO */


void
scene_pointer_tip_update (ScenePointerTip      *self,
                        GxrContext         *context,
                        graphene_matrix_t  *pose,
                        graphene_point3d_t *intersection_point)
{
  graphene_matrix_t transform;
  graphene_matrix_init_from_matrix (&transform, pose);
  gxr_math_matrix_set_translation_point (&transform, intersection_point);
  scene_object_set_transformation (SCENE_OBJECT (self), &transform);

  scene_pointer_tip_update_apparent_size (self, context);
}

GulkanTexture *
scene_pointer_tip_get_texture (ScenePointerTip *self)
{
  return self->texture;
}

GulkanClient*
scene_pointer_tip_get_gulkan_client (ScenePointerTip *self)
{
  return self->gulkan;
}

static void
_update_texture (ScenePointerTip *self);

static gboolean
_cancel_animation (ScenePointerTip *self)
{
  if (self->animation != NULL)
    {
      g_source_remove (self->animation->callback_id);
      g_free (self->animation);
      self->animation = NULL;
      return TRUE;
    }
  else
    return FALSE;
}

static void
_init_texture (ScenePointerTip *self)
{
  GulkanClient *client = scene_pointer_tip_get_gulkan_client (self);

  GdkPixbuf* pixbuf = scene_pointer_tip_update_pixbuf (self, 1.0f);

  GulkanTexture *texture =
    gulkan_texture_new_from_pixbuf (client, pixbuf,
                                    VK_FORMAT_R8G8B8A8_SRGB,
                                    self->upload_layout,
                                    false);
  g_object_unref (pixbuf);

  scene_pointer_tip_set_and_submit_texture (self, texture);
}

void
scene_pointer_tip_init_settings (ScenePointerTip *self)
{
  g_debug ("Initializing pointer tip!");

  ScenePointerTipSettings *s = &self->settings;
  s->keep_apparent_size = TRUE;
  s->width_meters = 0.05f * GXR_TIP_VIEWPORT_SCALE;
  scene_pointer_tip_set_width_meters (self, s->width_meters);
  graphene_point3d_init (&s->active_color, 0.078f, 0.471f, 0.675f);
  graphene_point3d_init (&s->passive_color, 1.0f, 1.0f, 1.0f);
  s->texture_width = 64;
  s->texture_height = 64;
  s->pulse_alpha = 0.25;
  _init_texture (self);
  _update_texture (self);
}

/* draws a circle in the center of a cairo surface of dimensions WIDTHxHEIGHT.
 * scale affects the radius of the circle and should be in [0,2].
 * a_in is the alpha value at the center, a_out at the outer border. */
static void
_draw_gradient_circle (cairo_t              *cr,
                       int                   w,
                       int                   h,
                       double                radius,
                       graphene_point3d_t   *color,
                       double                a_in,
                       double                a_out)
{
  double center_x = w / 2;
  double center_y = h / 2;

  cairo_pattern_t *pat = cairo_pattern_create_radial (center_x, center_y,
                                                      0.75 * radius,
                                                      center_x, center_y,
                                                      radius);
  cairo_pattern_add_color_stop_rgba (pat, 0,
                                     (double) color->x,
                                     (double) color->y,
                                     (double) color->z, a_in);

  cairo_pattern_add_color_stop_rgba (pat, 1,
                                     (double) color->x,
                                     (double) color->y,
                                     (double) color->z, a_out);
  cairo_set_source (cr, pat);
  cairo_arc (cr, center_x, center_y, radius, 0, 2 * M_PI);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);
}

static GdkPixbuf*
_render_cairo (int                 w,
               int                 h,
               double              radius,
               graphene_point3d_t *color,
               double              pulse_alpha,
               float               progress)
{
  cairo_surface_t *surface =
      cairo_image_surface_create (CAIRO_FORMAT_ARGB32, w, h);

  cairo_t *cr = cairo_create (surface);
  cairo_set_source_rgba (cr, 0, 0, 0, 0);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint (cr);

  /* Draw pulse */
  if (progress != 1.0f)
    {
      float pulse_scale = GXR_TIP_VIEWPORT_SCALE * (1.0f - progress);
      graphene_point3d_t white = { 1.0f, 1.0f, 1.0f };
      _draw_gradient_circle (cr, w, h, radius * (double) pulse_scale, &white,
                             pulse_alpha, 0.0);
    }

  cairo_set_operator (cr, CAIRO_OPERATOR_MULTIPLY);

  /* Draw tip */
  _draw_gradient_circle (cr, w, h, radius, color, 1.0, 0.0);

  cairo_destroy (cr);

  /* Since we cannot set a different format for raw upload,
   * we need to use GdkPixbuf to suit OpenVRs needs */
  GdkPixbuf *pixbuf = gdk_pixbuf_get_from_surface (surface, 0, 0, w, h);

  cairo_surface_destroy (surface);

  return pixbuf;
}

/** _render:
 * Renders the pointer tip with the desired colors.
 * If background scale is > 1, a transparent white background circle is rendered
 * behind the pointer tip. */
GdkPixbuf*
scene_pointer_tip_update_pixbuf (ScenePointerTip *self,
                                 float          progress)
{
  int w = self->settings.texture_width * GXR_TIP_VIEWPORT_SCALE;
  int h = self->settings.texture_height * GXR_TIP_VIEWPORT_SCALE;

  graphene_point3d_t *color = self->active ? &self->settings.active_color :
                                             &self->settings.passive_color;

  double radius = self->settings.texture_width / 2.0;

  GdkPixbuf *pixbuf =
    _render_cairo (w, h, radius, color, self->settings.pulse_alpha, progress);

  return pixbuf;
}

static gboolean
_animate_cb (gpointer _animation)
{
  ScenePointerTipAnimation *animation = (ScenePointerTipAnimation *) _animation;
  ScenePointerTip *self = animation->tip;

  GulkanTexture *texture = scene_pointer_tip_get_texture (self);

  GdkPixbuf* pixbuf = scene_pointer_tip_update_pixbuf (self, animation->progress);

  gulkan_texture_upload_pixbuf (texture, pixbuf, self->upload_layout);

  g_object_unref (pixbuf);

  animation->progress += 0.05f;

  if (animation->progress > 1)
    {
      self->animation = NULL;
      g_free (animation);
      return FALSE;
    }

  return TRUE;
}

void
scene_pointer_tip_animate_pulse (ScenePointerTip *self)
{
  if (self->animation != NULL)
    scene_pointer_tip_set_active (self, self->active);

  self->animation = g_malloc (sizeof (ScenePointerTipAnimation));
  self->animation->progress = 0;
  self->animation->tip = self;
  self->animation->callback_id = g_timeout_add (20, _animate_cb,
                                                self->animation);
}

static void
_update_texture (ScenePointerTip *self)
{
  GdkPixbuf* pixbuf = scene_pointer_tip_update_pixbuf (self, 1.0f);
  GulkanTexture *texture = scene_pointer_tip_get_texture (self);

  gulkan_texture_upload_pixbuf (texture, pixbuf, self->upload_layout);
  g_object_unref (pixbuf);
}

/**
 * scene_pointer_tip_set_active:
 * @self: a #ScenePointerTip
 * @active: whether to use the active or inactive style
 *
 * Changes whether the active or inactive style is rendered.
 * Also cancels animations.
 */
void
scene_pointer_tip_set_active (ScenePointerTip *self,
                              gboolean       active)
{
  if (scene_pointer_tip_get_texture (self) == NULL)
    return;

  /* New texture needs to be rendered when
   *  - animation is being cancelled
   *  - active status changes
   * Otherwise the texture should already show the current active status. */

  gboolean animation_cancelled = _cancel_animation (self);
  if (!animation_cancelled && self->active == active)
    return;

  self->active = active;

  _update_texture (self);
}

/**
 * scene_pointer_tip_update_apparent_size:
 * @self: a #ScenePointerTip
 * @context: a #GxrContext
 *
 * Note: Move pointer tip to the desired location before calling.
 */
void
scene_pointer_tip_update_apparent_size (ScenePointerTip *self,
                                      GxrContext    *context)
{
  if (!self->settings.keep_apparent_size)
    return;

  graphene_matrix_t tip_pose;
  scene_object_get_transformation (SCENE_OBJECT (self), &tip_pose);

  graphene_point3d_t tip_point;
  graphene_ext_matrix_get_translation_point3d (&tip_pose, &tip_point);

  graphene_matrix_t hmd_pose;
  gboolean has_pose = gxr_context_get_head_pose (context, &hmd_pose);
  if (!has_pose)
    {
      scene_pointer_tip_set_width_meters (self, self->settings.width_meters);
      return;
    }

  graphene_point3d_t hmd_point;
  graphene_ext_matrix_get_translation_point3d (&hmd_pose, &hmd_point);

  float distance = graphene_point3d_distance (&tip_point, &hmd_point, NULL);

  float w = self->settings.width_meters
            / GXR_TIP_APPARENT_SIZE_DISTANCE * distance;

  scene_pointer_tip_set_width_meters (self, w);
}

void
scene_pointer_tip_update_texture_resolution (ScenePointerTip *self,
                                           int            width,
                                           int            height)
{
  ScenePointerTipSettings *s = &self->settings;
  s->texture_width = width;
  s->texture_height = height;

  _init_texture (self);
}

void
scene_pointer_tip_update_color (ScenePointerTip      *self,
                                gboolean            active_color,
                                graphene_point3d_t *color)

{
  ScenePointerTipSettings *s = &self->settings;

  if (active_color)
    graphene_point3d_init_from_point (&s->active_color, color);
  else
    graphene_point3d_init_from_point (&s->passive_color, color);

  if ((!self->active && !active_color) || (self->active && active_color))
    {
      _cancel_animation (self);
      _update_texture (self);
    }
}

void
scene_pointer_tip_update_pulse_alpha (ScenePointerTip *self,
                                      double         alpha)
{
  self->settings.pulse_alpha = alpha;
}

void
scene_pointer_tip_update_keep_apparent_size (ScenePointerTip *self,
                                             gboolean       keep_apparent_size)
{
  self->settings.keep_apparent_size = keep_apparent_size;
  scene_pointer_tip_set_width_meters (self, self->settings.width_meters);
}

void
scene_pointer_tip_update_width_meters (ScenePointerTip *self,
                                     float          width)
{
  self->settings.width_meters = width * GXR_TIP_VIEWPORT_SCALE;
  scene_pointer_tip_set_width_meters (self, self->settings.width_meters);
}
