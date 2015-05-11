#include "../include/core.hpp"
#include "../include/winstack.hpp"

Move::Move(Core *c) {
    win = nullptr;
    hook.action = std::bind(std::mem_fn(&Move::Intermediate), this);
    hid = c->addHook(&hook);

    using namespace std::placeholders;

    press.active = true;
    press.type   = BindingTypePress;
    press.mod    = Mod1Mask;
    press.button = Button1;
    press.action = std::bind(std::mem_fn(&Move::Initiate), this, _1);
    c->addBut(&press);


    release.active = false;
    release.type   = BindingTypeRelease;
    release.mod    = AnyModifier;
    release.button = Button1;
    release.action = std::bind(std::mem_fn(&Move::Terminate), this, _1);
    c->addBut(&release);
}

void Move::Initiate(Context *ctx) {
    auto xev = ctx->xev.xbutton;
    auto w = core->wins-> findWindowAtCursorPosition
        (Point(xev.x_root, xev.y_root));

    if(w){
        err << "moving";
        core->wins->focusWindow(w);
        win = w;

        release.active = true;
        hook.enable();


        this->sx = xev.x_root;
        this->sy = xev.y_root;

        XGrabPointer(core->d, core->overlay, TRUE,
                ButtonPressMask | ButtonReleaseMask |
                PointerMotionMask,
                GrabModeAsync, GrabModeAsync,
                core->root, None, CurrentTime);
    }
}

void Move::Terminate(Context *ctx) {

    if(!ctx)
        return;

    hook.disable();
    release.active = false;

    auto xev = ctx->xev.xbutton;

    win->transform.translation = glm::mat4();

    int dx = (xev.x_root - sx) * core->scaleX;
    int dy = (xev.y_root - sy) * core->scaleY;

    int nx = win->attrib.x + dx;
    int ny = win->attrib.y + dy;

    WinUtil::moveWindow(win, nx, ny);
    XUngrabPointer(core->d, CurrentTime);
    core->wins->focusWindow(win);

    core->redraw = true;
}

void Move::Intermediate() {

    win->transform.translation =
        glm::translate(glm::mat4(), glm::vec3(
                    float(core->mousex - sx) / float(core->width / 2.0),
                    float(sy - core->mousey) / float(core->height / 2.0),
                    0.f));
    core->redraw = true;
}

Resize::Resize(Core *c) {
    win = nullptr;

    hook.action = std::bind(std::mem_fn(&Resize::Intermediate), this);
    hid = c->addHook(&hook);

    using namespace std::placeholders;

    press.active = true;
    press.type   = BindingTypePress;
    press.mod    = ControlMask;
    press.button = Button1;
    press.action = std::bind(std::mem_fn(&Resize::Initiate), this, _1);
    c->addBut(&press);


    release.active = false;
    release.type   = BindingTypeRelease;
    release.mod    = AnyModifier;
    release.button = Button1;
    release.action = std::bind(std::mem_fn(&Resize::Terminate), this,_1);
    c->addBut(&release);
}

void Resize::Initiate(Context *ctx) {
    if(!ctx)
        return;

    auto xev = ctx->xev.xbutton;
    auto w = WinUtil::getAncestor(core->wins->findWindow(xev.window));

    if(w){

        core->wins->focusWindow(w);
        win = w;
        hook.enable();
        release.active = true;

        if(w->attrib.width == 0)
            w->attrib.width = 1;
        if(w->attrib.height == 0)
            w->attrib.height = 1;

        this->sx = xev.x_root;
        this->sy = xev.y_root;

        XGrabPointer(core->d, core->overlay, TRUE,
                ButtonPressMask | ButtonReleaseMask |
                PointerMotionMask,
                GrabModeAsync, GrabModeAsync,
                core->root, None, CurrentTime);
    }
}

void Resize::Terminate(Context *ctx) {
    if(!ctx)
        return;

    hook.disable();
    release.active = false;

    win->transform.scalation = glm::mat4();
    win->transform.translation = glm::mat4();

    int dw = (core->mousex - sx) * core->scaleX;
    int dh = (core->mousey - sy) * core->scaleY;

    int nw = win->attrib.width  + dw;
    int nh = win->attrib.height + dh;
    WinUtil::resizeWindow(win, nw, nh);

    XUngrabPointer(core->d, CurrentTime);
    core->wins->focusWindow(win);
    core->redraw = true;
}

void Resize::Intermediate() {

    int dw = core->mousex - sx;
    int dh = core->mousey - sy;

    int nw = win->attrib.width  + dw;
    int nh = win->attrib.height + dh;

    float kW = float(nw) / float(win->attrib.width );
    float kH = float(nh) / float(win->attrib.height);


    float w2 = float(core->width) / 2.;
    float h2 = float(core->height) / 2.;

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
    core->redraw = true;
}
