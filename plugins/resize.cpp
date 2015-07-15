#include "../include/wm.hpp"
/* specialized class for operations on window(for ex. Move and Resize) */
class Resize : public Plugin {
    private:
        int sx, sy; // starting pointer x, y
        FireWindow win; // window we're operating on

    private:
        ButtonBinding press;
        ButtonBinding release;
        Hook hook;
    public:

    void initOwnership() {
        owner->name = "resize";
        owner->compatAll = true;
    }

    void init() {
        win = nullptr;

        hook.action = std::bind(std::mem_fn(&Resize::Intermediate), this);
        core->addHook(&hook);

        using namespace std::placeholders;

        press.active = true;
        press.type   = BindingTypePress;
        press.mod    = Mod4Mask;
        press.button = Button1;
        press.action = std::bind(std::mem_fn(&Resize::Initiate), this, _1);
        core->addBut(&press);


        release.active = false;
        release.type   = BindingTypeRelease;
        release.mod    = AnyModifier;
        release.button = Button1;
        release.action = std::bind(std::mem_fn(&Resize::Terminate), this,_1);
        core->addBut(&release);
    }

    void Initiate(Context *ctx) {
        if(!ctx)
            return;

        auto xev = ctx->xev.xbutton;
        auto w = core->getWindowAtPoint(xev.x_root,xev.y_root);

        if(!w)
            return;

        if(!core->activateOwner(owner)) {
            return;
        }

        owner->grab();

        core->focusWindow(w);
        win = w;
        hook.enable();
        release.active = true;

        if(w->attrib.width == 0)
            w->attrib.width = 1;
        if(w->attrib.height == 0)
            w->attrib.height = 1;

        this->sx = xev.x_root;
        this->sy = xev.y_root;

        owner->active = true;
        core->setRedrawEverything(true);
    }

    void Terminate(Context *ctx) {
        if(!ctx)
            return;

        hook.disable();
        release.active = false;

        win->transform.scalation = glm::mat4();
        win->transform.translation = glm::mat4();

        GetTuple(cmx, cmy, core->getMouseCoord());

        int dw = (cmx - sx) * core->scaleX;
        int dh = (cmy - sy) * core->scaleY;

        int nw = win->attrib.width  + dw;
        int nh = win->attrib.height + dh;
        WinUtil::resizeWindow(win, nw, nh);

        core->setRedrawEverything(false);
        core->damageWindow(win);
        core->deactivateOwner(owner);
    }

    void Intermediate() {


        GetTuple(cmx, cmy, core->getMouseCoord());
        int dw = cmx - sx;
        int dh = cmy - sy;

        int nw = win->attrib.width  + dw;
        int nh = win->attrib.height + dh;

        float kW = float(nw) / float(win->attrib.width );
        float kH = float(nh) / float(win->attrib.height);


        GetTuple(sw, sh, core->getScreenSize());

        float w2 = float(sw) / 2.;
        float h2 = float(sh) / 2.;

        float tlx = float(win->attrib.x) - w2,
              tly = h2 - float(win->attrib.y);

        float ntlx = kW * tlx;
        float ntly = kH * tly;

        win->transform.translation =
            glm::translate(glm::mat4(), glm::vec3(
                        float(tlx - ntlx) / w2, float(tly - ntly) / h2,
                        0.f));

        win->transform.scalation =
            glm::scale(glm::mat4(), glm::vec3(kW, kH, 1.f));
    }
};

extern "C" {
    Plugin *newInstance() {
        return new Resize();
    }
}
