#include "../include/core.hpp"
#include "../include/opengl.hpp"

#include <GL/glext.h>
#include <EGL/egl.h>

extern "C" {
#include "./wlc/include/wlc/geometry.h"
#include "./wlc/include/wlc/wlc.h"
#include "./wlc/src/resources/types/surface.h"
#include "./wlc/src/resources/types/buffer.h"
#include "./wlc/src/compositor/view.h"
#include "./wlc/src/resources/resources.h"
#include "./wlc/src/platform/context/context.h"
#include "./wlc/src/compositor/output.h"

struct wlc_output* out;

//<<<<<<< Updated upstream
//GLuint background;
//
//void terminate(ctx *context) { }
//void resolution(ctx *context, const wlc_size *mode, const wlc_size *resolution) {
//    glViewport(0, 0, mode->w, mode->h);
//}
//
//void surface_destroy(ctx *context, struct wlc_context *bound, struct wlc_surface *surface) {
//    for(int i = 0; i < 3; i++)
//        if(surface->textures[i])
//            glDeleteTextures(1, &surface->textures[i]);
//}
//
//void surface_gen_textures(struct wlc_surface *surface, const GLuint num_textures)                                                      
//{                                                                                                                                 
//   assert(surface);                                                                                                               
//                                                                                                                                  
//   for (GLuint i = 0; i < num_textures; ++i) {                                                                                    
//      if (surface->textures[i])                                                                                                   
//         continue;                                                                                                                
//                                                                                                                                  
//      glGenTextures(1, &surface->textures[i]);                                                                    
//      glBindTexture(GL_TEXTURE_2D, surface->textures[i]);                                                         
//      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);                                        
//      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);                                        
//   }                                                                                                                              
//}                                                                                                                                 
//
//bool shm_attach(struct wlc_surface *surface, struct wlc_buffer *buffer, wl_shm_buffer *shm_buffer) {                                                                                                                                                                                                                                                                         
//   assert(surface && buffer && shm_buffer);                                                                                                                                                                                                                               
//                                                                                                                                                                                                                                                                          
//   buffer->shm_buffer = shm_buffer;                                                                                                                                                                                                                                       
//   buffer->size.w = wl_shm_buffer_get_width(shm_buffer);                                                                                                                                                                                                                  
//   buffer->size.h = wl_shm_buffer_get_height(shm_buffer);                                                                                                                                                                                                                 
//   GLint pitch;                                                                                                                                                                                                                                                           
//
//   GLenum gl_format, gl_pixel_type;                                                                                                                                                                                                                                       
//   switch (wl_shm_buffer_get_format(shm_buffer)) {                                                                                                                                                                                                                        
//      case WL_SHM_FORMAT_XRGB8888:                                                                                                                                                                                                                                        
//         pitch = wl_shm_buffer_get_stride(shm_buffer) / 4;                                                                                                                                                                                                                
//         gl_format = GL_RGBA;                                                                                                                                                                                                                                         
//         gl_pixel_type = GL_UNSIGNED_BYTE;                                                                                                                                                                                                                                
//         surface->format = SURFACE_RGB;                                                                                                                                                                                                                                   
//         break;                                                                                                                                                                                                                                                           
//      case WL_SHM_FORMAT_ARGB8888:                                                                                                                                                                                                                                        
//         pitch = wl_shm_buffer_get_stride(shm_buffer) / 4;                                                                                                                                                                                                                
//         gl_format = GL_RGBA;                                                                                                                                                                                                                                         
//         gl_pixel_type = GL_UNSIGNED_BYTE;                                                                                                                                                                                                                                
//         surface->format = SURFACE_RGBA;                                                                                                                                                                                                                                  
//         break;                                                                                                                                                                                                                                                           
//      case WL_SHM_FORMAT_RGB565:                                                                                                                                                                                                                                          
//         pitch = wl_shm_buffer_get_stride(shm_buffer) / 2;                                                                                                                                                                                                                
//         gl_format = GL_RGB;                                                                                                                                                                                                                                              
//         gl_pixel_type = GL_UNSIGNED_SHORT_5_6_5;                                                                                                                                                                                                                         
//         surface->format = SURFACE_RGB;                                                                                                                                                                                                                                   
//         break;                                                                                                                                                                                                                                                           
//      default:                                                                                                                                                                                                                                                            
//         /* unknown shm buffer format */                                                                                                                                                                                                                                  
//         return false;                                                                                                                                                                                                                                                    
//   }                                                                                                                                                                                                                                                                      
//                                                                                                                                                                                                                                                                          
//   struct wlc_view *view;                                                                                                                                                                                                                                                 
//   if ((view = (struct wlc_view*)convert_from_wlc_handle(surface->view, "view")) && view->x11.id)                                                                                                                                                                                           
//      surface->format = wlc_x11_window_get_surface_format(&view->x11);                                                                                                                                                                                                    
//                                                                                                                                                                                                                                                                          
//   surface_gen_textures(surface, 1);                                                                                                                                                                                                                                      
//
//   glBindTexture(GL_TEXTURE_2D, surface->textures[0]);                                                                                                                                                                                                    
//   glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch);                                                                                                                                                                                                        
//   glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);                                                                                                                                                                                                           
//   glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);                                                                                                                                                                                                             
//   wl_shm_buffer_begin_access(buffer->shm_buffer);                                                                                                                                                                                                                        
//   void *data = wl_shm_buffer_get_data(buffer->shm_buffer);                                                                                                                                                                                                               
//   glTexImage2D(GL_TEXTURE_2D, 0, gl_format, pitch, buffer->size.h, 0, gl_format, gl_pixel_type, data); 
//   wl_shm_buffer_end_access(buffer->shm_buffer);                                                                                                                                                                                                                          
//   return true;                                                                                                                                                                                                                                                           
//}                                                                                                                                                                                                                                                                         
//      
//
//bool surface_attach(ctx *context, struct wlc_context *bound,
//        struct wlc_surface *surface, struct wlc_buffer *buffer) {
//
//    assert(context && bound && surface);                                                                                           
//
//    wl_resource *wl_buffer;                                                                                                 
//    if (!buffer || !(wl_buffer = convert_to_wl_resource(buffer, "buffer"))) {                                                      
//        surface_destroy(context, bound, surface);                                                                                   
//        return true;                                                                                                                
//    }                                                                                                                              
//
//    bool attached = false;                                                                                                         
//
//    wl_shm_buffer *shm_buffer = wl_shm_buffer_get(wl_buffer);                                                               
//    if (shm_buffer) {                                                                                                              
//        attached = shm_attach(surface, buffer, shm_buffer);                                                                         
//    } else {                                                                                                                       
//        /* unknown buffer */                                                                                                        
//        std::cout << "Unknown buffer" << std::endl;
//    }                                                                                                                              
//
//    return attached;                                                                                                               
//}           
//
//bool inited = false;
//
//void                                                                                                                                                         
//surface_paint_internal(struct ctx *context, struct wlc_surface *surface,                                                                                            
//        const struct wlc_geometry *geometry)                                                                                                
//{                                                                                                                                                                   
//   assert(context && surface && geometry);                                                                                                              
//
//   //wlc_context_bind(&out->context);
//
//   GLuint vao, vbo;
//   OpenGL::generateVAOVBO(geometry->origin.x, geometry->origin.y,
//           geometry->size.w, geometry->size.h, vao, vbo);
//
//   OpenGL::renderTexture(surface->textures[0], vao, vbo);
//}
//
//void view_paint(ctx *context, struct wlc_view *view) {
//
//    assert(context && view);                                                                                                       
//
//    struct wlc_surface *surface;                                                                                                   
//    if (!(surface = (struct wlc_surface*)convert_from_wlc_resource(view->surface, "surface")))                                                          
//        return;                                                                                                                     
//
//    struct wlc_geometry geometry, dummyy;                                                                                                  
//    wlc_view_get_bounds(view, &geometry, &dummyy);                                                                       
//
//    surface_paint_internal(context, surface, &geometry);                                                                
//}
//   
//void surface_paint(struct ctx *context, struct wlc_surface *surface, const wlc_geometry *geometry) {
//     surface_paint_internal(context, surface, geometry);                                                                 
//}
//
//void pointer_paint(struct ctx *context, const wlc_point *pos) {
//    GLuint vao, vbo;
//    OpenGL::generateVAOVBO(pos->x, pos->y, 100, 100, vao, vbo);
//    OpenGL::renderTransformedTexture(background, vao, vbo, glm::mat4());
//}
//
//void read_pixels(struct ctx *context, wlc_geometry *geometry, void *out_data) {
//    glReadPixels(geometry->origin.x, geometry->origin.y,
//            geometry->size.w, geometry->size.h,
//            GL_RGBA, GL_UNSIGNED_BYTE, out_data);
//}
//
//
//
//void clear(ctx *context) {
//
//    glClearColor(1, 1, 0, 1);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//    GLuint vao, vbo;
//    OpenGL::generateVAOVBO(0, 0, 1366, 768, vao, vbo);
//    OpenGL::renderTransformedTexture(background, vao, vbo, glm::mat4());
//}
//
//
//void opengl_init(wlc_render_api *api) {
//
//    OpenGL::initOpenGL("/usr/local/share/fireman/shaders/");
//    background = GLXUtils::loadImage("/home/ilex/Desktop/maxresdefault.jpg");
//
//    api->terminate = terminate;
//    api->resolution = resolution;
//    api->surface_destroy = surface_destroy;
//    api->surface_attach = surface_attach;
//    api->view_paint = view_paint;
//    api->surface_paint = surface_paint;
//    api->pointer_paint = pointer_paint;
//    api->read_pixels = read_pixels;
//    api->clear = clear;
//}
//=======
//void opengl_init(wlc_render_api *api) {
//
//    printf("try\n");
//    std::cout.flush();
//
//    OpenGL::initOpenGL("/usr/local/share/fireman/shaders");
//    OpenGL::depth = 24;
//
//    background = GLXUtils::loadImage("/home/ilex/Desktop/maxresdefault.jpg");
//
//    api->terminate = terminate;
//    api->resolution = resolution;
//    api->surface_destroy = surface_destroy;
//    api->surface_attach = surface_attach;
//    api->view_paint = view_paint;
//    api->surface_paint = surface_paint;
//    api->pointer_paint = pointer_paint;
//    api->read_pixels = read_pixels;
//    api->clear = clear;
//}
//>>>>>>> Stashed changes


static struct {
   struct {
      wlc_handle view;
      struct wlc_point grab;
      uint32_t edges;
   } action;
} compositor;

static bool
start_interactive_action(wlc_handle view, const struct wlc_point *origin)
{
   if (compositor.action.view)
      return false;

   compositor.action.view = view;
   compositor.action.grab = *origin;
   wlc_view_bring_to_front(view);
   return true;
}

static void
start_interactive_move(wlc_handle view, const struct wlc_point *origin)
{
   start_interactive_action(view, origin);
}

static void
start_interactive_resize(wlc_handle view, uint32_t edges, const struct wlc_point *origin)
{
   const struct wlc_geometry *g;
   if (!(g = wlc_view_get_geometry(view)) || !start_interactive_action(view, origin))
      return;

   const int32_t halfw = g->origin.x + g->size.w / 2;
   const int32_t halfh = g->origin.y + g->size.h / 2;

   if (!(compositor.action.edges = edges)) {
      compositor.action.edges = (origin->x < halfw ? WLC_RESIZE_EDGE_LEFT : (origin->x > halfw ? WLC_RESIZE_EDGE_RIGHT : 0)) |
                                (origin->y < halfh ? WLC_RESIZE_EDGE_TOP : (origin->y > halfh ? WLC_RESIZE_EDGE_BOTTOM : 0));
   }

   wlc_view_set_state(view, WLC_BIT_RESIZING, true);
}

static void
stop_interactive_action(void)
{
   if (!compositor.action.view)
      return;

   wlc_view_set_state(compositor.action.view, WLC_BIT_RESIZING, false);
   memset(&compositor.action, 0, sizeof(compositor.action));
}

static wlc_handle
get_topmost(wlc_handle output, size_t offset)
{
   size_t memb;
   const wlc_handle *views = wlc_output_get_views(output, &memb);
   return (memb > 0 ? views[(memb - 1 + offset) % memb] : 0);
}

static void
relayout(wlc_handle output)
{
   // very simple layout function
   // you probably don't want to layout certain type of windows in wm

   const struct wlc_size *r;
   if (!(r = wlc_output_get_resolution(output)))
      return;
}

static void
output_resolution(wlc_handle output, const struct wlc_size *from, const struct wlc_size *to)
{
   (void)from, (void)to;
   relayout(output);
}

static bool
view_created(wlc_handle view)
{
   wlc_view_bring_to_front(view);
   wlc_view_focus(view);
   relayout(wlc_view_get_output(view));
   return true;
}

static void
view_destroyed(wlc_handle view)
{
   wlc_view_focus(get_topmost(wlc_view_get_output(view), 0));
   relayout(wlc_view_get_output(view));
}

static void
view_focus(wlc_handle view, bool focus)
{
   wlc_view_set_state(view, WLC_BIT_ACTIVATED, focus);
}

static void
view_request_move(wlc_handle view, const struct wlc_point *origin)
{
   start_interactive_move(view, origin);
}

static void
view_request_resize(wlc_handle view, uint32_t edges, const struct wlc_point *origin)
{
   start_interactive_resize(view, edges, origin);
}

static bool
keyboard_key(wlc_handle view, uint32_t time, const struct wlc_modifiers *modifiers, uint32_t key, enum wlc_key_state state)
{
   (void)time, (void)key;
   const uint32_t sym = wlc_keyboard_get_keysym_for_key(key, NULL);

   if (state == WLC_KEY_STATE_PRESSED) {
      if (view) {
         if (modifiers->mods & WLC_BIT_MOD_CTRL && sym == XKB_KEY_q) {
            wlc_view_close(view);
            return true;
         } else if (modifiers->mods & WLC_BIT_MOD_CTRL && sym == XKB_KEY_Down) {
            wlc_view_send_to_back(view);
            wlc_view_focus(get_topmost(wlc_view_get_output(view), 0));
            return true;
         }
      }

      if (modifiers->mods & WLC_BIT_MOD_CTRL && sym == XKB_KEY_Escape) {
         wlc_terminate();
         return true;
      } else if (modifiers->mods & WLC_BIT_MOD_CTRL && sym == XKB_KEY_Return) {
         return true;
      }
   }

   return false;
}

static bool
pointer_button(wlc_handle view, uint32_t time, const struct wlc_modifiers *modifiers, uint32_t button, enum wlc_button_state state, const struct wlc_point *position)
{
   (void)button, (void)time, (void)modifiers;

   if (state == WLC_BUTTON_STATE_PRESSED) {
      wlc_view_focus(view);
      if (view) {
      }
   } else {
      stop_interactive_action();
   }

   return (compositor.action.view ? true : false);
}

static bool
pointer_motion(wlc_handle handle, uint32_t time, const struct wlc_point *position)
{
   (void)handle, (void)time;

   if (compositor.action.view) {
      const int32_t dx = position->x - compositor.action.grab.x;
      const int32_t dy = position->y - compositor.action.grab.y;
      struct wlc_geometry g = *wlc_view_get_geometry(compositor.action.view);

      if (compositor.action.edges) {
         const struct wlc_size min = { 80, 40 };

         struct wlc_geometry n = g;
         if (compositor.action.edges & WLC_RESIZE_EDGE_LEFT) {
            n.size.w -= dx;
            n.origin.x += dx;
         } else if (compositor.action.edges & WLC_RESIZE_EDGE_RIGHT) {
            n.size.w += dx;
         }

         if (compositor.action.edges & WLC_RESIZE_EDGE_TOP) {
            n.size.h -= dy;
            n.origin.y += dy;
         } else if (compositor.action.edges & WLC_RESIZE_EDGE_BOTTOM) {
            n.size.h += dy;
         }

         if (n.size.w >= min.w) {
            g.origin.x = n.origin.x;
            g.size.w = n.size.w;
         }

         if (n.size.h >= min.h) {
            g.origin.y = n.origin.y;
            g.size.h = n.size.h;
         }

         wlc_view_set_geometry(compositor.action.view, compositor.action.edges, &g);
      } else {
         g.origin.x += dx;
         g.origin.y += dy;
         wlc_view_set_geometry(compositor.action.view, 0, &g);
      }

      compositor.action.grab = *position;
   }

   // In order to give the compositor control of the pointer placement it needs
   // to be explicitly set after receiving the motion event:
   wlc_pointer_set_position(position);
   return (compositor.action.view ? true : false);
}

static void
cb_log(enum wlc_log_type type, const char *str)
{
   (void)type;
   printf("%s\n", str);
}

bool output_created(wlc_handle handle) {
    out = (struct wlc_output*)convert_from_wlc_handle(handle, "output");
    return true;
}

bool inited = false;

static void                                                                                                                                                                                                                                                                 
surface_paint_internal(struct ctx *context, struct wlc_surface *surface,                                                                                                                                                                                                    
        const struct wlc_geometry *geometry, struct paint *settings)                                                                                                                                                                                                        
{                                                                                                                                                                                                                                                                           
   assert(context && surface && geometry && settings);                                                                                                                                                                                                                      
                                                                                                                                                                                                                                                                            
//   const struct wlc_geometry *g = geometry;                                                                                                                                                                                                                                 
//                                                                                                                                                                                                                                                                            
//   if (!wlc_size_equals(&surface->size, &geometry->size)) {                                                                                                                                                                                                                 
//      if (wlc_geometry_equals(&settings->visible, geometry)) {                                                                                                                                                                                                              
//         settings->filter = true;                                                                                                                                                                                                                                           
//      } else {                                                                                                                                                                                                                                                              
//         // black borders are requested                                                                                                                                                                                                                                     
//         struct paint settings2 = *settings;                                                                                                                                                                                                                                
//         settings2.program = (settings2.program == PROGRAM_RGBA || settings2.program == PROGRAM_RGB ? settings2.program : PROGRAM_RGB);                                                                                                                                     
//         texture_paint(context, &context->textures[TEXTURE_BLACK], 1, geometry, &settings2);                                                                                                                                                                                
//         g = &settings->visible;                                                                                                                                                                                                                                            
//      }                                                                                                                                                                                                                                                                     
//   }                                                                                                                                                                                                                                                                        
//                                                                                                                                                                                                                                                                            
//
//   texture_paint(context, surface->textures, 3, g, settings);                                                                                                                                                                                                               
   GLuint vao, vbo;

   OpenGL::generateVAOVBO(geometry->origin.x, geometry->origin.y,
           geometry->size.w, geometry->size.h, vao, vbo);

}                                                                                                                                                                                                                                                                           
                                                                                                                                                                                                                                                                            
static void                                                                                                                                                                                                                                                                 
surface_paint(struct ctx *context, struct wlc_surface *surface, const struct wlc_geometry *geometry)                                                                                                                                                                        
{                                                                                                                                                                                                                                                                           
   struct paint settings;                                                                                                                                                                                                                                                   
   memset(&settings, 0, sizeof(settings));                                                                                                                                                                                                                                  
   settings.dim = 1.0f;                                                                                                                                                                                                                                                     
   settings.program = (enum program_type)surface->format;                                                                                                                                                                                                                   
   settings.visible = *geometry;                                                                                                                                                                                                                                            
   surface_paint_internal(context, surface, geometry, &settings);                                                                                                                                                                                                           
}                                                                                                                                                                                                                                                                           
                                                                                                                                                                                                                                                                            
static void                                                                                                                                                                                                                                                                 
view_paint(struct ctx *context, struct wlc_view *view)                                                                                                                                                                                                                      
{                                                                                                                                                                                                                                                                           
   assert(context && view);                                                                                                                                                                                                                                                 
                                                                                                                                                                                                                                                                            
   struct wlc_surface *surface;                                                                                                                                                                                                                                             
   if (!(surface = convert_from_wlc_resource(view->surface, "surface")))                                                                                                                                                                                                    
      return;                                                                                                                                                                                                                                                               
                                                                                                                                                                                                                                                                            
   struct paint settings;                                                                                                                                                                                                                                                   
   memset(&settings, 0, sizeof(settings));                                                                                                                                                                                                                                  
   settings.dim = ((view->commit.state & WLC_BIT_ACTIVATED) || (view->type & WLC_BIT_UNMANAGED) ? 1.0f : DIM);                                                                                                                                                              
   settings.program = (enum program_type)surface->format;                                                                                                                                                                                                                   
                                                                                                                                                                                                                                                                            
   struct wlc_geometry geometry;                                                                                                                                                                                                                                            
   wlc_view_get_bounds(view, &geometry, &settings.visible);                                                                                                                                                                                                                 
   surface_paint_internal(context, surface, &geometry, &settings);                                                                                                                                                                                                          
}     

void clear(struct ctx* c) {
    if(!inited) {
    
        OpenGL::initOpenGL("/usr/local/share/fireman/shaders/");
        inited= true;
    }
}

int main(int argc, char *argv[]) {

    wlc_log_set_handler(cb_log);

    static struct wlc_interface interface;

    interface.clear = clear;
    //interface.backend_init      = opengl_init;
    //interface.output.created    = output_created;
    interface.output.resolution = output_resolution;

//    interface.view.created        = view_created;
    interface.view.destroyed      = view_destroyed;
    interface.view.focus          = view_focus;
    interface.view.request.move   = view_request_move;
    interface.view.request.resize = view_request_resize;

    interface.keyboard.key = keyboard_key;

    interface.pointer.button = pointer_button;
    interface.pointer.motion = pointer_motion;

    if (!wlc_init(&interface, argc, argv))
        return EXIT_FAILURE;

    std::cout << "WLC_INIT" << std::endl;

    setvbuf(stdout, NULL, _IONBF, 0); 
    setvbuf(stderr, NULL, _IONBF, 0);

    wlc_run();
    return EXIT_SUCCESS;
}
}
