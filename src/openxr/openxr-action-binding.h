/*
 * gxr
 * Copyright 2020 Collabora Ltd.
 * Author: Christoph Haag <christop.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_OPENXR_ACTION_BINDING_H_
#define GXR_OPENXR_ACTION_BINDING_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include "openxr-action-set.h"

/* TODO: get bindings from file, allocate dynamically */
#define NUM_INTERACTION_PROFILES 3
#define NUM_ACTION_PATHS 12
/* currently 1 binding per hand hardcoded */
#define NUM_COMPONENTS 2

typedef struct {
  char *name;
  char *component[NUM_COMPONENTS];
  int num_components;

  /* whether this action is bound in this interaction profile */
  gboolean bound;
} OpenXRActionBinding;

typedef struct {
  char *interaction_profile;
  OpenXRActionBinding action_bindings[NUM_ACTION_PATHS];
} OpenXRBinding;

static char *interaction_profiles[NUM_INTERACTION_PROFILES] = {
  "/interaction_profiles/khr/simple_controller",
  "/interaction_profiles/mnd/ball_on_stick_controller",
  "/interaction_profiles/valve/index_controller",
};

static OpenXRBinding b[NUM_INTERACTION_PROFILES] = {
  {
    .interaction_profile = "/interaction_profiles/khr/simple_controller",

    .action_bindings[0] = {
      .name = "/actions/wm/in/grab_window",
      .component = {
        "/user/hand/left/input/select/click",
        "/user/hand/right/input/select/click"
      },
      .num_components = NUM_COMPONENTS,
      .bound = true,
    },

    .action_bindings[1] = {
      .name = "/actions/wm/in/reset_orientation",
      .bound = false,
    },

    .action_bindings[2] = {
      .name = "/actions/wm/in/show_keyboard",
      .bound = false,
    },

    .action_bindings[3] = {
      .name = "/actions/wm/in/push_pull",
      .bound = false,
    },


    .action_bindings[4] = {
      .name = "/actions/wm/in/push_pull_scale",
      .bound = false,
    },

    .action_bindings[5] = {
      .name = "/actions/wm/in/hand_pose",
      .component[0] = "/user/hand/left/input/aim/pose",
      .component[1] = "/user/hand/right/input/aim/pose",
      .num_components = NUM_COMPONENTS,
      .bound = true,
    },

    .action_bindings[6] = {
      .name = "/actions/wm/in/hand_pose_hand_grip",
      .component[0] = "/user/hand/left/input/grip/pose",
      .component[1] = "/user/hand/right/input/grip/pose",
      .num_components = NUM_COMPONENTS,
      .bound = true,
    },

    .action_bindings[7] = {
      .name = "/actions/wm/out/haptic",
      .component[0] = "/user/hand/left/output/haptic",
      .component[1] = "/user/hand/right/output/haptic",
      .num_components = NUM_COMPONENTS,
      .bound = true,
    },

    .action_bindings[8] = {
      .name = "/actions/wm/in/menu",
      .bound = false,
    },



    .action_bindings[9] = {
      .name = "/actions/mouse_synth/in/left_click",
      .bound = false,
    },

    .action_bindings[10] = {
      .name = "/actions/mouse_synth/in/right_click",
      .bound = false,
    },

    .action_bindings[11] = {
      .name = "/actions/mouse_synth/in/scroll",
      .bound = false,
    },
  },

  {
    .interaction_profile = "/interaction_profiles/mnd/ball_on_stick_controller",

    .action_bindings[0] = {
      .name ="/actions/wm/in/grab_window",
      .component[0] = "/user/hand/left/input/select/click",
      .component[1] = "/user/hand/right/input/select/click",
      .num_components = NUM_COMPONENTS,
      .bound = true,
    },

    .action_bindings[1] = {
      .name = "/actions/wm/in/reset_orientation",
      .bound = false,
    },

    .action_bindings[2] = {
      .name = "/actions/wm/in/show_keyboard",
      .bound = false,
    },

    .action_bindings[3] = {
      .name = "/actions/wm/in/push_pull",
      .bound = false,
    },

    .action_bindings[4] = {
      .name = "/actions/wm/in/push_pull_scale",
      .bound = false,
    },

    .action_bindings[5] = {
      .name = "/actions/wm/in/hand_pose",
      .component[0] = "/user/hand/left/input/aim/pose",
      .component[1] = "/user/hand/right/input/aim/pose",
      .num_components = NUM_COMPONENTS,
      .bound = true,
    },

    .action_bindings[6] = {
      .name = "/actions/wm/in/hand_pose_hand_grip",
      .component[0] = "/user/hand/left/input/grip/pose",
      .component[1] = "/user/hand/right/input/grip/pose",
      .num_components = NUM_COMPONENTS,
      .bound = true,
    },

    .action_bindings[7] = {
      .name = "/actions/wm/out/haptic",
      .component[0] = "/user/hand/left/output/haptic",
      .component[1] = "/user/hand/right/output/haptic",
      .num_components = NUM_COMPONENTS,
      .bound = true,
    },

    .action_bindings[8] = {
      .name = "/actions/wm/in/menu",
      .bound = false,
    },

    .action_bindings[9] = {
      .name = "/actions/mouse_synth/in/left_click",
      .bound = false,
    },

    .action_bindings[10] = {
      .name = "/actions/mouse_synth/in/right_click",
      .bound = false,
    },

    .action_bindings[11] = {
      .name = "/actions/mouse_synth/in/scroll",
      .bound = false,
    },
  },

  {
    .interaction_profile = "/interaction_profiles/valve/index_controller",

    .action_bindings[0] = {
      .name = "/actions/wm/in/grab_window",
      .component[0] = "/user/hand/left/input/trigger/value",
      .component[1] = "/user/hand/right/input/trigger/value",
      .num_components = NUM_COMPONENTS,
      .bound = true,
    },

    .action_bindings[1] = {
      .name = "/actions/wm/in/reset_orientation",
      .component[0] = "/user/hand/left/input/trigger/click",
      .component[1] = "/user/hand/right/input/trigger/click",
      .num_components = NUM_COMPONENTS,
      .bound = true,
    },

    .action_bindings[2] = {
      .name = "/actions/wm/in/show_keyboard",
      .component[0] = "/user/hand/left/input/thumbstick/click",
      .component[1] = "/user/hand/right/input/thumbstick/click",
      .num_components = NUM_COMPONENTS,
      .bound = true,
    },

    .action_bindings[3] = {
      .name = "/actions/wm/in/push_pull",
      .component[0] = "/user/hand/left/input/trackpad/y",
      .component[1] = "/user/hand/right/input/trackpad/y",
      .num_components = NUM_COMPONENTS,
      .bound = true,
    },

    .action_bindings[4] = {
      .name = "/actions/wm/in/push_pull_scale",
      .component[0] = "/user/hand/left/input/trackpad",
      .component[1] = "/user/hand/right/input/trackpad",
      .bound = true,
    },

    .action_bindings[5] = {
      .name = "/actions/wm/in/hand_pose",
      .component[0] = "/user/hand/left/input/aim/pose",
      .component[1] = "/user/hand/right/input/aim/pose",
      .num_components = NUM_COMPONENTS,
      .bound = true,
    },

    .action_bindings[6] = {
      .name = "/actions/wm/in/hand_pose_hand_grip",
      .component[0] = "/user/hand/left/input/grip/pose",
      .component[1] = "/user/hand/right/input/grip/pose",
      .num_components = NUM_COMPONENTS,
      .bound = true,
    },

    .action_bindings[7] = {
      .name = "/actions/wm/out/haptic",
      .component[0] = "/user/hand/left/output/haptic",
      .component[1] = "/user/hand/right/output/haptic",
      .num_components = NUM_COMPONENTS,
      .bound = true,
    },

    .action_bindings[8] = {
      .name = "/actions/wm/in/menu",
      .component[0] = "/user/hand/left/input/b/click",
      .component[1] = "/user/hand/right/input/b/click",
      .num_components = NUM_COMPONENTS,
      .bound = true,
    },

    .action_bindings[9] = {
      .name = "/actions/mouse_synth/in/left_click",
      .component[0] = "/user/hand/left/input/a/click",
      .component[1] = "/user/hand/right/input/a/click",
      .num_components = NUM_COMPONENTS,
      .bound = true,
    },

    .action_bindings[10] = {
      .name = "/actions/mouse_synth/in/right_click",
      .component[0] = "/user/hand/left/input/b/click",
      .component[1] = "/user/hand/right/input/b/click",
      .num_components = NUM_COMPONENTS,
      .bound = true,
    },

    .action_bindings[11] = {
      .name = "/actions/mouse_synth/in/scroll",
      .component[0] = "/user/hand/left/input/trackpad",
      .component[1] = "/user/hand/right/input/trackpad",
      .bound = false,
    },
  },
};

static inline void
openxr_action_binding_get (OpenXRBinding **bindings,
                           int *num_bindings,
                           int *actions_per_binding)
{
  *bindings = b;
  *num_bindings = NUM_INTERACTION_PROFILES;
  *actions_per_binding = NUM_ACTION_PATHS;
}

static inline void
openxr_action_bindings_get_interaction_profiles (char ***profiles,
                                                 int *num_profiles)
{
  *profiles = interaction_profiles;
  *num_profiles = NUM_INTERACTION_PROFILES;
}

#endif  // GXR_OPENXR_ACTION_BINDING_H_