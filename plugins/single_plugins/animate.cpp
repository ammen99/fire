#include <core.hpp>

class Animation {
    public:
    virtual bool Step(){return false;}// return true if continue, false otherwise
    virtual bool Run() {
        /* should we start? */
        return true;
    }
    virtual ~Animation() {}
};

class AnimationHook {
    Hook hook;
    public:
    Animation *anim;
    AnimationHook(Animation *_anim) {
        this->anim = _anim;
        if(anim->Run()) {
            this->hook.action =
                std::bind(std::mem_fn(&AnimationHook::step), this);
            core->addHook(&hook);
            this->hook.enable();
        }
    }

    void step() {
        if(!this->anim->Step()) {
            core->remHook(hook.id);
            delete anim;
        }
    }
};

struct FadeWindowData : public WindowData {
    bool fadeIn = false; // used to prevent activation
    bool fadeOut = false; // of fading multiple times
};

class Fade : public Animation {

    public:
    static int duration;
    enum Mode { FadeIn = 1, FadeOut = -1 };

    private:
    FireWindow win;
    Mode mode;
    int progress = 0;
    int maxstep = 0;
    int target = 0;
    bool run = true;
    bool restoretr = true;
    bool savetr; // used to restore transparency

    public:
    Fade (FireWindow _win, Mode _mode) :
        win(_win), mode(_mode) {
            run = true;

            if(ExistsData(win, "fade")) {
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

    bool Step() {
        progress += mode;
        win->transform.color[3] = (float(progress) / float(maxstep));
        win->addDamage();

        if(progress == target) {
            if(restoretr)
                win->transparent = savetr;

            if(mode == FadeIn)
                GetData(FadeWindowData, win, "fade")->fadeIn = false;
            if(mode == FadeOut)
                GetData(FadeWindowData, win, "fade")->fadeOut= false;

            win->transparent = savetr;
            win->keepCount--;

            if(mode == FadeOut && !win->keepCount) {
                // just unmap
                win->norender = true,
                    XFreePixmap(core->d, win->pixmap);
                core->focusWindow(core->getActiveWindow());

                if(win->destroyed) // window is closed
                    core->removeWindow(win);
            }
            return false;
        }
        return true;
    }

    bool Run() { return this->run; }
    ~Fade() {}
};
int Fade::duration;


class AnimatePlugin : public Plugin {
    std::vector<AnimationHook*> animations;
    SignalListener map, unmap;

    public:
        void initOwnership() {
            owner->name = "animate";
            owner->compatAll = true;
        }

        void updateConfiguration() {
            Fade::duration = options["duration"]->data.ival;
        }

        void init() {
            options.insert(newIntOption("duration", 150));
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
            animations.push_back(new AnimationHook(new Fade(win,
                            Fade::FadeIn)));
        }

        void unmapWindow(SignalListenerData data) {
            auto win = *((FireWindow*) data[0]);
            animations.push_back(new AnimationHook(new Fade(win,
                            Fade::FadeOut)));
        }

        void fini() {
            animations.clear();
            core->disconnectSignal("map-window", map.id);
            core->disconnectSignal("unmap-window", unmap.id);
        }
};

extern "C" {
    Plugin *newInstance() {
        return new AnimatePlugin();
    }
}
