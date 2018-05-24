#include <stdio.h>

#define COGL_ENABLE_EXPERIMENTAL_API 1
#include <cogl/cogl.h>

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glext.h>

void gl_debug_msg(const char *msg, unsigned length)
{
  glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 0,
                       GL_DEBUG_SEVERITY_NOTIFICATION, length, msg);
}

/* The state for this example... */
typedef struct _Data
{
  CoglFramebuffer *fb;
  int framebuffer_width;
  int framebuffer_height;

  CoglMatrix view;

  CoglIndices *indices;
  CoglPrimitive *prim;
  CoglTexture *texture;
  CoglPipeline *crate_pipeline;

  GTimer *timer;

  CoglBool swap_ready;

} Data;

/* A static identity matrix initialized for convenience. */
static CoglMatrix identity;
/* static colors initialized for convenience. */
static CoglColor white;

/* A cube modelled using 4 vertices for each face.
 *
 * We use an index buffer when drawing the cube later so the GPU will
 * actually read each face as 2 separate triangles.
 */
static CoglVertexP3T2 vertices[] =
{
  /* Front face */
  { /* pos = */ -1.0f, -1.0f,  1.0f, /* tex coords = */ 0.0f, 1.0f},
  { /* pos = */  1.0f, -1.0f,  1.0f, /* tex coords = */ 1.0f, 1.0f},
  { /* pos = */  1.0f,  1.0f,  1.0f, /* tex coords = */ 1.0f, 0.0f},
  { /* pos = */ -1.0f,  1.0f,  1.0f, /* tex coords = */ 0.0f, 0.0f},

  /* Back face */
  { /* pos = */ -1.0f, -1.0f, -1.0f, /* tex coords = */ 1.0f, 0.0f},
  { /* pos = */ -1.0f,  1.0f, -1.0f, /* tex coords = */ 1.0f, 1.0f},
  { /* pos = */  1.0f,  1.0f, -1.0f, /* tex coords = */ 0.0f, 1.0f},
  { /* pos = */  1.0f, -1.0f, -1.0f, /* tex coords = */ 0.0f, 0.0f},

  /* Top face */
  { /* pos = */ -1.0f,  1.0f, -1.0f, /* tex coords = */ 0.0f, 1.0f},
  { /* pos = */ -1.0f,  1.0f,  1.0f, /* tex coords = */ 0.0f, 0.0f},
  { /* pos = */  1.0f,  1.0f,  1.0f, /* tex coords = */ 1.0f, 0.0f},
  { /* pos = */  1.0f,  1.0f, -1.0f, /* tex coords = */ 1.0f, 1.0f},

  /* Bottom face */
  { /* pos = */ -1.0f, -1.0f, -1.0f, /* tex coords = */ 1.0f, 1.0f},
  { /* pos = */  1.0f, -1.0f, -1.0f, /* tex coords = */ 0.0f, 1.0f},
  { /* pos = */  1.0f, -1.0f,  1.0f, /* tex coords = */ 0.0f, 0.0f},
  { /* pos = */ -1.0f, -1.0f,  1.0f, /* tex coords = */ 1.0f, 0.0f},

  /* Right face */
  { /* pos = */ 1.0f, -1.0f, -1.0f, /* tex coords = */ 1.0f, 0.0f},
  { /* pos = */ 1.0f,  1.0f, -1.0f, /* tex coords = */ 1.0f, 1.0f},
  { /* pos = */ 1.0f,  1.0f,  1.0f, /* tex coords = */ 0.0f, 1.0f},
  { /* pos = */ 1.0f, -1.0f,  1.0f, /* tex coords = */ 0.0f, 0.0f},

  /* Left face */
  { /* pos = */ -1.0f, -1.0f, -1.0f, /* tex coords = */ 0.0f, 0.0f},
  { /* pos = */ -1.0f, -1.0f,  1.0f, /* tex coords = */ 1.0f, 0.0f},
  { /* pos = */ -1.0f,  1.0f,  1.0f, /* tex coords = */ 1.0f, 1.0f},
  { /* pos = */ -1.0f,  1.0f, -1.0f, /* tex coords = */ 0.0f, 1.0f}
};

static void
paint (Data *data)
{
  CoglFramebuffer *fb = data->fb;
  float rotation;

  cogl_framebuffer_clear4f (fb,
                            COGL_BUFFER_BIT_COLOR|COGL_BUFFER_BIT_DEPTH,
                            0, 0, 0, 1);

  cogl_framebuffer_push_matrix (fb);

  cogl_framebuffer_translate (fb,
                              data->framebuffer_width / 2,
                              data->framebuffer_height / 2,
                              0);

  cogl_framebuffer_scale (fb, 75, 75, 75);

  /* Update the rotation based on the time the application has been
     running so that we get a linear animation regardless of the frame
     rate */
  rotation = g_timer_elapsed (data->timer, NULL) * 60.0f;

  /* Rotate the cube separately around each axis.
   *
   * Note: Cogl matrix manipulation follows the same rules as for
   * OpenGL. We use column-major matrices and - if you consider the
   * transformations happening to the model - then they are combined
   * in reverse order which is why the rotation is done last, since
   * we want it to be a rotation around the origin, before it is
   * scaled and translated.
   */
  cogl_framebuffer_rotate (fb, rotation, 0, 0, 1);
  cogl_framebuffer_rotate (fb, rotation, 0, 1, 0);
  cogl_framebuffer_rotate (fb, rotation, 1, 0, 0);

  cogl_primitive_draw (data->prim, fb, data->crate_pipeline);

  cogl_framebuffer_pop_matrix (fb);
}

static void
frame_event_cb (CoglOnscreen *onscreen,
                CoglFrameEvent event,
                CoglFrameInfo *info,
                void *user_data)
{
  Data *data = user_data;

  if (event == COGL_FRAME_EVENT_SYNC)
    data->swap_ready = TRUE;
}

GdkPixbuf *
load_gdk_pixbuf ()
{
  GError *error = NULL;
  GdkPixbuf * pixbuf_unflipped =
    gdk_pixbuf_new_from_resource ("/res/cat.jpg", &error);

  if (error != NULL) {
    fprintf (stderr, "Unable to read file: %s\n", error->message);
    g_error_free (error);
    return NULL;
  } else {
    GdkPixbuf * pixbuf = gdk_pixbuf_flip (pixbuf_unflipped, FALSE);
    g_object_unref (pixbuf_unflipped);
    return pixbuf;
  }
}


void
print_pixbuf_info (GdkPixbuf * pixbuf)
{
  gint n_channels = gdk_pixbuf_get_n_channels (pixbuf);

  GdkColorspace colorspace = gdk_pixbuf_get_colorspace (pixbuf);

  gint bits_per_sample = gdk_pixbuf_get_bits_per_sample (pixbuf);
  gboolean has_alpha = gdk_pixbuf_get_has_alpha (pixbuf);

  gint width = gdk_pixbuf_get_width (pixbuf);
  gint height = gdk_pixbuf_get_height (pixbuf);

  gint rowstride = gdk_pixbuf_get_rowstride (pixbuf);

  g_print ("We loaded a pixbuf.\n");
  g_print ("channels %d\n", n_channels);
  g_print ("colorspace %d\n", colorspace);
  g_print ("bits_per_sample %d\n", bits_per_sample);
  g_print ("has_alpha %d\n", has_alpha);
  g_print ("pixel dimensions %dx%d\n", width, height);
  g_print ("rowstride %d\n", rowstride);
}

int
main (int argc, char **argv)
{
  CoglContext *ctx;
  CoglOnscreen *onscreen;
  CoglFramebuffer *fb;
  CoglError *error = NULL;
  Data data;
  //PangoRectangle hello_label_size;
  float fovy, aspect, z_near, z_2d, z_far;
  CoglDepthState depth_state;

  ctx = cogl_context_new (NULL, &error);
  if (!ctx) {
      fprintf (stderr, "Failed to create context: %s\n", error->message);
      return 1;
  }

  onscreen = cogl_onscreen_new (ctx, 640, 480);
  fb = onscreen;
  data.fb = fb;
  data.framebuffer_width = cogl_framebuffer_get_width (fb);
  data.framebuffer_height = cogl_framebuffer_get_height (fb);

  data.timer = g_timer_new ();

  cogl_onscreen_show (onscreen);

  cogl_framebuffer_set_viewport (fb,
                                 0, 0,
                                 data.framebuffer_width,
                                 data.framebuffer_height);

  fovy = 60; /* y-axis field of view */
  aspect = (float)data.framebuffer_width/(float)data.framebuffer_height;
  z_near = 0.1; /* distance to near clipping plane */
  z_2d = 1000; /* position to 2d plane */
  z_far = 2000; /* distance to far clipping plane */

  cogl_framebuffer_perspective (fb, fovy, aspect, z_near, z_far);

  /* Since the pango renderer emits geometry in pixel/device coordinates
   * and the anti aliasing is implemented with the assumption that the
   * geometry *really* does end up pixel aligned, we setup a modelview
   * matrix so that for geometry in the plane z = 0 we exactly map x
   * coordinates in the range [0,stage_width] and y coordinates in the
   * range [0,stage_height] to the framebuffer extents with (0,0) being
   * the top left.
   *
   * This is roughly what Clutter does for a ClutterStage, but this
   * demonstrates how it is done manually using Cogl.
   */
  cogl_matrix_init_identity (&data.view);
  cogl_matrix_view_2d_in_perspective (&data.view, fovy, aspect, z_near, z_2d,
                                      data.framebuffer_width,
                                      data.framebuffer_height);
  cogl_framebuffer_set_modelview_matrix (fb, &data.view);

  /* Initialize some convenient constants */
  cogl_matrix_init_identity (&identity);
  cogl_color_init_from_4ub (&white, 0xff, 0xff, 0xff, 0xff);

  /* rectangle indices allow the GPU to interpret a list of quads (the
   * faces of our cube) as a list of triangles.
   *
   * Since this is a very common thing to do
   * cogl_get_rectangle_indices() is a convenience function for
   * accessing internal index buffers that can be shared.
   */
  data.indices = cogl_get_rectangle_indices (ctx, 6 /* n_rectangles */);
  data.prim = cogl_primitive_new_p3t2 (ctx, COGL_VERTICES_MODE_TRIANGLES,
                                       G_N_ELEMENTS (vertices),
                                       vertices);
  /* Each face will have 6 indices so we have 6 * 6 indices in total... */
  cogl_primitive_set_indices (data.prim,
                              data.indices,
                              6 * 6);

  /* Load a jpeg crate texture from a file */

    GdkPixbuf * pixbuf = load_gdk_pixbuf ();
  print_pixbuf_info (pixbuf);

  gl_debug_msg("before upload", 13);
  /*


  CoglTexture2D *
  */


  //glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  //glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
/*
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
*/
  int width = gdk_pixbuf_get_width (pixbuf);
  int height = gdk_pixbuf_get_height (pixbuf);
  guchar *pixels = gdk_pixbuf_get_pixels (pixbuf);

  GLenum format;
  CoglPixelFormat cogl_format;
  switch (gdk_pixbuf_get_n_channels (pixbuf))
  {
    case 3:
      format = GL_RGB;
    cogl_format = COGL_PIXEL_FORMAT_RGB_888;
      break;
    case 4:
      format = GL_RGBA;
    cogl_format = COGL_PIXEL_FORMAT_RGBA_8888;
          break;
    default:
      format = GL_RGBA;
      cogl_format = COGL_PIXEL_FORMAT_RGBA_8888;
      }



//glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_RGB, 2370, 1927, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
//glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, 2370);

  GLuint tex;
  glGenTextures (1, &tex);
  glBindTexture (GL_TEXTURE_2D, tex);

  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, width);
  /*
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
  glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
  */

//glBindTexture(GL_TEXTURE_2D, texture = 4)
//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2370, 1927, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
  glBindTexture (GL_TEXTURE_2D, tex);


  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, width, height,
                0, format, GL_UNSIGNED_BYTE, pixels);

  data.texture =
    cogl_texture_2d_gl_new_from_foreign (ctx,
                                     tex,
                                     width,
                                     height,
                                     cogl_format);
/*

  gint width = gdk_pixbuf_get_width (pixbuf);
  gint height = gdk_pixbuf_get_height (pixbuf);

  gint rowstride = gdk_pixbuf_get_rowstride (pixbuf);

  guchar *pixels = gdk_pixbuf_get_pixels (pixbuf);

  CoglPixelFormat format;
  switch (gdk_pixbuf_get_n_channels (pixbuf))
  {
    case 3:
      format = COGL_PIXEL_FORMAT_RGB_888;
      break;
    case 4:
      format = COGL_PIXEL_FORMAT_RGBA_8888;
      break;
    default:
      format = COGL_PIXEL_FORMAT_RGBA_8888;
  }

  data.texture =
  cogl_texture_2d_new_from_data (ctx,
                               width,
                               height,
                               format,
                               rowstride,
                               pixels,
                               &error);

*/
    gl_debug_msg("after upload", 12);

  /*
  data.texture =
    cogl_texture_2d_new_from_file (ctx,
                                   "/home/bmonkey/workspace/aur/cogl/trunk/src/cogl/examples/crate.jpg",
                                   &error);
 */
  if (!data.texture)
    g_error ("Failed to load texture: %s", error->message);

  /* a CoglPipeline conceptually describes all the state for vertex
   * processing, fragment processing and blending geometry. When
   * drawing the geometry for the crate this pipeline says to sample a
   * single texture during fragment processing... */
  data.crate_pipeline = cogl_pipeline_new (ctx);
  cogl_pipeline_set_layer_texture (data.crate_pipeline, 0, data.texture);

  /* Since the box is made of multiple triangles that will overlap
   * when drawn and we don't control the order they are drawn in, we
   * enable depth testing to make sure that triangles that shouldn't
   * be visible get culled by the GPU. */
  cogl_depth_state_init (&depth_state);
  cogl_depth_state_set_test_enabled (&depth_state, TRUE);

  cogl_pipeline_set_depth_state (data.crate_pipeline, &depth_state, NULL);

  data.swap_ready = TRUE;

  cogl_onscreen_add_frame_callback (COGL_ONSCREEN (fb),
                                    frame_event_cb,
                                    &data,
                                    NULL); /* destroy notify */

  while (1)
    {
      CoglPollFD *poll_fds;
      int n_poll_fds;
      int64_t timeout;

      if (data.swap_ready)
        {
          paint (&data);
          cogl_onscreen_swap_buffers (COGL_ONSCREEN (fb));
        }

      cogl_poll_renderer_get_info (cogl_context_get_renderer (ctx),
                                   &poll_fds, &n_poll_fds, &timeout);

      g_poll ((GPollFD *) poll_fds, n_poll_fds,
              timeout == -1 ? -1 : timeout / 1000);

      cogl_poll_renderer_dispatch (cogl_context_get_renderer (ctx),
                                   poll_fds, n_poll_fds);
    }

  return 0;
}

