#include "../include/core.hpp"

Animation::~Animation() {}

AnimationHook::AnimationHook(Animation *_anim, Core *c) {
    this->anim = _anim;
    this->hook.action = std::bind(std::mem_fn(&AnimationHook::step), this);
    c->addHook(&hook);
    this->hook.enable();
}

void AnimationHook::step() {
    if(!this->anim->Step()) {
        std::cout << "Closing everything" << std::endl;
        delete this->anim;
        this->hook.disable();
       // delete this; // TODO: add deletion of used hooks
    }
}

AnimationHook::~AnimationHook() {}

Fade::Fade (FireWindow _win, Mode _mode, int _steps, bool _destroy) :
    win(_win), mode(_mode), maxstep(_steps), destroy(_destroy) {
        if(mode == FadeIn)
            this->progress = 0,
            this->target = maxstep;
        else
            this->progress = maxstep,
            this->target = 0;
    }

bool Fade::Step() {
    progress += mode;
    win->opacity = (float(progress) / float(maxstep)) * 0xffff;
    core->damageWindow(win);

    if(progress == target) {
        if(mode == FadeOut && !destroy)
            win->norender = true;

        if(mode == FadeOut && destroy) {
            core->closeWindow(win);
            core->removeWindow(win);
        }
        return false;
    }
    return true;
}

