#include <stdio.h>
#include <string.h>

#include "openvr-vulkan-uploader.h"
#include "openvr_capi_global.h"
#include "openvr-global.h"

gboolean
_system_init_fn_table2 (OpenVRVulkanUploader *self)
{
  INIT_FN_TABLE (self->system, System);
}

gboolean
_overlay_init_fn_table2 (OpenVRVulkanUploader *self)
{
  INIT_FN_TABLE (self->overlay, Overlay);
}

gboolean
_compositor_init_fn_table (OpenVRVulkanUploader *self)
{
  INIT_FN_TABLE (self->compositor, Compositor);
}

gboolean
example_init (OpenVRVulkanUploader *self)
{
  EVRInitError error;
  VR_InitInternal(&error, EVRApplicationType_VRApplication_Overlay);

  if (error != EVRInitError_VRInitError_None) {
    g_printerr ("Could not init OpenVR runtime: Error code %s\n",
                VR_GetVRInitErrorAsSymbol (error));
    return false;
  }

  _system_init_fn_table2 (self);

  if (!_compositor_init_fn_table (self))
  {
    g_printerr ("Compositor initialization failed.\n");
    return false;
  }

  if (!openvr_vulkan_uploader_init_vulkan (self))
  {
    g_printerr ("Unable to initialize Vulkan!\n");
    return false;
  }

  if (_overlay_init_fn_table2 (self))
  {
    EVROverlayError err = self->overlay->CreateDashboardOverlay (
      "vulkan-overlay", "vkovl",
      &self->overlay_handle, &self->thumbnail_handle);

    if (err != EVROverlayError_VROverlayError_None)
      {
        g_printerr ("Could not create overlay: %s\n",
                    self->overlay->GetOverlayErrorNameFromEnum (err));
        return false;
      }
  } else {
    return false;
  }

  return true;
}

void
_main_loop (OpenVRVulkanUploader *uploader)
{
  bool quit = false;
  while (!quit)
  {
    // Process SteamVR events
    struct VREvent_t event;
    while (uploader->overlay->PollNextOverlayEvent (uploader->overlay_handle,
                                                   &event, sizeof (event)))
    {
      if (event.eventType == EVREventType_VREvent_MouseButtonDown)
        quit = true;
    }

    openvr_vulkan_uploader_submit_frame (uploader);
  }
}

int
main(int argc, char *argv[])
{
  OpenVRVulkanUploader *uploader = openvr_vulkan_uploader_new ();

  for (int i = 1; i < argc; i++)
    if (!strcmp (argv[i], "--validate"))
      uploader->enable_validation = true;

  if (example_init (uploader))
    _main_loop(uploader);

  g_object_unref (uploader);

  return 0;
}
