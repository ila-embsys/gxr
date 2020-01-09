/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include <glib-unix.h>

#include <gxr.h>

typedef struct Example
{
  GSList *controllers;
  GxrOverlayModel *model_overlay;
  GMainLoop *loop;
  guint current_model_list_index;
  GSList *models;
  GxrActionSet *action_set;
} Example;

static gboolean
_sigint_cb (gpointer _self)
{
  Example *self = (Example*) _self;
  g_main_loop_quit (self->loop);
  return TRUE;
}

static void
_pose_cb (GxrAction    *action,
          GxrPoseEvent *event,
          Example      *self)
{
  (void) action;

  graphene_point3d_t translation_point;

  graphene_point3d_init (&translation_point, .0f, .1f, -.1f);

  graphene_matrix_t transformation_matrix;
  graphene_matrix_init_translate (&transformation_matrix, &translation_point);

  graphene_matrix_scale (&transformation_matrix, 1.0f, 1.0f, 0.5f);

  graphene_matrix_t transformed;
  graphene_matrix_multiply (&transformation_matrix,
                            &event->pose,
                            &transformed);

  GxrOverlay *overlay = gxr_overlay_model_get_overlay (self->model_overlay);
  gxr_overlay_set_transform_absolute (overlay, &transformed);

  free (event);
}

static gboolean
_update_model (Example *self)
{
  graphene_vec4_t color;
  graphene_vec4_init (&color, 1., 1., 1., 1.);

  GSList* name = g_slist_nth (self->models, self->current_model_list_index);
  g_print ("Setting Model '%s' [%d/%d]\n",
           (gchar *) name->data,
           self->current_model_list_index + 1,
           g_slist_length (self->models));

  if (!gxr_overlay_model_set_model (self->model_overlay,
                                    (gchar *) name->data, &color))
    return FALSE;

  return TRUE;
}

static gboolean
_next_model (Example *self)
{
  if (self->current_model_list_index == g_slist_length (self->models) - 1)
    self->current_model_list_index = 0;
  else
    self->current_model_list_index++;

  if (!_update_model (self))
    return FALSE;

  return TRUE;
}

static gboolean
_previous_model (Example *self)
{
  if (self->current_model_list_index == 0)
    self->current_model_list_index = g_slist_length (self->models) - 1;
  else
    self->current_model_list_index--;

  if (!_update_model (self))
    return FALSE;

  return TRUE;
}

static void
_next_cb (GxrAction       *action,
          GxrDigitalEvent *event,
          Example         *self)
{
  (void) action;
  (void) event;

  if (event->active && event->changed && event->state)
    _next_model (self);
}

static void
_previous_cb (GxrAction       *action,
              GxrDigitalEvent *event,
              Example         *self)
{
  (void) action;
  (void) event;

  if (event->active && event->changed && event->state)
    _previous_model (self);
}

static gboolean
_poll_events_cb (gpointer _self)
{
  Example *self = (Example*) _self;

  if (!gxr_action_sets_poll (&self->action_set, 1))
    return FALSE;

  return TRUE;
}

static gboolean
_init_model_overlay (Example *self)
{
  self->model_overlay = gxr_overlay_model_new ("model", "A 3D model overlay");

  graphene_vec4_t color;
  graphene_vec4_init (&color, 1., 1., 1., 1.);

  GSList* model_name = g_slist_nth (self->models,
                                    self->current_model_list_index);
  if (!gxr_overlay_model_set_model (self->model_overlay,
                               (gchar *) model_name->data, &color))
    return FALSE;

  char name_ret[GXR_MODEL_NAME_MAX];
  graphene_vec4_t color_ret;

  uint32_t id;
  if (!gxr_overlay_model_get_model (self->model_overlay, name_ret,
                                   &color_ret, &id))
    return FALSE;

  g_print ("GetOverlayRenderModel returned id %d name: %s\n", id, name_ret);

    GxrOverlay *overlay = gxr_overlay_model_get_overlay (self->model_overlay);
  if (!gxr_overlay_set_width_meters (overlay, 0.5f))
    return FALSE;

  if (!gxr_overlay_show (overlay))
    return FALSE;

  return TRUE;
}

static void
_print_model (gpointer name, gpointer unused)
{
  (void) unused;
  g_print ("Model: %s\n", (gchar*) name);
}

static void
_cleanup (Example *self)
{
  g_print ("bye\n");
  g_main_loop_unref (self->loop);

  g_object_unref (self->model_overlay);
  g_slist_free_full (self->models, g_free);

  GxrContext *context = gxr_context_get_instance ();
  g_object_unref (context);
}

int
main ()
{
  GxrContext *context = gxr_context_get_instance ();
  // TODO: init gulkan for scene, OpenXR backend
  if (!gxr_context_init_runtime (context, GXR_APP_OVERLAY))
    {
      g_printerr ("Could not init VR runtime.\n");
      return false;
    }

  if (!gxr_action_load_manifest (
        context,
        "gxr",
        "/res/bindings",
        "example_model_actions.json",
        "example_model_bindings_vive_controller.json",
        NULL))
    return -1;

  Example self = {
    .loop = g_main_loop_new (NULL, FALSE),
    .current_model_list_index = 0,
    .models = NULL,
    .action_set = gxr_action_set_new_from_url (context, "/actions/model")
  };

  gxr_action_set_connect (self.action_set, GXR_ACTION_DIGITAL,
                          "/actions/model/in/next",
                          (GCallback) _next_cb, &self);

  gxr_action_set_connect (self.action_set, GXR_ACTION_DIGITAL,
                          "/actions/model/in/previous",
                          (GCallback) _previous_cb, &self);

  gxr_action_set_connect (self.action_set, GXR_ACTION_POSE,
                          "/actions/model/in/hand_primary",
                          (GCallback) _pose_cb, &self);

  self.models = gxr_context_get_model_list (context);
  g_slist_foreach (self.models, _print_model, NULL);

  if (!_init_model_overlay (&self))
    return -1;

  g_timeout_add (20, _poll_events_cb, &self);

  g_unix_signal_add (SIGINT, _sigint_cb, &self);

  /* start glib main loop */
  g_main_loop_run (self.loop);

  _cleanup (&self);

  return 0;
}
