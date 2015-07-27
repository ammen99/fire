#include "../include/core.hpp"

class Expo : public Plugin {
    private:
        KeyBinding toggle;
        ButtonBinding press, release;

        int stepNum;
        int expostep;

        float offXtarget, offYtarget;
        float offXcurrent, offYcurrent;
        float sclXtarget, sclYtarget;
        float sclXcurrent, sclYcurrent;

        float stepoffX, stepoffY, stepsclX, stepsclY;

        Hook hook;
        bool active;
        std::function<FireWindow(int, int)> save; // used to restore
    public:

    void updateConfiguration() {
        expostep = getSteps(options["duration"]->data.ival);
    }

    void init() {
        options.insert(newIntOption("duration", 1000));

        using namespace std::placeholders;
        toggle.key = XKeysymToKeycode(core->d, XK_e);
        toggle.mod = Mod4Mask;
        toggle.active = true;
        toggle.action =
            std::bind(std::mem_fn(&Expo::Toggle), this, _1);

        core->addKey(&toggle, true);

        active = false;

        release.active = false;
        release.action =
            std::bind(std::mem_fn(&Expo::buttonRelease), this, _1);
        release.type = BindingTypeRelease;
        release.mod = AnyModifier;
        release.button = Button1;
        core->addBut(&release);

        press.active = false;
        press.action =
            std::bind(std::mem_fn(&Expo::buttonPress), this, _1);
        press.type = BindingTypePress;
        press.mod = Mod4Mask;
        press.button = Button1;
        core->addBut(&press);

        hook.action = std::bind(std::mem_fn(&Expo::zoom), this);
        core->addHook(&hook);
    }
    void initOwnership() {
        owner->name = "expo";
        owner->compatAll = false;
        owner->compat.insert("move");
    }

    void buttonRelease(Context *ctx) {

        GetTuple(w, h, core->getScreenSize());
        GetTuple(vw, vh, core->getWorksize());

        auto xev = ctx->xev.xbutton;

        int vpw = w / vw;
        int vph = h / vh;

        int vx = xev.x_root / vpw;
        int vy = xev.y_root / vph;

        core->switchWorkspace(std::make_tuple(vx, vy));

        recalc();
        finalizeZoom();
    }

    void buttonPress(Context *ctx) {
        buttonRelease(ctx);
        Toggle(ctx);
    }

    void recalc() {

        GetTuple(vx, vy, core->getWorkspace());
        GetTuple(vwidth, vheight, core->getWorksize());

        float midx = vwidth / 2;
        float midy = vheight / 2;

        float offX = float(vx - midx) * 2.f / float(vwidth );
        float offY = float(midy - vy) * 2.f / float(vheight);

        if(vwidth % 2 == 0)
            offX += 1.f / float(vwidth);
        if(vheight % 2 == 0)
            offY += -1.f / float(vheight);

        float scaleX = 1.f / float(vwidth);
        float scaleY = 1.f / float(vheight);
        core->scaleX = vwidth;
        core->scaleY = vheight;

        offXtarget = offX;
        offYtarget = offY;
        sclXtarget = scaleX;
        sclYtarget = scaleY;
    }

    void finalizeZoom() {
        Transform::gtrs = glm::translate(glm::mat4(),
                glm::vec3(offXtarget, offYtarget, 0.f));
        Transform::gscl = glm::scale(glm::mat4(),
                glm::vec3(sclXtarget, sclYtarget, 0.f));
    }

    void Toggle(Context *ctx) {
        using namespace std::placeholders;

        if(!active) {
            active = !active;

            press.active = true;
            release.active = true;

            if(!core->activateOwner(owner))
                return;

            owner->grab();

            save = core->getWindowAtPoint;
            core->getWindowAtPoint =
                std::bind(std::mem_fn(&Expo::findWindow), this, _1, _2);

            hook.enable();

            core->setRedrawEverything(true);
            stepNum = expostep;
            recalc();

            offXcurrent = 0;
            offYcurrent = 0;
            sclXcurrent = 1;
            sclYcurrent = 1;
        }else {
            active = !active;

            press.active = false;
            release.active = false;

            core->deactivateOwner(owner);

            core->scaleX = 1;
            core->scaleY = 1;

            hook.enable();
            stepNum = expostep;

            sclXcurrent = sclXtarget;
            sclYcurrent = sclYtarget;
            offXcurrent = offXtarget;
            offYcurrent = offYtarget;

            sclXtarget = 1;
            sclYtarget = 1;
            offXtarget = 0;
            offYtarget = 0;
        }
    }

    void zoom() {

        if(stepNum == expostep) {
            stepoffX = (offXtarget - offXcurrent) / float(expostep);
            stepoffY = (offYtarget - offYcurrent) / float(expostep);
            stepsclX = (sclXtarget - sclXcurrent) / float(expostep);
            stepsclY = (sclYtarget - sclYcurrent) / float(expostep);
        }

        if(stepNum--) {
            offXcurrent += stepoffX;
            offYcurrent += stepoffY;
            sclYcurrent += stepsclY;
            sclXcurrent += stepsclX;

            Transform::gtrs = glm::translate(glm::mat4(),
                    glm::vec3(offXcurrent, offYcurrent, 0.f));
            Transform::gscl = glm::scale(glm::mat4(),
                    glm::vec3(sclXcurrent, sclYcurrent, 0.f));
        }
        else {
            finalizeZoom();
            hook.disable();
            if(!active) {
                output = core->getMaximisedRegion();
                core->setRedrawEverything(false);
                core->dmg = core->getMaximisedRegion();
                core->getWindowAtPoint = save;
            }

        }
    }

    FireWindow findWindow(int px, int py) {
        GetTuple(w, h, core->getScreenSize());
        GetTuple(vw, vh, core->getWorksize());
        GetTuple(cvx, cvy, core->getWorkspace());

        int vpw = w / vw;
        int vph = h / vh;

        int vx = px / vpw;
        int vy = py / vph;
        int x =  px % vpw;
        int y =  py % vph;

        int realx = (vx - cvx) * w + x * vw;
        int realy = (vy - cvy) * h + y * vh;

        return save(realx, realy);
    }
};

extern "C" {
    Plugin *newInstance() {
        return new Expo();
    }
}
