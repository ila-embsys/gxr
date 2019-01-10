#include <glib.h>
#include <glib-unix.h>

#include "openvr-scene-renderer.h"

typedef struct Example
{
  OpenVRSceneRenderer *renderer;
  GMainLoop *loop;
} Example;

void
_cleanup (Example *self)
{
  g_print ("bye\n");
  g_object_unref (self->renderer);
}

gboolean
_sigint_cb (gpointer _self)
{
  Example *self = (Example*) _self;
  g_main_loop_quit (self->loop);
  return TRUE;
}

gboolean
_iterate_cb (gpointer _self)
{
  Example *self = (Example*) _self;
  openvr_scene_renderer_render (self->renderer);
  return true;
}

int
main ()
{
  Example self = {
    .loop = g_main_loop_new (NULL, FALSE),
    .renderer = openvr_scene_renderer_new ()
  };

  if (!openvr_scene_renderer_initialize (self.renderer))
    {
      _cleanup (&self);
      return 1;
    }

  g_timeout_add (1, _iterate_cb, &self);

  g_unix_signal_add (SIGINT, _sigint_cb, &self);

  /* start glib main loop */
  g_main_loop_run (self.loop);
  g_main_loop_unref (self.loop);

  _cleanup (&self);

  return 0;
}
