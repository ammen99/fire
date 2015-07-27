#include "../include/core.hpp"
#include "../include/opengl.hpp"


float getFactor(int x, int y, float percent) {
    GetTuple(width, height, core->getScreenSize());

    float d = std::sqrt(x * x + y * y);
    float d1 = std::sqrt(width * width + height *height);
    if(d != 0)
        return percent * d1 / d;
    else
        return 0;
}

void getOffsetStepForWindow(FireWindow win, float c,
        float &offX, float &offY) {
    GetTuple(sw, sh, core->getScreenSize());

    int newposX = sw / 2 - win->attrib.width / (2.f / c);
    int newposY = sh / 2 - win->attrib.height/ (2.f / c);

    offX =  2 * float(win->attrib.x - newposX) / float(sw);
    offY = -2 * float(win->attrib.y - newposY) / float(sh);
}

struct WinAttrib {
    FireWindow win;
    bool tr = false;
    float rotateAngle;
    float offX;
    float offY;
    float offZ;
    bool scale = false;
    float scaleStart = -1;
    float scaleEnd = -1;
};

class ATSwitcher : public Plugin {

    KeyBinding initiate;
    KeyBinding forward;
    KeyBinding backward;
    KeyBinding terminate;
    std::vector<FireWindow> windows;

#define MAXDIRS 10
    std::queue<int> dirs;

    FireWindow background;
    bool active, block = false; // block is waiting to exit
    int index;

    Hook rnd; // used to render windows
    Hook ini;
    Hook exit;

    int initsteps;
    int steps = 20;
    int curstep = 0;
    /* windows that should be animated in step() */
    std::vector<WinAttrib> winsToMove;

    struct {
        float offset = 0.6f;
        float angle = M_PI / 6.;
        float back = 0.3f;
    } attribs;


    public:

    void initOwnership() {
        owner->name = "switcher";
        owner->compatAll = false;
    }

    void updateConfiguration() {
        steps = getSteps(options["duration"]->data.ival);
        initsteps = getSteps(options["init"]->data.ival);
    }

    void init() {

        options.insert(newIntOption("duration", 1000));
        options.insert(newIntOption("init", 1000));

        using namespace std::placeholders;

        active = false;

        initiate.mod = Mod1Mask;
        initiate.key = XKeysymToKeycode(core->d, XK_Tab);
        initiate.type = BindingTypePress;
        initiate.action =
            std::bind(std::mem_fn(&ATSwitcher::handleKey), this, _1);
        core->addKey(&initiate, true);

        forward.mod = 0;
        forward.key = XKeysymToKeycode(core->d, XK_Right);
        forward.type = BindingTypePress;
        forward.action = initiate.action;
        core->addKey(&forward, false);

        backward.mod = 0;
        backward.key = XKeysymToKeycode(core->d, XK_Left);
        backward.type = BindingTypePress;
        backward.action = initiate.action;
        core->addKey(&backward, false);

        terminate.mod = 0;
        terminate.key = XKeysymToKeycode(core->d, XK_Return);
        terminate.type = BindingTypePress;
        terminate.action = initiate.action;
        core->addKey(&terminate, false);

        rnd.action = std::bind(std::mem_fn(&ATSwitcher::step), this);
        core->addHook(&rnd);

        ini.action = std::bind(std::mem_fn(&ATSwitcher::initHook), this);
        core->addHook(&ini);

        exit.action = std::bind(std::mem_fn(&ATSwitcher::exitHook), this);
        core->addHook(&exit);
    }

    void handleKey(Context *ctx) {
        auto xev = ctx->xev.xkey;

        if(xev.keycode == XKeysymToKeycode(core->d, XK_Tab)) {
            if(active) {
                if(ini.getState() || rnd.getState()) {
                    if(!block)
                        dirs.push(0), block = true;
                } else
                    Terminate();
            }
            else {
                if(ini.getState() || rnd.getState() || exit.getState())
                    return;
                else
                    Initiate();
            }
        }

        if(xev.keycode == XKeysymToKeycode(core->d, XK_Left)) {
            if(rnd.getState() || ini.getState())
                dirs.push(1);
            else
                moveLeft();
        }

        if(xev.keycode == XKeysymToKeycode(core->d, XK_Right)) {
            if(rnd.getState() || ini.getState())
                dirs.push(-1);
            else
                moveRight();
        }

        if(xev.keycode == XKeysymToKeycode(core->d, XK_Return))
            if(active) {
                if(ini.getState() || rnd.getState()) {
                    if(!block)
                        dirs.push(0), block = true;
                } else
                    Terminate();
            }
    }

    void Initiate() {

        if(!core->activateOwner(owner))
            return;

        owner->grab();

        windows.clear();
        windows = core->getWindowsOnViewport(core->getWorkspace());
        std::cout << "Switcher initiated with " << windows.size() <<
            " windows" << std::endl;

        if(windows.size() == 1) {
            core->deactivateOwner(owner);
            return;
        }

        active = true;

        background = nullptr;
        auto it = windows.begin();
        while(it != windows.end()) { // find background window
            if((*it)->type == WindowTypeDesktop) {
                background = (*it),
                windows.erase(it);
                break;
            }
            ++it;
        }

        for(auto w : windows) {

            std::cout << "Switcher got winID = " << w->id << std::endl;
            WinAttrib wia;
            wia.win = w;

            float offx, offy;
            getOffsetStepForWindow(w, 1, offx, offy);
            wia.offX = -offx;
            wia.offY = -offy;
            winsToMove.push_back(wia);

            w->transform.translation = glm::translate(
                    glm::mat4(), glm::vec3(offx, offy, 0));
            w->disableVBOChange = true;
            OpenGL::generateVAOVBO(w->attrib.width,
                    w->attrib.height, w->vao, w->vbo);
        }
////
        OpenGL::transformed = true;
        if(background) {
            background->transform.translation = glm::translate(glm::mat4(),
                    glm::vec3(0.f, 0.f, -1.0f));
            background->transform.scalation   = glm::scale(glm::mat4(),
                    glm::vec3(1.51f, 1.51f, 1.f));
        }

        backward.enable(); forward.enable(); terminate.enable();

        attribs.offset = 0.6f;
        attribs.angle = M_PI / 6.;
        attribs.back = 0.3f;

        if(windows.size() == 2)
            attribs.offset = 0.4f,
            attribs.angle = M_PI / 5.,
            attribs.back = 0.;

        index = 0;
        core->setRedrawEverything(true);
        curstep = 0;
        ini.enable();
    }

    void initHook() {

        auto c = (.5f * float(curstep) +
                float(initsteps - curstep)) / float(initsteps);
        if(background)
            background->transform.color = glm::vec4(c, c, c, 1.0);

        for(auto attrib : winsToMove) {
            attrib.win->transform.translation =
                glm::translate(attrib.win->transform.translation,
                        glm::vec3(attrib.offX / float(initsteps),
                                  attrib.offY / float(initsteps), 0));

            attrib.win->transform.scalation =
                glm::scale(glm::mat4(), glm::vec3(c, c, 1));
        }

        curstep++;
        if(curstep == initsteps) {
            winsToMove.clear();
            for(auto w : this->windows) {
                w->norender = true;
            }
            ini.disable();
            rnd.enable();
            curstep = 0;

            auto size = windows.size();
            if(size < 1)
                return;

            auto prev = (index + size - 1) % size;
            auto next = (index + size + 1) % size;

            windows[index]->norender = false;
            windows[next ]->norender = false;
            windows[prev ]->norender = false;

            WinAttrib prv;
            prv.win = windows[prev];
            prv.offX = -attribs.offset;
            prv.offZ = -attribs.back;
            prv.rotateAngle = attribs.angle;

            prv.scale = true;
            prv.scaleStart = 0.5;
            prv.scaleEnd = getFactor(windows[prev]->attrib.width,
                    windows[prev]->attrib.height, 0.5);
            winsToMove.push_back(prv);

            if(prev == next)
                next = index;
            else {
                WinAttrib ind;
                ind.win = windows[index];

                ind.rotateAngle = 0;
                ind.offX = 0;
                ind.offZ = 0;

                ind.scale = true;
                ind.scaleStart = 0.5;
                ind.scaleEnd = getFactor(windows[index]->attrib.width,
                        windows[index]->attrib.height,
                        0.5);

                winsToMove.push_back(ind);
            }

            WinAttrib nxt;
            nxt.win = windows[next];
            nxt.offX = attribs.offset;
            nxt.offZ = -attribs.back;
            nxt.rotateAngle = -attribs.angle;

            nxt.scale = true;
            nxt.scaleStart = 0.5;
            nxt.scaleEnd = getFactor(windows[next]->attrib.width,
                    windows[next]->attrib.height, 0.5);

            winsToMove.push_back(nxt);
            for(int i = 0; i < windows.size(); i++) {
                if(i != next && i != prev && i != index) {
                    auto c = getFactor(windows[i]->attrib.width,
                                  windows[i]->attrib.height, 0.5);
                    windows[i]->transform.scalation =
                        glm::scale(glm::mat4(), glm::vec3(c, c, 1));
                }
            }
        }
    }

    void exitHook() {
        for(auto attr : winsToMove) {
            if(attr.scale) {
                auto c = (attr.scaleEnd * float(curstep) +
                        attr.scaleStart * float(initsteps - curstep))
                    / float(initsteps);
                attr.win->transform.scalation =
                    glm::scale(glm::mat4(), glm::vec3(c, c, 1));
            }

            attr.win->transform.translation = glm::translate(
                    attr.win->transform.translation,
                    glm::vec3(attr.offX / initsteps,
                              attr.offY / initsteps,
                              attr.offZ / initsteps));

            attr.win->transform.rotation = glm::rotate(
                    attr.win->transform.rotation,
                    attr.rotateAngle / initsteps,
                    glm::vec3(0, 1, 0));
        }
        curstep++;
        if(curstep == initsteps) {
            exit.disable();
            block = false;
            for(auto w : windows)
                w->norender = false,
                w->transform.scalation   =
                w->transform.translation =
                w->transform.rotation    = glm::mat4(),
                w->transform.color[3] = 1,
                w->disableVBOChange = false,
                OpenGL::generateVAOVBO(w->attrib.x,
                        w->attrib.y,
                        w->attrib.width,
                        w->attrib.height,
                        w->vao, w->vbo);

            core->setRedrawEverything(false);
            core->dmg = core->getMaximisedRegion();

            if(background)
                background->transform.color = glm::vec4(1, 1, 1, 1),
                background->transform.translation = glm::mat4(),
                background->transform.scalation   = glm::mat4();

            OpenGL::transformed = false;
            winsToMove.clear();
        }
    }


    void step() {
        for(auto attr : winsToMove) {
            attr.win->transform.translation =
                glm::translate(attr.win->transform.translation,
                glm::vec3(attr.offX / float(steps), 0.f,
                          attr.offZ / float(steps)));

            attr.win->transform.rotation =
                glm::rotate(attr.win->transform.rotation,
                        attr.rotateAngle / float(steps),
                        glm::vec3(0, 1, 0));

            if(attr.scale) {
                auto c = (float(curstep) * attr.scaleEnd +
                        float(steps - curstep) * attr.scaleStart)
                    /float(steps);

                attr.win->transform.scalation =
                    glm::scale(glm::mat4(), glm::vec3(c, c, 1));
            }
        }
        curstep++;
        if(curstep == steps) {
            rnd.disable();
            winsToMove.clear();
            if(!dirs.empty()) {
                auto ndir = dirs.front();
                dirs.pop();
                if(ndir == 1)
                    moveLeft();
                else if(ndir == -1)
                    moveRight();
                else
                    Terminate();
            }
        }
    }

    void setTransform(int dir) {
        rnd.enable();
        curstep = 0;

        auto size = windows.size();
        if(size <= 1)
            return;

        auto prev = (index + size - 1) % size;
        auto next = (index + size + 1) % size;

        windows[index]->norender = false;
        windows[next ]->norender = false;
        windows[prev ]->norender = false;

        float factor = 1;

        if(prev == next)
            factor = 2;

        WinAttrib prv;
        prv.win = windows[prev];
        prv.offX = -attribs.offset * factor;
        prv.offZ = -attribs.back;
        prv.rotateAngle = attribs.angle * factor;

        winsToMove.push_back(prv);

        if(prev == next)
            next = index;
        else {
            WinAttrib ind;
            ind.win = windows[index];

            ind.rotateAngle = attribs.angle * dir;
            ind.offX = -attribs.offset * dir;
            ind.offZ = attribs.back;

            winsToMove.push_back(ind);
        }

        WinAttrib nxt;
        nxt.win = windows[next];
        nxt.offX = attribs.offset * factor;
        nxt.offZ = -attribs.back;
        nxt.rotateAngle = -attribs.angle * factor;
        winsToMove.push_back(nxt);

        core->focusWindow(windows[index]);

        if(!active) {
        }
    }

    void reset(int dir) {
        auto size = windows.size();
        auto prev = (index + size - 1) % size;
        auto next = (index + size + 1) % size;

        windows[index]->norender = true;
        windows[next ]->norender = true;
        windows[prev ]->norender = true;

        if(prev == next)
            return;

        int clear;
        if(dir == 1)
            clear = prev;
        else
            clear = next;

        windows[clear]->transform.translation =
        windows[clear]->transform.rotation    = glm::mat4();

    }

    void moveRight() {
        if(windows.size() == 1)
            return;

        reset(-1);
        index = (index - 1 + windows.size()) % windows.size();
        setTransform(-1);
    }

    void moveLeft() {
        if(windows.size() == 1)
            return;

        reset(1);
        index = (index + 1) % windows.size();
        setTransform(1);
    }

    void Terminate() {

        core->deactivateOwner(owner);
        owner->ungrab();

        active = false;
        backward.disable(); forward.disable(); terminate.disable();

        curstep = 0;
        exit.enable();

        auto size = windows.size();

        WinAttrib ind;
        ind.win = windows[index];
        ind.scale = true;
        ind.scaleStart = getFactor(ind.win->attrib.width,
                ind.win->attrib.height, 0.5);
        ind.scaleEnd = 1;

        float offx, offy;
        getOffsetStepForWindow(ind.win, 1, offx, offy);
        ind.offX = offx;
        ind.offY = offy;
        ind.offZ = 0;
        ind.rotateAngle = 0;
        if(size == 1) {
            winsToMove.push_back(ind);
            return;
        }

        auto prev = (index + size - 1) % size;
        auto next = (index + size + 1) % size;

        for(int i = 0; i < size; i++) {
            WinAttrib wia;
            wia.win = windows[i];

            if(i != prev && i != next && i != index) {
                windows[i]->transform.translation =
                windows[i]->transform.scalation =
                windows[i]->transform.rotation = glm::mat4();
                windows[i]->updateVBO();

                wia.tr = true;
                windows[i]->transform.color[3] = 0;
                wia.offX = 0;
                wia.offY = 0;
                wia.offZ = 0;
                winsToMove.push_back(wia);

                continue;
            }

            float offx, offy;
            getOffsetStepForWindow(windows[i], 1, offx, offy);

            if(i == prev)
                wia.rotateAngle = -attribs.angle,
                wia.offX = attribs.offset + offx,
                wia.offY = offy,
                wia.offZ = attribs.back;

            if(i == index) {
                if(prev != next)
                    winsToMove.push_back(ind);
                else
                    next = index;
            }

            if(i == next)
                wia.rotateAngle = attribs.angle,
                wia.offX = -attribs.offset + offx,
                wia.offY = offy,
                wia.offZ = attribs.back;
            winsToMove.push_back(wia);
        }
    }
};

extern "C" {
    Plugin *newInstance() {
        return new ATSwitcher();
    }
}
