#include "../include/wm.hpp"


void Run::init(Core *core) {
    KeyBinding *run = new KeyBinding();
    run->action = [core](Context *ctx){
        core->run(const_cast<char*>("dmenu_run"));
    };

    run->active = true;
    run->mod = Mod1Mask;
    run->type = BindingTypePress;
    run->key = XKeysymToKeycode(core->d, XK_r);

    core->addKey(run, true);
}

void Exit::init(Core *core) {
    KeyBinding *exit = new KeyBinding();
    exit->action = [core](Context *ctx){
        core->terminate = true;
    };

    exit->active = true;
    exit->mod = ControlMask;
    exit->type = BindingTypePress;
    exit->key = XKeysymToKeycode(core->d, XK_q);

    core->addKey(exit, true);
}

void Close::init(Core *core) {
    KeyBinding *close = new KeyBinding();
    close->active = true;
    close->mod = Mod1Mask;
    close->type = BindingTypePress;
    close->key = XKeysymToKeycode(core->d, XK_F4);
    close->action = [core](Context *ctx) {
        auto w = core->getActiveWindow();
        core->closeWindow(w);
        new AnimationHook(new Fade(w, Fade::FadeOut), core);
        //core->destroyWindow(core->getActiveWindow());
    };
    core->addKey(close, true);
}

void Focus::init(Core *core) {
    focus.type = BindingTypePress;
    focus.button = Button1;
    focus.mod = NoMods;
    focus.active = true;

    focus.action = [core] (Context *ctx){
        auto xev = ctx->xev.xbutton;
        auto w =
            core->getWindowAtPoint(Point(xev.x_root, xev.y_root));

        if(w)
            core->focusWindow(w);
    };
    core->addBut(&focus);
}

void RefreshWin::init(Core *core) {
    r.type = BindingTypePress;
    r.key = XKeysymToKeycode(core->d, XK_r);
    r.mod = ControlMask | Mod4Mask;
    r.active = true;
    r.action = [core] (Context *ctx) {
        auto w = core->getActiveWindow();
        if(!w)
            return;
        w->mapTryNum = 100;
        w->norender = false;
    };
}
