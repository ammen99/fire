#include "../include/core.hpp"

int Fade::duration;

Animation::~Animation() {}

AnimationHook::AnimationHook(Animation *_anim, Core *c) {
    this->anim = _anim;
    this->hook.action = std::bind(std::mem_fn(&AnimationHook::step), this);
    c->addHook(&hook);
    this->hook.enable();
}

void AnimationHook::step() {
    if(!this->anim->Step()) {
        delete this->anim;
        this->hook.disable();
       // delete this; // TODO: add deletion of used hooks
    }
}

AnimationHook::~AnimationHook() {}

Fade::Fade (FireWindow _win, Mode _mode) :
    win(_win), mode(_mode) {
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
    core->damageWindow(win);

    if(progress == target) {
        win->transparent = savetr;

        win->keepCount--;
        if(mode == FadeOut)      // just unmap
            win->norender = true;

        if(mode == FadeOut && win->destroyed) // window is closed
            core->removeWindow(win);

        return false;
    }
    return true;
}

