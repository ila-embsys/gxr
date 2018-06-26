#include <stdio.h>
#include <string.h>

#include "openvr-vulkan-uploader.h"

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

int main(int argc, char *argv[])
{
  OpenVRVulkanUploader *uploader = openvr_vulkan_uploader_new ();

  for (int i = 1; i < argc; i++)
  {
    if (!strcmp (argv[i], "--validate"))
    {
      uploader->enable_validation = true;
    }
  }

  if (!openvr_vulkan_uploader_init_openvr (uploader))
  {
    openvr_vulkan_uploader_shutdown (uploader);
    return 1;
  }

  _main_loop(uploader);
  openvr_vulkan_uploader_shutdown (uploader);

  return 0;
}
