#include "../include/wm.hpp"


void Run::init() {
    this->owner->special = true;
    KeyBinding *run = new KeyBinding();
    run->action = [](Context *ctx){
        core->run(const_cast<char*>("dmenu_run"));
    };

    run->mod = Mod1Mask;
    run->type = BindingTypePress;
    run->key = XKeysymToKeycode(core->d, XK_r);
    core->addKey(run, true);
}

void Exit::init() {
    this->owner->special = true;
    KeyBinding *exit = new KeyBinding();
    exit->action = [](Context *ctx){
        core->terminate = true;
    };

    exit->mod = ControlMask | Mod1Mask;
    exit->type = BindingTypePress;
    exit->key = XKeysymToKeycode(core->d, XK_q);
    core->addKey(exit, true);
}

void Refresh::init() {
    ref.key = XKeysymToKeycode(core->d, XK_r);
    ref.type = BindingTypePress;
    ref.mod = ControlMask | Mod1Mask;
    ref.action = [] (Context *ctx) {
        core->terminate = true;
        core->mainrestart = true;
    };
    core->addKey(&ref, true);
}

void Close::init() {
    KeyBinding *close = new KeyBinding();
    close->mod = Mod1Mask;
    close->type = BindingTypePress;
    close->key = XKeysymToKeycode(core->d, XK_F4);
    close->action = [](Context *ctx) {
        auto w = core->getActiveWindow();
        core->closeWindow(w);
        new AnimationHook(new Fade(w, Fade::FadeOut), core);
    };
    core->addKey(close, true);
}

void Focus::init() {
    this->owner->special = true;
    focus.type = BindingTypePress;
    focus.button = Button1;
    focus.mod = 0;

    focus.action = [] (Context *ctx){
        auto xev = ctx->xev.xbutton;
        auto w = core->getWindowAtPoint(xev.x_root, xev.y_root);
        if(w) core->focusWindow(w);
    };
    core->addBut(&focus, true);
}
