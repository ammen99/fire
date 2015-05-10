#include "../include/commonincludes.hpp"
#include "../include/core.hpp"
#include "../include/opengl.hpp"
#include "../include/winstack.hpp"

#include <glog/logging.h>

bool wmDetected;
std::mutex wmMutex;
namespace {
    bool inMapping;   //  used to determine whether
    bool ignoreWindow;//  a BadWindow    has arised
                      //  from a XMapWindow request
                      // when receiving CreateNotify
}

WinStack *Core::wins;

Context::Context(XEvent ev) : xev(ev){}
Hook::Hook() : active(false) {}

void Hook::enable() {
    if(this->active)
        return;
    this->active = true;
    core->cntHooks++;
}

void Hook::disable() {
    if(!this->active)
        return;
    this->active = false;
    core->cntHooks--;
}

bool Hook::getState() {
    return this->active;
}

Core::Core() {

    inMapping = false;
    d = XOpenDisplay(NULL);

    if ( d == nullptr )
        err << "Failed to open display!";


    XSynchronize(d, 1);
    root = DefaultRootWindow(d);
    fd.fd = ConnectionNumber(d);
    fd.events = POLLIN;


    XWindowAttributes xwa;
    XGetWindowAttributes(d, root, &xwa);
    width = xwa.width;
    height = xwa.height;

    XSetErrorHandler(&Core::onOtherWmDetected);

    XCompositeRedirectSubwindows(d, root, CompositeRedirectManual);
    XSelectInput (d, root,
             SubstructureRedirectMask | SubstructureNotifyMask   |
             StructureNotifyMask      | PropertyChangeMask       |
             LeaveWindowMask          | EnterWindowMask          |
             KeyPressMask             | KeyReleaseMask           |
             ButtonPressMask          | ButtonReleaseMask        |
             FocusChangeMask          | ExposureMask             |
             Button1MotionMask);

    if(wmDetected)
       err << "Another WM already running!\n", std::exit(-1);

    wins = new WinStack();

    XSetErrorHandler(&Core::onXError);
    overlay = XCompositeGetOverlayWindow(d, root);

    XserverRegion region;
    region = XFixesCreateRegion(d, NULL, 0);
    XFixesSetWindowShapeRegion(d, overlay, ShapeBounding, 0, 0, 0);
    XFixesSetWindowShapeRegion(d, overlay, ShapeInput, 0, 0, region);
    XFixesDestroyRegion(d, region);

    int dummy;
    XDamageQueryExtension(d, &damage, &dummy);

    move = new Move(this);
    resize = new Resize(this);

    ButtonBinding *focus = new ButtonBinding();
    focus->type = BindingTypePress;
    focus->button = Button1;
    focus->mod = NoMods;
    focus->active = true;

    focus->action = [] (Context *ctx){
        err << "Focusing window";
        auto xev = ctx->xev.xbutton;
        auto w =
            wins->findWindowAtCursorPosition
            (Point(xev.x_root, xev.y_root));

        if(w)
            wins->focusWindow(w);
    };
    addBut(focus);

    switchWorkspaceBindings[0] = XKeysymToKeycode(d, XK_h);
    switchWorkspaceBindings[1] = XKeysymToKeycode(d, XK_l);
    switchWorkspaceBindings[2] = XKeysymToKeycode(d, XK_j);
    switchWorkspaceBindings[3] = XKeysymToKeycode(d, XK_k);

    vwidth = vheight = 3;
    vx = vy = 0;
    wsswitch = new WSSwitch(this);

    cntHooks = 0;
    output = Rect(0, 0, width, height);

    KeyBinding *run = new KeyBinding();
    run->action = [](Context *ctx){
        core->run(const_cast<char*>("dmenu_run"));
    };

    run->active = true;
    run->mod = Mod1Mask;
    run->type = BindingTypePress;
    run->key = XKeysymToKeycode(d, XK_r);

    addKey(run, true);

    KeyBinding *close = new KeyBinding();
    auto exit = [](Context *ctx){
        std::exit(0);
    };

    close->active = true;
    close->mod = ControlMask;
    close->type = BindingTypePress;
    close->key = XKeysymToKeycode(d, XK_q);
    close->action = exit;

    addKey(close, true);

    err << "Before expo";
    expo = new Expo(this);
    err << "After expo";
}

Core::~Core(){
    XCompositeReleaseOverlayWindow(d, overlay);
    XCloseDisplay(d);
}

void Core::run(char *command) {
    auto pid = fork();
    if(!pid) {
        std::string str("DISPLAY=");
        str = str.append(XDisplayString(d));

        putenv(const_cast<char*>(str.c_str()));

        std::exit(execl("/bin/sh", "/bin/sh", "-c", command, NULL));
    }
}

#define MAXID (uint)(-1)
template<class T>
uint Core::getFreeID(std::unordered_map<uint, T> *map) {
    for(int i = 0; i < MAXID; i++)
        if(map->find(i) == map->end())
            return i;

    return MAXID;
}

uint Core::addHook(Hook *hook){
    auto id = getFreeID(&hooks);
    hooks.insert({id, hook});
    return id;
}

void Core::remHook(uint id) {
    hooks.erase(id);
}

uint Core::addKey(KeyBinding *kb, bool grab) {
    if(!kb)
        return -1;

    auto id = getFreeID(&keys);
    keys.insert({id, kb});

    if(grab)
    XGrabKey(d, kb->key, (kb->mod == NoMods ? AnyModifier : kb->mod), root,
            false, GrabModeAsync, GrabModeAsync);

    return id;
}

void Core::remKey(uint id) {
    auto it = keys.find(id);
    if(it == keys.end())
        return;

    auto kb = it->second;
    XUngrabKey(d, kb->key, kb->mod, root);
    keys.erase(it);
}

uint Core::addBut(ButtonBinding *bb, bool grab) {
    if(!bb)
        return -1;

    auto id = getFreeID(&buttons);
    buttons.insert({id, bb});

    if(grab)
    XGrabButton(d, bb->button, bb->mod,
            root, false, ButtonPressMask,
            GrabModeAsync, GrabModeAsync,
            None, None);

    return id;
}

void Core::remBut(uint id) {
    auto it = buttons.find(id);
    if(it == buttons.end())
        return;

    auto bb = it->second;

    XUngrabButton(d, bb->button, bb->mod, root);
    buttons.erase(it);
}


void Core::setBackground(const char *path) {
    auto texture = GLXUtils::loadImage(const_cast<char*>(path));

    uint vao, vbo;
    OpenGLWorker::generateVAOVBO(0, height, width, -height, vao, vbo);

    backgrounds.clear();
    backgrounds.resize(vheight);
    for(int i = 0; i < vheight; i++)
        backgrounds[i].resize(vwidth);

    for(int i = 0; i < vheight; i++){
        for(int j = 0; j < vwidth; j++) {

            backgrounds[i][j] = std::make_shared<__FireWindow>();

            backgrounds[i][j]->vao = vao;
            backgrounds[i][j]->vbo = vbo;
            backgrounds[i][j]->norender = false;
            backgrounds[i][j]->texture  = texture;

            backgrounds[i][j]->attrib.x = j * width;
            backgrounds[i][j]->attrib.y = i * height;
            backgrounds[i][j]->attrib.width  = width;
            backgrounds[i][j]->attrib.height = height;

            backgrounds[i][j]->type = WindowTypeDesktop;
            backgrounds[i][j]->regenVBOFromAttribs();
            wins->addWindow(backgrounds[i][j]);
        }
    }

}

FireWindow Core::findWindow(Window win) {
    return wins->findWindow(win);
}

void Core::addWindow(XCreateWindowEvent xev) {
    FireWindow w = std::make_shared<__FireWindow>();
    w->id = xev.window;

    if(xev.parent != root)
        w->transientFor = findWindow(xev.parent);

    w->xvi = nullptr;
    wins->addWindow(w);
}

void Core::renderAllWindows() {
    OpenGLWorker::preStage();
    wins->renderWindows();
    GLXUtils::endFrame(overlay);
}

void Core::wait(int timeout) {
    poll(&fd, 1, timeout / 1000); // convert from usec to msec
}

void Core::handleEvent(XEvent xev){
    switch(xev.type) {
        case Expose:
            redraw = true;
        case KeyPress: {
            // check keybindings
            for(auto kb : keys)
                if(kb.second->key == xev.xkey.keycode &&
                   kb.second->mod == xev.xkey.state)
                if(kb.second->active)
                    kb.second->action(new Context(xev));

            redraw = true;
            break;
        }

        case CreateNotify: {
            if (xev.xcreatewindow.window == overlay)
                break;

            err << "CreateNotify";
            inMapping = true;
            XMapWindow(core->d, xev.xcreatewindow.window);
            XSync(core->d, 0);
            inMapping = false;

            if(ignoreWindow){
                ignoreWindow = false;
                break;
            }
            addWindow(xev.xcreatewindow);

            redraw = true;
            break;
        }
        case DestroyNotify: {
            auto w = wins->findWindow(xev.xdestroywindow.window);
            if ( w == nullptr )
                break;

            wins->removeWindow(w, true);
            redraw = true;
            break;
        }
        case MapRequest: {
            auto w = wins->findWindow(xev.xmaprequest.window);
            if(w == nullptr)
                break;

            w->norender = false;
            WinUtil::syncWindowAttrib(w);
            auto parent = WinUtil::getAncestor(w);
            wins->restackTransients(parent);
            redraw = true;
        }
        case MapNotify: {

            auto w = wins->findWindow(xev.xmap.window);
            if(w == nullptr)
                break;

            w->norender = false;
            WinUtil::syncWindowAttrib(w);
            auto parent = WinUtil::getAncestor(w);
            wins->restackTransients(parent);
            w->attrib.map_state = IsViewable;
            redraw = true;
            break;
        }
        case UnmapNotify: {

            auto w = wins->findWindow(xev.xunmap.window);
            if(w == nullptr)
                break;

            w->norender = true;
            w->attrib.map_state = IsUnmapped;
            redraw = true;
            break;
        }

        case ButtonPress: {
            mousex = xev.xbutton.x_root;
            mousey = xev.xbutton.y_root;

            for(auto bb : buttons)
                if(bb.second->active)
                if(bb.second->type == BindingTypePress)
                if((bb.second->mod & xev.xbutton.state) ||
                   (bb.second->mod == NoMods            && // check if there
                    xev.xbutton.state == 0))               // are no mods
                if(bb.second->button == xev.xbutton.button)// and we're
                    bb.second->action(new Context(xev));   // listening for it

            XAllowEvents(d, ReplayPointer, xev.xbutton.time);
            break;
        }
        case ButtonRelease: // release bindings should be enabled
                            // only after some press binding has been
                            // activated => there is no need to check
                            // for buttons
            for(auto bb : this->buttons)
                if(bb.second->type == BindingTypeRelease)
                if(bb.second->active)
                    bb.second->action(new Context(xev));

            XAllowEvents(d, ReplayPointer, xev.xbutton.time);
            break;

        case MotionNotify:
            mousex = xev.xmotion.x_root;
            mousey = xev.xmotion.y_root;
            break;
        case EnterNotify: {
            auto w = wins->findWindow(xev.xcrossing.window);
            if(w)
                wins->focusWindow(w);
        }
        default:
            if(xev.type == damage + XDamageNotify)
                redraw = true;
            break;
    }
}

#define RefreshRate 200
#define Second 1000000

void Core::loop(){

    std::lock_guard<std::mutex> lock(wmMutex);
    redraw = true;

    int currentCycle = Second / RefreshRate;
    currentCycle -= 50;
    int baseCycle = currentCycle;

    timeval before, after;
    gettimeofday(&before, 0);
    bool hadEvents = false;

    XEvent xev;

    while(true) {

        while(XPending(d)) {
            XNextEvent(d, &xev);
            handleEvent(xev);
        }

        gettimeofday(&after, 0);
        int diff = (after.tv_sec - before.tv_sec) * 1000000 +
            after.tv_usec - before.tv_usec;

        if(diff < currentCycle) {
            wait(currentCycle - diff);
            if(fd.revents & POLLIN) { /* disable optimisation */
                hadEvents = true;
                currentCycle = baseCycle;
                continue;
            }
            fd.revents = 0;
        }
        else {
            if(cntHooks) {
                for (auto hook : hooks)
                    if(hook.second->getState())
                        hook.second->action();
            }

            if(redraw)
                renderAllWindows(),
                    redraw = false;

            /* optimisation when idle */
            if(!cntHooks && !hadEvents && currentCycle < Second)
                currentCycle *= 2;

            before = after;
            hadEvents = false;
        }
    }
}

int Core::onOtherWmDetected(Display* d, XErrorEvent *xev) {
    CHECK_EQ(static_cast<int>(xev->error_code), BadAccess);
    wmDetected = true;
    return 0;
}

int Core::onXError(Display *d, XErrorEvent *xev) {
    if(xev->resourceid == 0) // invalid window
        return 0;

    if(xev->error_code == BadMatch    ||
       xev->error_code == BadDrawable ||
       xev->error_code == BadWindow   ){

        err << "caught BadMatch/Drawable/Window. Disabling window drawing "
            << xev->resourceid;

        auto x = wins->findWindow(xev->resourceid);
        if (x != nullptr)
            x->norender = true;

        return 0;
    }

    err << "____________________________";
    err << "XError code   " << int(xev->error_code);
    char buf[512];
    XGetErrorText(d, xev->error_code, buf, 512);
    err << "XError string " << buf;
    err << "____________________________";
    return 0;
}

Core::Move::Move(Core *c) {
    win = nullptr;
    hook.action = std::bind(std::mem_fn(&Core::Move::Intermediate), this);
    hid = c->addHook(&hook);

    using namespace std::placeholders;

    press.active = true;
    press.type   = BindingTypePress;
    press.mod    = Mod1Mask;
    press.button = Button1;
    press.action = std::bind(std::mem_fn(&Core::Move::Initiate), this, _1);
    c->addBut(&press);


    release.active = false;
    release.type   = BindingTypeRelease;
    release.mod    = AnyModifier;
    release.button = Button1;
    release.action = std::bind(std::mem_fn(&Core::Move::Terminate), this, _1);
    c->addBut(&release);
}

void Core::Move::Initiate(Context *ctx) {
    auto xev = ctx->xev.xbutton;
    auto w = wins->findWindowAtCursorPosition(Point(xev.x_root, xev.y_root));

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

void Core::Move::Terminate(Context *ctx) {

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

void Core::Move::Intermediate() {

    win->transform.translation =
        glm::translate(glm::mat4(), glm::vec3(
                    float(core->mousex - sx) / float(core->width / 2.0),
                    float(sy - core->mousey) / float(core->height / 2.0),
                    0.f));
    core->redraw = true;
}

Core::Resize::Resize(Core *c) {
    win = nullptr;

    hook.action = std::bind(std::mem_fn(&Core::Resize::Intermediate), this);
    hid = c->addHook(&hook);

    using namespace std::placeholders;

    press.active = true;
    press.type   = BindingTypePress;
    press.mod    = ControlMask;
    press.button = Button1;
    press.action = std::bind(std::mem_fn(&Core::Resize::Initiate), this, _1);
    c->addBut(&press);


    release.active = false;
    release.type   = BindingTypeRelease;
    release.mod    = AnyModifier;
    release.button = Button1;
    release.action = std::bind(std::mem_fn(&Core::Resize::Terminate), this,_1);
    c->addBut(&release);
}

void Core::Resize::Initiate(Context *ctx) {
    if(!ctx)
        return;

    auto xev = ctx->xev.xbutton;
    auto w = WinUtil::getAncestor(core->wins->findWindow(xev.window));

    if(w){

        wins->focusWindow(w);
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

void Core::Resize::Terminate(Context *ctx) {
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

void Core::Resize::Intermediate() {

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

std::tuple<int, int> Core::getWorkspace() {
    return std::make_tuple(vx, vy);
}

void Core::switchWorkspace(std::tuple<int, int> nPos) {
    auto nx = std::get<0> (nPos);
    auto ny = std::get<1> (nPos);

    auto dx = (vx - nx) * width;
    auto dy = (vy - ny) * height;

    for(auto w : wins->wins)
        WinUtil::moveWindow(w, w->attrib.x + dx, w->attrib.y + dy);

    vx = nx;
    vy = ny;
}

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

    //err << "Switching viewport";
    //err << nx << " " << ny;
    //err << __FireWindow::output;
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

    int realx = (core->vx - vx) * core->width  + x * core->vwidth;
    int realy = (core->vy - vy) * core->height + y * core->vheight;
    err << realx << " " << realy;

    return save(Point(realx, realy));
}
void Core::Expo::handleKey(Context *ctx) {
}

Core *core;
