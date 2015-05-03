#include "../include/commonincludes.hpp"
#include "../include/core.hpp"
#include "../include/opengl.hpp"
#include "../include/winstack.hpp"

#include <glog/logging.h>

bool wmDetected;
std::mutex wmMutex;

WinStack *Core::wins;

Context::Context(XEvent ev) : xev(ev){}

Core::Core() {

    err << "init start";
    d = XOpenDisplay(NULL);

    if ( d == nullptr )
        err << "Failed to open display!";


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
    focus->button = Button1Mask | Button2Mask | Button3Mask;
    focus->mod = AnyModifier;
    focus->active = true;

    auto f = [] (Context *ctx){
        err << "focusing window";
        auto xev = ctx->xev.xbutton;
        auto w = wins->findWindow(xev.window);
        if(w)
            wins->focusWindow(w);
    };

    focus->action = f;
    addBut(focus);

    background = std::make_shared<__FireWindow>();

    err << "Init ended";
}

Core::Move::Move(Core *c) {
    win = nullptr;
    hook = Hook{ false,
            std::bind(std::mem_fn(&Core::Move::Intermediate), this)
            };
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

Core::Resize::Resize(Core *c) {
    win = nullptr;

    hook = Hook{ false,
            std::bind(std::mem_fn(&Core::Resize::Intermediate), this)
            };
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
    XGrabKey(d, kb->key, kb->mod, root,
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

Core::~Core(){
    XCompositeReleaseOverlayWindow(d, overlay);
    XCloseDisplay(d);
}

void Core::setBackground(const char *path) {
    background->texture = GLXUtils::loadImage(const_cast<char*>(path));
    background->norender = false;

    OpenGLWorker::generateVAOVBO(0, height, width, -height,
            background->vao, background->vbo);

    background->type = WindowTypeDesktop;
    wins->addWindow(background);
}

FireWindow Core::findWindow(Window win) {
    return wins->findWindow(win);
}

void Core::addWindow(XCreateWindowEvent xev) {
    FireWindow w = std::make_shared<__FireWindow>();
    w->id = xev.window;

    err << "xev.window = " << xev.window;
    err << "xev.parent = " << xev.parent;

    if(xev.parent != root)
        w->transientFor = findWindow(xev.parent);

    w->xvi = nullptr;
    wins->addWindow(w);
}

float degree = 0;
void rotate(FireWindow w) {
    degree += 10;
    w->transform.rotation = glm::rotate(glm::mat4(),
            degree * float(M_PI) / 180.0f, glm::vec3(0.0f, 1.0f, 0.0f));

}

void Core::handleEvent(XEvent xev){
    switch(xev.type) {
        case Expose:
            redraw = true;
        case KeyPress: {

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

            err << "Crash not";
            XMapWindow(core->d, xev.xcreatewindow.window);
            err << "Crash not middle";
            addWindow(xev.xcreatewindow);

            redraw = true;
            err << "Crash not 2";
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
        case MapNotify: {

            auto w = wins->findWindow(xev.xmap.window);
            if(w == nullptr)
                break;

            w->norender = false;
            WinUtil::syncWindowAttrib(w);
            redraw = true;
            break;
        }
        case UnmapNotify: {

            auto w = wins->findWindow(xev.xunmap.window);
            if(w == nullptr)
                break;

            w->norender = true;
            redraw = true;
            break;
        }

        case ButtonPress: {
            err << "ButtonPress";


            mousex = xev.xbutton.x_root;
            mousey = xev.xbutton.y_root;

            for(auto bb : buttons) {
                err << (xev.xbutton.state & Mod1Mask);
                err << (xev.xbutton.button == Button1);
                err << (bb.second->mod & xev.xbutton.state);
                err << (bb.second->button & xev.xbutton.button);
                err << (bb.second->type == BindingTypePress);
                err << (bb.second->active);
                err << std::endl;

                if(bb.second->mod & xev.xbutton.state &&
                   bb.second->button == xev.xbutton.button)
                if(bb.second->type == BindingTypePress)
                if(bb.second->active)
                    bb.second->action(new Context(xev));
            }

            err << "Allowing replay";
            XAllowEvents(d, ReplayPointer, xev.xbutton.time);
            break;
        }
        case ButtonRelease:
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
        default:
            if(xev.type == damage + XDamageNotify)
                redraw = true;
            break;
    }
}

#define RefreshRate 60
void Core::loop(){

    std::lock_guard<std::mutex> lock(wmMutex);
    redraw = true;

    int cycle = 1000000 / RefreshRate;
    cycle -= 50;
    timeval before, after;
    gettimeofday(&before, 0);

    XEvent xev;

    while(true) {

        while(XPending(d)) {
            XNextEvent(d, &xev);
            handleEvent(xev);
        }

        gettimeofday(&after, 0);
        int diff = (after.tv_sec - before.tv_sec) * 1000000 +
            after.tv_usec - before.tv_usec;

        if(diff < cycle) {
            wait(cycle - diff);
            if(fd.revents & POLLIN)
                continue;
            fd.revents = 0;
        }
        else {
            for (auto hook : hooks)
                if(hook.second->active)
                    hook.second->action();

            if(redraw)
                renderAllWindows(),
                    redraw = false;
            before = after;
        }
    }
}

int Core::onOtherWmDetected(Display* d, XErrorEvent *xev) {
    CHECK_EQ(static_cast<int>(xev->error_code), BadAccess);
    wmDetected = true;
    return 0;
}

int Core::onXError(Display *d, XErrorEvent *xev) {

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

void Core::renderAllWindows() {
    OpenGLWorker::preStage();
    wins->renderWindows();
    GLXUtils::endFrame(overlay);
}

void Core::wait(int timeout) {
    poll(&fd, 1, timeout);
}

void Core::Move::Initiate(Context *ctx) {
    auto xev = ctx->xev.xbutton;
    auto w = WinUtil::getAncestor(wins->findWindow(xev.window));
    if(w){
        err << "moving";
        core->wins->focusWindow(w);
        win = w;

        release.active = true;
        hook.active = true;

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

    hook.active = false;
    release.active = false;

    auto xev = ctx->xev.xbutton;

    win->transform.translation = glm::mat4();

    int dx = xev.x_root - sx;
    int dy = xev.y_root - sy;

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

void Core::Resize::Initiate(Context *ctx) {
    if(!ctx)
        return;

    auto xev = ctx->xev.xbutton;
    auto w = WinUtil::getAncestor(core->wins->findWindow(xev.window));

    if(w){

        wins->focusWindow(w);
        win = w;
        hook.active = true;
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

    hook.active = false;
    release.active = false;

    win->transform.scalation = glm::mat4();
    win->transform.translation = glm::mat4();

    int dw = core->mousex - sx;
    int dh = core->mousey - sy;

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
    vx = std::get<0> (nPos);
    vy = std::get<1> (nPos);
}

Core *core;
