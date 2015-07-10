#include "../include/wm.hpp"
#include "../include/winstack.hpp"

void WSSwitch::beginSwitch() {
    auto tup = dirs.front();
    dirs.pop();

    int ddx = std::get<0> (tup);
    int ddy = std::get<1> (tup);


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

    output = core->getRegionFromRect(std::min(tlx1, tlx2),
            std::min(tly1, tly2),
            std::max(brx1, brx2),
            std::max(bry1, bry2));

    __FireWindow::allDamaged = true;
    core->resetDMG = false;

    stepNum = 0;
}

#define MAXDIRS 6
void WSSwitch::moveWorkspace(int ddx, int ddy) {
    if(!hook.getState())
        hook.enable(),
        dirs.push(std::make_tuple(ddx, ddy)),
        beginSwitch();
    else
        if(dirs.size() < MAXDIRS)
        dirs.push(std::make_tuple(ddx, ddy));
}

void WSSwitch::handleSwitchWorkspace(Context *ctx) {
    if(!ctx)
        return;

    auto xev = ctx->xev.xkey;

    if(xev.keycode == switchWorkspaceBindings[0])
        moveWorkspace(1,  0);
    if(xev.keycode == switchWorkspaceBindings[1])
        moveWorkspace(-1, 0);
    if(xev.keycode == switchWorkspaceBindings[2])
        moveWorkspace(0, -1);
    if(xev.keycode == switchWorkspaceBindings[3])
        moveWorkspace(0,  1);
}

#define MAXSTEP 20
void WSSwitch::moveStep() {
    GetTuple(w, h, core->getScreenSize());

    if(stepNum == MAXSTEP){
        Transform::gtrs = glm::mat4();
        core->switchWorkspace(std::make_tuple(nx, ny));
        core->redraw = true;
        output = core->getMaximisedRegion();

        if(dirs.size() == 0) {
            hook.disable();
            __FireWindow::allDamaged = false;
            core->resetDMG = true;
        }
        else
            beginSwitch();
        return;
    }
    float progress = float(stepNum++) / float(MAXSTEP);

    float offx =  2.f * progress * float(dx) / float(w);
    float offy = -2.f * progress * float(dy) / float(h);

    if(!dirx)
        offx = 0;
    if(!diry)
        offy = 0;

    Transform::gtrs = glm::translate(glm::mat4(), glm::vec3(offx, offy, 0.0));
    core->redraw = true;
}


void WSSwitch::init(Core *core) {
    using namespace std::placeholders;

    switchWorkspaceBindings[0] = XKeysymToKeycode(core->d, XK_h);
    switchWorkspaceBindings[1] = XKeysymToKeycode(core->d, XK_l);
    switchWorkspaceBindings[2] = XKeysymToKeycode(core->d, XK_j);
    switchWorkspaceBindings[3] = XKeysymToKeycode(core->d, XK_k);

    for(int i = 0; i < 4; i++) {
        kbs[i].type = BindingTypePress;
        kbs[i].active = true;
        kbs[i].mod = ControlMask | Mod1Mask;
        kbs[i].key = switchWorkspaceBindings[i];

        kbs[i].action =
            std::bind(std::mem_fn(&WSSwitch::handleSwitchWorkspace),
            this, _1);

        core->addKey(&kbs[i], true);
    }

    hook.action = std::bind(std::mem_fn(&WSSwitch::moveStep), this);
    core->addHook(&hook);
}

void Expo::init(Core *core) {
    using namespace std::placeholders;
    for(int i = 0; i < 4; i++) {
        keys[i].key = switchWorkspaceBindings[i];
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

void Expo::buttonPress(Context *ctx) {
    buttonRelease(ctx);
    Toggle(ctx);
}

void Expo::recalc() {

    GetTuple(vx, vy, core->getWorkspace());
    GetTuple(vwidth, vheight, core->getWorksize());
    GetTuple(width, height, core->getScreenSize());

    int midx = core->vwidth / 2;
    int midy = core->vheight / 2;

    float offX = float(vx - midx) * 2.f / float(vwidth );
    float offY = float(midy - vy) * 2.f / float(vheight);

    float scaleX = 1.f / float(vwidth);
    float scaleY = 1.f / float(vheight);
    core->scaleX = vwidth;
    core->scaleY = vheight;

    offXtarget = offX;
    offYtarget = offY;
    sclXtarget = scaleX;
    sclYtarget = scaleY;

    output = core->getRegionFromRect(-vx * width, -vy * height,
            (vwidth  - vx) * width, (vheight - vy) * height);
}

void Expo::finalizeZoom() {
    Transform::gtrs = glm::translate(glm::mat4(),
            glm::vec3(offXtarget, offYtarget, 0.f));
    Transform::gscl = glm::scale(glm::mat4(),
            glm::vec3(sclXtarget, sclYtarget, 0.f));
}

void Expo::Toggle(Context *ctx) {
    using namespace std::placeholders;

    if(!active) {
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
            std::bind(std::mem_fn(&Expo::findWindow), this, _1, _2);

        hook.enable();

        __FireWindow::allDamaged = true;
        core->resetDMG = false;
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
        if(!active) {
            output = core->getMaximisedRegion();
            __FireWindow::allDamaged = false;
            core->resetDMG = true;
            core->wins->findWindowAtCursorPosition = save;
        }

    }
    core->redraw = true;
}

FireWindow Expo::findWindow(int px, int py) {
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
void Expo::handleKey(Context *ctx) {
}
