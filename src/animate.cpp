#include "../include/core.hpp"

int Fade::duration;

Animation::~Animation() {}
bool Animation::Run() {
    /* should we start? */
    return true;
}

AnimationHook::AnimationHook(Animation *_anim, Core *c) {
    this->anim = _anim;
    if(anim->Run()) {
        this->hook.action =
            std::bind(std::mem_fn(&AnimationHook::step), this);
        c->addHook(&hook);
        this->hook.enable();
    }
}


void AnimationHook::step() {
    if(!this->anim->Step()) {
        delete this->anim;
        this->hook.disable();
        std::cout << "disabled hook" << std::endl;
       // delete this; // TODO: add deletion of used hooks
    }
}

AnimationHook::~AnimationHook() {}

struct FadeWindowData : public WindowData {
    bool fadeIn = false; // used to prevent activation
    bool fadeOut = false; // of fading multiple times
};

Fade::Fade (FireWindow _win, Mode _mode) :
    win(_win), mode(_mode) {
        run = true;

        if(ExistsData(win, "fade")) {
            std::cout << "yep, data exists" << std::endl;
            auto data = GetData(FadeWindowData, win, "fade");
            if(mode == FadeIn && data->fadeIn)
                run = false;
            if(mode == FadeOut && data->fadeOut)
                run = false;
        }
        else AllocData(FadeWindowData, win, "fade");

        if(!run) return;

        if(mode == FadeIn)
            GetData(FadeWindowData, win, "fade")->fadeIn = true;
        if(mode == FadeOut)
            GetData(FadeWindowData, win, "fade")->fadeOut= true;

        if(mode == FadeOut && GetData(FadeWindowData, win, "fade")->fadeIn)
            restoretr = false;

        savetr = win->transparent;
        win->transparent = true;

        maxstep = getSteps(duration);
        win->keepCount++;

        if(mode == FadeIn)
            this->progress = 0,
            this->target = maxstep;
        else
            this->progress = maxstep,
            this->target = 0;
    }

bool Fade::Step() {
    progress += mode;
    win->transform.color[3] = (float(progress) / float(maxstep));
    std::cout << win->transform.color[3] << std::endl;
    core->damageWindow(win);

    if(progress == target) {
        std::cout << "ending" << std::endl;
        if(restoretr)
            win->transparent = savetr;

        if(mode == FadeIn)
            GetData(FadeWindowData, win, "fade")->fadeIn = false;
        if(mode == FadeOut)
            GetData(FadeWindowData, win, "fade")->fadeOut= false;

        win->transparent = savetr;
        win->keepCount--;
        std::cout << "midway" << std::endl;
        if(mode == FadeOut)      // just unmap
            XFreePixmap(core->d, win->pixmap),
            win->norender = true;

        if(mode == FadeOut && win->destroyed) // window is closed
            core->removeWindow(win);
        std::cout << "returning false" << std::endl;

        return false;
    }
    return true;
}

bool Fade::Run() {
    return this->run;
}

