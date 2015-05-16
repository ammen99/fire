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
Core *core;

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

    err << "Setting windows";
    XSetErrorHandler(&Core::onXError);
    overlay = XCompositeGetOverlayWindow(d, root);
    outputwin = GLXUtils::createNewWindowWithContext(overlay, this);

    err << "Created output window";
    enableInputPass(overlay);
    enableInputPass(outputwin);

    int dummy;
    XDamageQueryExtension(d, &damage, &dummy);

    move     = new Move(this);
    resize   = new Resize(this);
    wsswitch = new WSSwitch(this);
    expo     = new Expo(this);
    focus    = new Focus(this);
    exit     = new Exit(this);
    runn     = new Run(this);
    close    = new Close(this);

    cntHooks = 0;
    output = Rect(0, 0, width, height);

    Window w;
    Atom a;

    w = XCreateSimpleWindow (d, root, 0, 0, 1, 1, 0, None, None);
    Xutf8SetWMProperties (d, w, "xcompmgr", "xcompmgr",NULL,0,NULL,NULL,NULL);
    a = XInternAtom (d, "_NET_WM_CM_S0", False);
    XSetSelectionOwner (d, a, w, 0);
}

void Core::enableInputPass(Window win) {
    XserverRegion region;
    region = XFixesCreateRegion(d, NULL, 0);
    XFixesSetWindowShapeRegion(d, win, ShapeBounding, 0, 0, 0);
    XFixesSetWindowShapeRegion(d, win, ShapeInput, 0, 0, region);
    XFixesDestroyRegion(d, region);
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

    this->run(const_cast<char*>(std::string("feh --bg-scale ")
                .append(path).c_str()));

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

FireWindow Core::getActiveWindow() {
    return wins->activeWin;
}

void Core::addWindow(XCreateWindowEvent xev) {
    FireWindow w = std::make_shared<__FireWindow>();
    w->id = xev.window;

    if(xev.parent != root)
        w->transientFor = findWindow(xev.parent);

    w->xvi = nullptr;
    wins->addWindow(w);
}
void Core::destroyWindow(FireWindow win) {
    if(!win)
        return;

    int cnt;
    Atom *atoms;
    XGetWMProtocols(d, win->id, &atoms, &cnt);
    Atom wmdelete = XInternAtom(d, "WM_DELETE_WINDOW", 0);
    Atom wmproto  = XInternAtom(d, "WM_PROTOCOLS", 0);

    bool send = false; // should we send a wm_delete_window?
    for(int i = 0; i < cnt; i++)
        if(atoms[i] == wmdelete)
            send = true;

    if ( send ) {
        XEvent xev;

        xev.type         = ClientMessage;
        xev.xclient.window       = win->id;
        xev.xclient.message_type = wmproto;
        xev.xclient.format       = 32;
        xev.xclient.data.l[0]    = wmdelete;
        xev.xclient.data.l[1]    = CurrentTime;
        xev.xclient.data.l[2]    = 0;
        xev.xclient.data.l[3]    = 0;
        xev.xclient.data.l[4]    = 0;

        XSendEvent ( d, win->id, FALSE, NoEventMask, &xev );
    } else
        XKillClient ( d, win->id );
}

void Core::renderAllWindows() {
    OpenGLWorker::preStage();
    wins->renderWindows();
    GLXUtils::endFrame(outputwin);
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

            if(xev.xcreatewindow.window == outputwin)
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
