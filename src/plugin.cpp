#include "../include/core.hpp"

namespace {
    int grabCount = 0;
}

void _Ownership::grab() {
    if(this->grabbed || !this->active)
        return;

    this->grabbed = true;
    grabCount++;

    if(grabCount == 1) {
        XGrabPointer(core->d, core->overlay, TRUE,
                ButtonPressMask | ButtonReleaseMask |
                PointerMotionMask,
                GrabModeAsync, GrabModeAsync,
                core->root, None, CurrentTime);

        XGrabKeyboard(core->d, core->overlay, True,
                GrabModeAsync, GrabModeAsync, CurrentTime);
    }
}

void _Ownership::ungrab() {
    if(!grabbed || !active)
        return;

    grabbed = false;
    grabCount--;
    if(grabCount == 0) {
        XUngrabPointer(core->d, CurrentTime);
        XUngrabKeyboard(core->d, CurrentTime);
    }

    if(grabCount < 0)
        grabCount = 0;
}

void Plugin::initOwnership() {
    owner->name = "Unknown";
    owner->compatAll = true;
}
