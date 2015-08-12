#include "fade.hpp"

bool Animation::Step() {return false;}
bool Animation::Run() {return true;}
Animation::~Animation() {}

AnimationHook::AnimationHook(Animation *_anim) {
    std::cout << "new animation" << std::endl;
    this->anim = _anim;
    if(anim->Run()) {
        this->hook.action =
            std::bind(std::mem_fn(&AnimationHook::Step), this);
        core->addHook(&hook);
        this->hook.enable();
        std::cout << "Creating hook nr " << hook.id << std::endl;;
    }
    else {
        std::cout << "Deleting" << std::endl;
        delete anim;
        delete this;
    }
}

void AnimationHook::Step() {
    if(!this->anim->Step()) {
        core->remHook(hook.id);
        std::cout << "REMOVED HOOK" << std::endl;
        delete anim;
        std::cout << "REMOVED ANIM" << std::endl;
        delete this;
        std::cout << "REMOVED THIS" << std::endl;
    }
}

class AnimatePlugin : public Plugin {
    SignalListener map, unmap;

    public:
        void initOwnership() {
            owner->name = "animate";
            owner->compatAll = true;
        }

        void updateConfiguration() {
            fadeDuration = options["fade_duration"]->data.ival;
        }

        void init() {
            options.insert(newIntOption("fade_duration", 150));
            using namespace std::placeholders;
            map.action = std::bind(std::mem_fn(&AnimatePlugin::mapWindow),
                        this, _1);
            unmap.action = std::bind(std::mem_fn(&AnimatePlugin::unmapWindow),
                        this, _1);

            core->connectSignal("map-window", &map);
            core->connectSignal("unmap-window", &unmap);
        }

        void mapWindow(SignalListenerData data) {
            auto win = *((FireWindow*) data[0]);
            std::cout << "Creating animation for window " << win->id << std::endl;
            new AnimationHook(new Fade<FadeIn>(win));
        }

        void unmapWindow(SignalListenerData data) {
            auto win = *((FireWindow*) data[0]);
            std::cout << "Createing fade out for window " << win->id << std::endl;
            new AnimationHook(new Fade<FadeOut>(win));
        }

        void fini() {
            core->disconnectSignal("map-window", map.id);
            core->disconnectSignal("unmap-window", unmap.id);
        }
};

extern "C" {
    Plugin *newInstance() {
        return new AnimatePlugin();
    }
}
