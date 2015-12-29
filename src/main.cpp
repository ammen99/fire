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

bool keyboard_key(wlc_handle view, uint32_t time,
        const struct wlc_modifiers *modifiers, uint32_t key,
        enum wlc_key_state state) {

   (void)time, (void)key;
   uint32_t sym = wlc_keyboard_get_keysym_for_key(key, NULL);

   return core->process_key_event(sym, modifiers->mods, state);
}

bool pointer_button(wlc_handle view, uint32_t time,
        const struct wlc_modifiers *modifiers, uint32_t button,
        enum wlc_button_state state, const struct wlc_point *position) {

   (void)button, (void)time, (void)modifiers;

   assert(core);

   return core->process_button_event(button, modifiers->mods,
           state, *position);
}

bool pointer_motion(wlc_handle handle, uint32_t time, const struct wlc_point *position) {
    assert(core);
    return core->process_pointer_motion_event(*position);
}

static void
cb_log(enum wlc_log_type type, const char *str)
{
   (void)type;
   printf("%s\n", str);
}

bool view_created(wlc_handle view) {
    assert(core);
    core->add_window(view);
}

void view_destroyed(wlc_handle view) {
    assert(core);
    core->remove_window(core->find_window(view));
    core->focus_window(core->get_active_window());
}

void view_focus(wlc_handle view, bool focus) {
   wlc_view_set_state(view, WLC_BIT_ACTIVATED, focus);
   wlc_view_bring_to_front(view);
}

int main(int argc, char *argv[]) {

    wlc_log_set_handler(cb_log);

    static struct wlc_interface interface;

    interface.view.created        = view_created;
    interface.view.destroyed      = view_destroyed;
    interface.view.focus          = view_focus;

    interface.keyboard.key = keyboard_key;

    //interface.pointer.button = pointer_button;
    //interface.pointer.motion = pointer_motion;

    if (!wlc_init(&interface, argc, argv))
        return EXIT_FAILURE;

    core = new Core(0, 0);
    core->init();

    wlc_run();
    return EXIT_SUCCESS;
}
}
