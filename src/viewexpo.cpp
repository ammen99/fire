#include "../include/core.hpp"
#include "../include/winstack.hpp"

void WSSwitch::beginSwitch() {
    auto tup = dirs.front();
    dirs.pop();

    int ddx = std::get<0> (tup);
    int ddy = std::get<1> (tup);

    auto nx = (core->vx - ddx + core->vwidth ) % core->vwidth;
    auto ny = (core->vy - ddy + core->vheight) % core->vheight;

    this->nx = nx;
    this->ny = ny;

    dirx = ddx;
    diry = ddy;

    dx = (core->vx - nx) * core->width;
    dy = (core->vy - ny) * core->height;

    int tlx1 = 0;
    int tly1 = 0;
    int brx1 = core->width;
    int bry1 = core->height;

    int tlx2 = -dx;
    int tly2 = -dy;
    int brx2 = tlx2 + core->width;
    int bry2 = tly2 + core->height;

    output = Rect(std::min(tlx1, tlx2),
            std::min(tly1, tly2),
            std::max(brx1, brx2),
            std::max(bry1, bry2));
    stepNum = 0;
}

void WSSwitch::moveWorkspace(int ddx, int ddy) {
    if(!hook.getState())
        hook.enable(),
        dirs.push(std::make_tuple(ddx, ddy)),
        beginSwitch();
    else
        dirs.push(std::make_tuple(ddx, ddy));
}

void WSSwitch::handleSwitchWorkspace(Context *ctx) {
    if(!ctx)
        return;

    auto xev = ctx->xev.xkey;

    if(xev.keycode == core->switchWorkspaceBindings[0])
        moveWorkspace(1,  0);
    if(xev.keycode == core->switchWorkspaceBindings[1])
        moveWorkspace(-1, 0);
    if(xev.keycode == core->switchWorkspaceBindings[2])
        moveWorkspace(0, -1);
    if(xev.keycode == core->switchWorkspaceBindings[3])
        moveWorkspace(0,  1);
}

#define MAXSTEP 60
void WSSwitch::moveStep() {
    if(stepNum == MAXSTEP){
        Transform::gtrs = glm::mat4();
        core->switchWorkspace(std::make_tuple(nx, ny));
        core->redraw = true;
        output = Rect(0, 0, core->width, core->height);

        if(dirs.size() == 0)
            hook.disable();
        else
            beginSwitch();
        return;
    }
    float progress = float(stepNum++) / float(MAXSTEP);

    float offx =  2.f * progress * float(dx) / float(core->width );
    float offy = -2.f * progress * float(dy) / float(core->height);

    if(!dirx)
        offx = 0;
    if(!diry)
        offy = 0;

    Transform::gtrs = glm::translate(glm::mat4(), glm::vec3(offx, offy, 0.0));
    core->redraw = true;
}


WSSwitch::WSSwitch(Core *core) {
    using namespace std::placeholders;

    core->switchWorkspaceBindings[0] = XKeysymToKeycode(core->d, XK_h);
    core->switchWorkspaceBindings[1] = XKeysymToKeycode(core->d, XK_l);
    core->switchWorkspaceBindings[2] = XKeysymToKeycode(core->d, XK_j);
    core->switchWorkspaceBindings[3] = XKeysymToKeycode(core->d, XK_k);

    core->vwidth = core->vheight = 3;
    core->vx = core->vy = 0;

    for(int i = 0; i < 4; i++) {
        kbs[i].type = BindingTypePress;
        kbs[i].active = true;
        kbs[i].mod = ControlMask | Mod1Mask;
        kbs[i].key = core->switchWorkspaceBindings[i];

        kbs[i].action =
            std::bind(std::mem_fn(&WSSwitch::handleSwitchWorkspace),
            this, _1);

        core->addKey(&kbs[i], true);
    }

    hook.action = std::bind(std::mem_fn(&WSSwitch::moveStep), this);
    core->addHook(&hook);
}

Expo::Expo(Core *core) {
    using namespace std::placeholders;
    for(int i = 0; i < 4; i++) {
        keys[i].key = core->switchWorkspaceBindings[i];
        keys[i].active = false;
        keys[i].mod = NoMods;
        keys[i].action =
            std::bind(std::mem_fn(&Expo::handleKey), this, _1);
        core->addKey(&keys[i]);
    }

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

void Expo::buttonRelease(Context *ctx) {

    auto xev = ctx->xev.xbutton;

    int vpw = core->width / core->vwidth;
    int vph = core->height / core->vheight;

    int vx = xev.x_root / vpw;
    int vy = xev.y_root / vph;

    core->switchWorkspace(std::make_tuple(vx, vy));

    recalc();
    finalizeZoom();
}

void Expo::buttonPress(Context *ctx) {
    //err << "Button Press from expo wird gecallt";
    buttonRelease(ctx);
    Toggle(ctx);
}

void Expo::recalc() {

    int midx = core->vwidth / 2;
    int midy = core->vheight / 2;

    float offX = float(core->vx - midx) * 2.f / float(core->vwidth );
    float offY = float(midy - core->vy) * 2.f / float(core->vheight);

    float scaleX = 1.f / float(core->vwidth);
    float scaleY = 1.f / float(core->vheight);
    core->scaleX = core->vwidth;
    core->scaleY = core->vheight;

    offXtarget = offX;
    offYtarget = offY;
    sclXtarget = scaleX;
    sclYtarget = scaleY;

    output = Rect(-core->vx * core->width, // output everything
            -core->vy * core->height,
            (core->vwidth  - core->vx) * core->width,
            (core->vheight - core->vy) * core->height);
}

void Expo::finalizeZoom() {
    err << "Finalizing zoom";
    err << offXtarget << " " << offYtarget;
    err << sclXtarget << " " << sclYtarget;
    Transform::gtrs = glm::translate(glm::mat4(),
            glm::vec3(offXtarget, offYtarget, 0.f));
    Transform::gscl = glm::scale(glm::mat4(),
            glm::vec3(sclXtarget, sclYtarget, 0.f));
}

#define MAXSTEP 60

void Expo::Toggle(Context *ctx) {
    using namespace std::placeholders;
    if(!active) {
        err << "Activating expo";
        active = !active;

        press.active = true;
        release.active = true;
        XGrabPointer(core->d, core->overlay, TRUE,
                ButtonPressMask | ButtonReleaseMask |
                PointerMotionMask,
                GrabModeAsync, GrabModeAsync,
                core->root, None, CurrentTime);

        save = core->wins->findWindowAtCursorPosition;
        core->wins->findWindowAtCursorPosition =
            std::bind(std::mem_fn(&Expo::findWindow), this, _1);

        hook.enable();
        stepNum = MAXSTEP;
        recalc();

        offXcurrent = 0;
        offYcurrent = 0;
        sclXcurrent = 1;
        sclYcurrent = 1;

        core->redraw = true;
    }else {
        active = !active;

        press.active = false;
        release.active = false;
        XUngrabPointer(core->d, CurrentTime);

        core->scaleX = 1;
        core->scaleY = 1;

        core->redraw = true;
        core->wins->findWindowAtCursorPosition = save;



        hook.enable();
        stepNum = MAXSTEP;

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

void Expo::zoom() {

    err << "Running, stepNum = " << stepNum;
    err << offXcurrent << " " << offYcurrent;
    err << sclXcurrent << " " << sclYcurrent;
    if(stepNum == MAXSTEP) {
        stepoffX = (offXtarget - offXcurrent) / float(MAXSTEP);
        stepoffY = (offYtarget - offYcurrent) / float(MAXSTEP);
        stepsclX = (sclXtarget - sclXcurrent) / float(MAXSTEP);
        stepsclY = (sclYtarget - sclYcurrent) / float(MAXSTEP);
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
        if(!active)
            output = Rect(0, 0, core->width, core->height);
    }
    core->redraw = true;
}

FireWindow Expo::findWindow(Point p) {
    int vpw = core->width / core->vwidth;
    int vph = core->height / core->vheight;

    int vx = p.x / vpw;
    int vy = p.y / vph;
    int x =  p.x % vpw;
    int y =  p.y % vph;

    int realx = (vx - core->vx) * core->width  + x * core->vwidth;
    int realy = (vy - core->vy) * core->height + y * core->vheight;

    return save(Point(realx, realy));
}
void Expo::handleKey(Context *ctx) {
}
