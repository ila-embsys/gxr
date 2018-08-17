#pragma once

#include "cairo_content.h"

static gboolean
draw_content (ClutterCanvas *canvas,
              cairo_t       *cr,
              int            width,
              int            height)
{
  (void) canvas;
  draw_rounded_quad (cr, width, height);
  return TRUE;
}

void
create_rotated_quad_stage (ClutterActor* stage)
{
  ClutterActor *actor;
  ClutterContent *canvas;
  ClutterTransition *transition;

  /* create a stage */
  clutter_stage_set_title (CLUTTER_STAGE (stage),
                           "Rectangle with rounded corners");
  clutter_stage_set_use_alpha (CLUTTER_STAGE (stage), TRUE);
  clutter_actor_set_background_color (stage, CLUTTER_COLOR_Black);
  clutter_actor_set_size (stage, 500, 500);
  clutter_actor_set_opacity (stage, 64);

  /* our 2D canvas, courtesy of Cairo */
  canvas = clutter_canvas_new ();
  clutter_canvas_set_size (CLUTTER_CANVAS (canvas), 300, 300);

  /* the actor that will display the contents of the canvas */
  actor = clutter_actor_new ();
  clutter_actor_set_content (actor, canvas);
  clutter_actor_set_content_gravity (actor, CLUTTER_CONTENT_GRAVITY_CENTER);
  clutter_actor_set_content_scaling_filters (actor,
                                             CLUTTER_SCALING_FILTER_TRILINEAR,
                                             CLUTTER_SCALING_FILTER_LINEAR);
  clutter_actor_set_pivot_point (actor, 0.5f, 0.5f);
  ClutterConstraint *constraint =
    clutter_align_constraint_new (stage, CLUTTER_ALIGN_BOTH, 0.5);
  clutter_actor_add_constraint (actor, constraint);
  clutter_actor_set_request_mode (actor, CLUTTER_REQUEST_CONTENT_SIZE);
  clutter_actor_add_child (stage, actor);

  /* the actor now owns the canvas */
  g_object_unref (canvas);

  /* create the continuous animation of the actor spinning around its center */
  transition = clutter_property_transition_new ("rotation-angle-y");
  clutter_transition_set_from (transition, G_TYPE_DOUBLE, 0.0);
  clutter_transition_set_to (transition, G_TYPE_DOUBLE, 360.0);
  clutter_timeline_set_duration (CLUTTER_TIMELINE (transition), 2000);
  clutter_timeline_set_repeat_count (CLUTTER_TIMELINE (transition), -1);
  clutter_actor_add_transition (actor, "rotateActor", transition);

  /* the actor now owns the transition */
  g_object_unref (transition);

  /* connect our drawing code */
  g_signal_connect (canvas, "draw", G_CALLBACK (draw_content), NULL);

  // g_signal_connect (actor, "paint", G_CALLBACK (enter_cb), NULL);
  //g_signal_connect (stage, "paint", G_CALLBACK (enter_cb), NULL);

  /* invalidate the canvas, so that we can draw before the main loop starts */
  clutter_content_invalidate (canvas);
}
