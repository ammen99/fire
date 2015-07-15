#include "move.hpp"

void Move::initOwnership() {
    owner->name = "move";
    owner->compatAll = true;
}

void Move::updateConfiguration() {
}

void Move::init() {
    win = nullptr;
    hook.action = std::bind(std::mem_fn(&Move::Intermediate), this);
    core->addHook(&hook);

    using namespace std::placeholders;

    press.active = true;
    press.type   = BindingTypePress;
    press.mod    = Mod1Mask;
    press.button = Button1;
    press.action = std::bind(std::mem_fn(&Move::Initiate), this, _1);
    core->addBut(&press);

    release.active = false;
    release.type   = BindingTypeRelease;
    release.mod    = AnyModifier;
    release.button = Button1;
    release.action = std::bind(std::mem_fn(&Move::Terminate), this, _1);
    core->addBut(&release);
}

void Move::Initiate(Context *ctx) {
    auto xev = ctx->xev.xbutton;
    auto w = core->getWindowAtPoint(xev.x_root, xev.y_root);

    if(!w)
        return;
    win = w;

    if(!core->activateOwner(owner)){
        return;
    }

    owner->grab();

    core->focusWindow(w);
    hook.enable();
    release.active = true;

    this->sx = xev.x_root;
    this->sy = xev.y_root;
    core->setRedrawEverything(true);
}

void Move::Terminate(Context *ctx) {

    if(!ctx)
        return;

    hook.disable();
    release.active = false;
    core->deactivateOwner(owner);


    auto xev = ctx->xev.xbutton;

    win->transform.translation = glm::mat4();

    int dx = (xev.x_root - sx) * core->scaleX;
    int dy = (xev.y_root - sy) * core->scaleY;

    int nx = win->attrib.x + dx;
    int ny = win->attrib.y + dy;

    WinUtil::moveWindow(win, nx, ny);
    core->setRedrawEverything(false);

    core->focusWindow(win);
    core->damageWindow(win);
}

void Move::Intermediate() {

    GetTuple(cmx, cmy, core->getMouseCoord());
    GetTuple(w, h, core->getScreenSize());

    win->transform.translation =
        glm::translate(glm::mat4(), glm::vec3(
                    float(cmx - sx) / float(w / 2.0),
                    float(sy - cmy) / float(h / 2.0),
                    0.f));
}

extern "C" {
    Plugin *newInstance() {
        return new Move();
    }
}

