#include "../include/core.hpp"
#include "../include/winstack.hpp"

void Core::WSSwitch::moveWorkspace(int ddx, int ddy) {
    auto nx = (core->vx - ddx + core->vwidth ) % core->vwidth;
    auto ny = (core->vy - ddy + core->vheight) % core->vheight;

    dirx = ddx;
    diry = ddy;

    dx = (core->vx - nx) * core->width;
    dy = (core->vy - ny) * core->height;

    this->nx = nx;
    this->ny = ny;

    stepNum = 0;
    hook.enable();


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
}

void Core::WSSwitch::handleSwitchWorkspace(Context *ctx) {
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
void Core::WSSwitch::moveStep() {
    if(stepNum == MAXSTEP){
        Transform::gtrs = glm::mat4();
        core->switchWorkspace(std::make_tuple(nx, ny));
        hook.disable();
        core->redraw = true;
        output = Rect(0, 0, core->width, core->height);
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


Core::WSSwitch::WSSwitch(Core *core) {
    using namespace std::placeholders;

    for(int i = 0; i < 4; i++) {
        kbs[i].type = BindingTypePress;
        kbs[i].active = true;
        kbs[i].mod = ControlMask | Mod1Mask;
        kbs[i].key = core->switchWorkspaceBindings[i];

        kbs[i].action =
            std::bind(std::mem_fn(&Core::WSSwitch::handleSwitchWorkspace),
            this, _1);

        core->addKey(&kbs[i], true);
    }

    hook.action = std::bind(std::mem_fn(&Core::WSSwitch::moveStep), this);
    core->addHook(&hook);
}

Core::Expo::Expo(Core *core) {
    using namespace std::placeholders;
    for(int i = 0; i < 4; i++) {
        err << "Beginning";
        keys[i].key = core->switchWorkspaceBindings[i];
        keys[i].active = false;
        keys[i].mod = NoMods;
        keys[i].action =
            std::bind(std::mem_fn(&Core::Expo::handleKey), this, _1);
        core->addKey(&keys[i]);
        err << "Exnd";
    }

    err << "Added keys";

    toggle.key = XKeysymToKeycode(core->d, XK_e);
    toggle.mod = Mod4Mask;
    toggle.active = true;
    toggle.action =
        std::bind(std::mem_fn(&Core::Expo::Toggle), this, _1);

    core->addKey(&toggle, true);

    active = false;
}

void Core::Expo::Toggle(Context *ctx) {
    using namespace std::placeholders;
    if(!active) {
        active = !active;

        save = core->wins->findWindowAtCursorPosition;
        core->wins->findWindowAtCursorPosition =
            std::bind(std::mem_fn(&Core::Expo::findWindow), this, _1);

        int midx = core->vwidth / 2;
        int midy = core->vheight / 2;

        float offX = float(core->vx - midx) * 2.f / float(core->vwidth );
        float offY = float(midy - core->vy) * 2.f / float(core->vheight);

        float scaleX = 1.f / float(core->vwidth);
        float scaleY = 1.f / float(core->vheight);
        core->scaleX = core->vwidth;
        core->scaleY = core->vheight;

        Transform::gtrs = glm::translate(Transform::gtrs,
                          glm::vec3(offX, offY, 0.f));
        Transform::gscl = glm::scale(Transform::gscl,
                          glm::vec3(scaleX, scaleY, 0.f));

        output = Rect(-core->vx * core->width, // output everything
                      -core->vy * core->height,
                      (core->vwidth  - core->vx) * core->width,
                      (core->vheight - core->vy) * core->height);

        core->redraw = true;
    }else {
        active = !active;
        Transform::gtrs = glm::mat4();
        Transform::gscl = glm::mat4();

        output = Rect(0, 0, core->width, core->height);

        core->scaleX = 1;
        core->scaleY = 1;

        core->redraw = true;
        core->wins->findWindowAtCursorPosition = save;
    }
}

FireWindow Core::Expo::findWindow(Point p) {
    int vpw = core->width / core->vwidth;
    int vph = core->height / core->vheight;

    int vx = p.x / vpw;
    int vy = p.y / vph;
    int x =  p.x % vpw;
    int y =  p.y % vph;

    err << "Finding winodw";
    err << vpw << " " << vph;
    err << vx << " " << vy;
    err << x  << " " << y ;

    int realx = (vx - core->vx) * core->width  + x * core->vwidth;
    int realy = (vy - core->vy) * core->height + y * core->vheight;
    err << realx << " " << realy;

    return save(Point(realx, realy));
}
void Core::Expo::handleKey(Context *ctx) {
}
