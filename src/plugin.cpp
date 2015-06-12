#include "../include/core.hpp"

void _Ownership::grab() {
    if(this->grabbed || !this->active)
        return;

    this->grabbed = true;
    XGrabPointer(core->d, core->overlay, TRUE,
            ButtonPressMask | ButtonReleaseMask |
            PointerMotionMask,
            GrabModeAsync, GrabModeAsync,
            core->root, None, CurrentTime);

    XGrabKeyboard(core->d, core->overlay, True,
            GrabModeAsync, GrabModeAsync, CurrentTime);
}

void _Ownership::ungrab() {
    if(!grabbed || !active)
        return;

    XUngrabPointer(core->d, CurrentTime);
    XUngrabKeyboard(core->d, CurrentTime);
}

void Plugin::initOwnership() {
    owner->name = "Unknown";
    owner->compatAll = true;
}
