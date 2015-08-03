#include "../include/core.hpp"

class VSwitch : public Plugin {
    private:
        KeyBinding kbs[4];
        KeyCode switchWorkspaceBindings[4];

        Hook hook;
        int stepNum;
        int vstep;
        int dirx, diry;
        int dx, dy;
        int nx, ny;
        std::queue<std::tuple<int, int> >dirs; // series of moves we have to do
    public:

    void initOwnership() {
        owner->name = "vswitch";
        owner->compatAll = false;
    }

    void updateConfiguration() {
        vstep = getSteps(options["duration"]->data.ival);
    }

    void beginSwitch() {
        auto tup = dirs.front();
        dirs.pop();

        GetTuple(ddx, ddy, tup);
        GetTuple(vx, vy, core->getWorkspace());
        GetTuple(vw, vh, core->getWorksize());
        GetTuple(sw, sh, core->getScreenSize());

        auto nx = (vx - ddx + vw) % vw;
        auto ny = (vy - ddy + vh) % vh;

        this->nx = nx;
        this->ny = ny;

        dirx = ddx;
        diry = ddy;

        dx = (vx - nx) * sw;
        dy = (vy - ny) * sh;

        int tlx1 = 0;
        int tly1 = 0;
        int brx1 = sw;
        int bry1 = sh;

        int tlx2 = -dx;
        int tly2 = -dy;
        int brx2 = tlx2 + sw;
        int bry2 = tly2 + sh;

        core->setRedrawEverything(true);
        output = core->getRegionFromRect(std::min(tlx1, tlx2),
                std::min(tly1, tly2),
                std::max(brx1, brx2),
                std::max(bry1, bry2));

        core->activateOwner(owner);
        stepNum = 0;
    }

#define MAXDIRS 6
    void insertNextDirection(int ddx, int ddy) {
        if(!hook.getState())
            hook.enable(),
            dirs.push(std::make_tuple(ddx, ddy)),
            beginSwitch();
        else if(dirs.size() < MAXDIRS)
            dirs.push(std::make_tuple(ddx, ddy));
    }

    void handleKey(Context *ctx) {
        if(!ctx)
            return;

        auto xev = ctx->xev.xkey;

        if(xev.keycode == switchWorkspaceBindings[0])
            insertNextDirection(1,  0);
        if(xev.keycode == switchWorkspaceBindings[1])
            insertNextDirection(-1, 0);
        if(xev.keycode == switchWorkspaceBindings[2])
            insertNextDirection(0, -1);
        if(xev.keycode == switchWorkspaceBindings[3])
            insertNextDirection(0,  1);
    }

    void Step() {
        GetTuple(w, h, core->getScreenSize());

        if(stepNum == vstep){
            Transform::gtrs = glm::mat4();
            core->switchWorkspace(std::make_tuple(nx, ny));
            core->setRedrawEverything(false);

            if(dirs.size() == 0) {
                hook.disable();
                core->deactivateOwner(owner);
            }
            else
                beginSwitch();
            return;
        }
        float progress = float(stepNum++) / float(vstep);

        float offx =  2.f * progress * float(dx) / float(w);
        float offy = -2.f * progress * float(dy) / float(h);

        if(!dirx)
            offx = 0;
        if(!diry)
            offy = 0;

        Transform::gtrs = glm::translate(glm::mat4(), glm::vec3(offx, offy, 0.0));
    }

    void init() {
        using namespace std::placeholders;

        options.insert(newIntOption("duration", 500));

        switchWorkspaceBindings[0] = XKeysymToKeycode(core->d, XK_h);
        switchWorkspaceBindings[1] = XKeysymToKeycode(core->d, XK_l);
        switchWorkspaceBindings[2] = XKeysymToKeycode(core->d, XK_j);
        switchWorkspaceBindings[3] = XKeysymToKeycode(core->d, XK_k);

        for(int i = 0; i < 4; i++) {
            kbs[i].type = BindingTypePress;
            kbs[i].mod = ControlMask | Mod1Mask;
            kbs[i].key = switchWorkspaceBindings[i];
            kbs[i].action = std::bind(std::mem_fn(&VSwitch::handleKey), this, _1);
            core->addKey(&kbs[i], true);
        }

        hook.action = std::bind(std::mem_fn(&VSwitch::Step), this);
        core->addHook(&hook);
    }
};
extern "C" {
    Plugin *newInstance() {
        return new VSwitch();
    }
}
