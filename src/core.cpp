#include "../include/commonincludes.hpp"
#include "../include/opengl.hpp"
#include "../include/winstack.hpp"
#include "../include/wm.hpp"

#include <sstream>
#include <memory>

bool wmDetected;
std::mutex wmMutex;


Core *core;
std::fstream err;


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

Core::Core(int vx, int vy) {

    err.open("/home/ilex/work/cwork/fire/log2",
            std::ios::out | std::ios::trunc);

    if(!err.is_open())
        std::cout << "Failed to open debug output, exiting" << std::endl,
        std::exit(0);

    err << "Creating new Core" << std::endl;

    d = XOpenDisplay(NULL);

    if ( d == nullptr )
        err << "Failed to open display!" << std::endl;


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
    outputwin = GLXUtils::createNewWindowWithContext(overlay, this);

    enableInputPass(overlay);
    enableInputPass(outputwin);

    int dummy;
    XDamageQueryExtension(d, &damage, &dummy);

    cntHooks = 0;
    output = Rect(0, 0, width, height);

    initDefaultPlugins();

    // enable compositing to be recognized by other programs
    Atom a;
    s0owner = XCreateSimpleWindow (d, root, 0, 0, 1, 1, 0, None, None);
    Xutf8SetWMProperties (d, s0owner,
            "xcompmgr", "xcompmgr",
            NULL, 0, NULL, NULL, NULL);

    a = XInternAtom (d, "_NET_WM_CM_S0", False);
    XSetSelectionOwner (d, a, s0owner, 0);

    run(const_cast<char*>("setxkbmap -model pc104 -layout us,bg -variant ,phonetic -option grp:alt_shift_toggle"));

    WinUtil::init(this);
    GLXUtils::initGLX(this);
    OpenGLWorker::initOpenGL(this, "/home/ilex/work/cwork/fire/shaders");
    resetDMG = true;

    core = this;
    addExistingWindows();


    vwidth = vheight = 3;
    this->vx = vx;
    this->vy = vy;

    for(auto p : plugins)
        p->init(this);

    dmg = Rect(0, 0, width, height);
}

void Core::enableInputPass(Window win) {
    XserverRegion region;
    region = XFixesCreateRegion(d, NULL, 0);
    XFixesSetWindowShapeRegion(d, win, ShapeBounding, 0, 0, 0);
    XFixesSetWindowShapeRegion(d, win, ShapeInput, 0, 0, region);
    XFixesDestroyRegion(d, region);
}
;
void Core::addExistingWindows() {
    Window dummy1, dummy2;
    uint size;
    Window  *children;

    XQueryTree(d, root, &dummy1, &dummy2, &children, &size);

    if(size == 0)
        return;

    for(int i = size - 1; i >= 0; i--)
        if(children[i] != overlay   &&
           children[i] != outputwin &&
           children[i] != s0owner    )

            addWindow(children[i]);

    for(int i = 0; i < size; i++) {
        auto w = findWindow(children[i]);
        if(w)
            w->transientFor = WinUtil::getTransient(w);
    }

    for(auto i = 0; i < size; i++) {
        auto w = findWindow(children[i]);
        if(w)
            WinUtil::initWindow(w);
    }

}
Core::~Core(){

    for(auto p : plugins)
        p.reset();

    int nx, ny;
    for(auto w : wins->wins) {
        nx = w->attrib.x % width;
        ny = w->attrib.y % height;

        nx += width; ny += height;
        ny %= width; ny %= height;

        WinUtil::moveWindow(w, nx, ny);
    }

    XDestroyWindow(core->d, outputwin);
    XDestroyWindow(core->d, s0owner);
    XCompositeReleaseOverlayWindow(d, overlay);
    XCloseDisplay(d);

    err.close();
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

            backgrounds[i][j]->attrib.x = (i - vx) * width;
            backgrounds[i][j]->attrib.y = (j - vy) * height;
            backgrounds[i][j]->attrib.width  = width;
            backgrounds[i][j]->attrib.height = height;

            backgrounds[i][j]->type = WindowTypeDesktop;
            backgrounds[i][j]->regenVBOFromAttribs();
            backgrounds[i][j]->transform.color = glm::vec4(1,1,1,1);
            wins->addWindow(backgrounds[i][j]);
        }
    }

}

FireWindow Core::findWindow(Window win) {
    if(win == 0)
        return nullptr;
    return wins->findWindow(win);
}

FireWindow Core::getActiveWindow() {
    return wins->activeWin;
}

void Core::addWindow(XCreateWindowEvent xev) {
    FireWindow w = std::make_shared<__FireWindow>();
    w->id = xev.window;

    if(xev.parent != root && xev.parent != 0)
        w->transientFor = findWindow(xev.parent);

    w->xvi = nullptr;
    w->keepCount = 0;

    wins->addWindow(w);
    WinUtil::initWindow(w);

    if(w->type != WindowTypeWidget)
        wins->focusWindow(w);
}
void Core::addWindow(Window id) {
    XCreateWindowEvent xev;
    xev.window = id;
    xev.parent = 0;
    addWindow(xev);
}
void Core::closeWindow(FireWindow win) {
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

    wins->focusWindow(wins->getTopmostToplevel());
}

void Core::renderAllWindows() {
    OpenGLWorker::preStage();
    wins->renderWindows();
    GLXUtils::endFrame(outputwin);
}

void Core::wait(int timeout) {
    poll(&fd, 1, timeout / 1000); // convert from usec to msec
}

void Core::mapWindow(FireWindow win) {
    win->norender = false;
    new AnimationHook(new Fade(win), this);
    win->attrib.map_state = IsViewable;
    damageWindow(win);

    WinUtil::syncWindowAttrib(win);
    auto parent = WinUtil::getAncestor(win);
    wins->restackTransients(parent);
}

void Core::unmapWindow(FireWindow win) {
    win->attrib.map_state = IsUnmapped;
    new AnimationHook(new Fade(win, Fade::FadeOut), this);
}

void Core::handleEvent(XEvent xev){
    switch(xev.type) {
        case Expose:
            dmg = Rect(0, 0, width, height);
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

            auto it = findWindow(xev.xcreatewindow.window);
            if(it != nullptr) {
                WinUtil::finishWindow(it);
                wins->removeWindow(it);
            }

            addWindow(xev.xcreatewindow);
            mapWindow(findWindow(xev.xcreatewindow.window));
            break;
        }
        case DestroyNotify: {
            auto w = wins->findWindow(xev.xdestroywindow.window);
            if(w == nullptr)
                break;

            w->destroyed = true;
            break;
        }
        case MapRequest: {
            auto w = wins->findWindow(xev.xmaprequest.window);
            if(w == nullptr)
                break;
            mapWindow(w);
            break;
        }
        case MapNotify: {
            auto w = wins->findWindow(xev.xmap.window);
            if(w == nullptr)
                break;

            mapWindow(w);
            break;
        }
        case UnmapNotify: {
            auto w = wins->findWindow(xev.xunmap.window);
            if(w == nullptr)
                break;
            unmapWindow(w);
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
                if(bb.second->button == xev.xbutton.button) {// and we're
                    bb.second->action(new Context(xev));
                    break;
                }

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
        case EnterNotify:
            break;
        default:
            if(xev.type == damage + XDamageNotify) {
                redraw = true;
                XDamageNotifyEvent *x =
                    reinterpret_cast<XDamageNotifyEvent*> (&xev);

                auto w = findWindow(x->drawable);
                if(!w)
                    return;

                auto damagedArea =
                    Rect(x->area.x + w->attrib.x,
                         x->area.y + w->attrib.y,
                         x->area.x + w->attrib.x + x->area.width,
                         x->area.y + w->attrib.y + x->area.height);

                if(!__FireWindow::allDamaged)
                    if(damagedArea & output)
                    dmg = dmg + damagedArea;
            }
            break;
    }
}

#define RefreshRate 200
#define Second 1000000
#define MaxDelay 1000
#define MinRR 61

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

    while(!terminate) {
        /* handle current events */
        while(XPending(d)) {
            XNextEvent(d, &xev);
            handleEvent(xev);
        }

        gettimeofday(&after, 0);
        int diff = (after.tv_sec - before.tv_sec) * 1000000 +
            after.tv_usec - before.tv_usec;

        if(diff < currentCycle) {     // we have time to next redraw, wait
            wait(currentCycle - diff);// for events

            if(fd.revents & POLLIN || !resetDMG) { /* disable optimisation */
                hadEvents = true;
                currentCycle = baseCycle;
                continue;
            }
            fd.revents = 0;
        }
        else {
            if(cntHooks) { // if running hooks, run them
                for (auto hook : hooks)
                    if(hook.second->getState())
                        hook.second->action();
            }

            if(redraw ||
                    ((dmg.brx - dmg.tlx != 0 &&  // if we have damage
                      dmg.bry - dmg.tly != 0) ||
                        __FireWindow::allDamaged)) // or we just redraw
                renderAllWindows(),                // everything
                redraw = false;

            /* optimisation when too slow,
             * so we can update more rarely,
             * i.e reduce lagging */
            if(diff - currentCycle > MaxDelay && Second / MinRR <= currentCycle)
                currentCycle += 2000; // 1ms slower redraws

            /* optimisation when idle */
            if(!cntHooks && !hadEvents && currentCycle < Second && resetDMG)
                currentCycle *= 2;

            before = after;
            hadEvents = false;
        }
    }
}

int Core::onOtherWmDetected(Display* d, XErrorEvent *xev) {
    if(static_cast<int>(xev->error_code) == BadAccess)
        wmDetected = true;
    return 0;
}

int Core::onXError(Display *d, XErrorEvent *xev) {
    if(xev->resourceid == 0) // invalid window
        return 0;

    return 0;
    /* some of the calls to obtain a texture from this window
     * have failed, so don't draw it the next time */

    if(xev->error_code == BadMatch    ||
       xev->error_code == BadDrawable ||
       xev->error_code == BadWindow   ){

       std::cout << "caught BadMatch/Drawable/Window. Disabling window drawing "
            << xev->resourceid;

        auto x = core->wins->findWindow(xev->resourceid);
        if (x != nullptr)
            x->norender = true;

        return 0;
    }

    std::cout << std::endl << "____________________________" << std::endl;
    std::cout << "XError code   " << int(xev->error_code) << std::endl;
    char buf[512];
    XGetErrorText(d, xev->error_code, buf, 512);
    std::cout << "XError string " << buf << std::endl;
    std::cout << "ResourceID = " << xev->resourceid << std::endl;
    std::cout << "____________________________" << std::endl << std::endl;
    return 0;
}

std::tuple<int, int> Core::getWorkspace() {
    return std::make_tuple(vx, vy);
}

std::tuple<int, int> Core::getWorksize() {
    return std::make_tuple(vwidth, vheight);
}

std::tuple<int, int> Core::getScreenSize() {
    return std::make_tuple(width, height);
}

std::tuple<int, int> Core::getMouseCoord() {
    return std::make_tuple(mousex, mousey);
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

    auto ws = getWindowsOnViewport(this->getWorkspace());
    if(ws.size() != 0)
        wins->focusWindow(ws[0]);
}

std::vector<FireWindow> Core::getWindowsOnViewport(std::tuple<int, int> vp) {
    auto x = std::get<0>(vp);
    auto y = std::get<1>(vp);

    Rect view((x - vx    ) * width, (y - vy    ) * height,
            (x - vx + 1) * width, (y - vy + 1) * height);

    std::vector<FireWindow> ret;
    for(auto w : wins->wins)
        if(w->getRect() & view && !w->norender)
            ret.push_back(w);

    return ret;
}

void Core::damageWindow(FireWindow win) {
    if(!win)
        return;

    if(win->norender)
        return;

    XserverRegion reg =
        XFixesCreateRegionFromWindow(core->d, win->id,
                WindowRegionBounding);

    XDamageAdd(core->d, win->id, reg);

    XFixesDestroyRegion(core->d, reg);
}

void Core::focusWindow(FireWindow win) {
    if(!win)
        return;

    damageWindow(win);
    wins->focusWindow(win);
}

void Core::removeWindow(FireWindow win) {
    WinUtil::finishWindow(win);
    wins->removeWindow(win);
}

FireWindow Core::getWindowAtPoint(Point p) {
    return wins->findWindowAtCursorPosition(p);
}

template<class T>
PluginPtr Core::createPlugin() {
    return std::static_pointer_cast<Plugin>(std::make_shared<T>());
}

void Core::initDefaultPlugins() {
    plugins.push_back(createPlugin<Move>());
    plugins.push_back(createPlugin<Resize>());
    plugins.push_back(createPlugin<WSSwitch>());
    plugins.push_back(createPlugin<Expo>());
    plugins.push_back(createPlugin<Focus>());
    plugins.push_back(createPlugin<Exit>());
    plugins.push_back(createPlugin<Run>());
    plugins.push_back(createPlugin<Close>());
    plugins.push_back(createPlugin<ATSwitcher>());
    plugins.push_back(createPlugin<Grid>());
    plugins.push_back(createPlugin<RefreshWin>());
}
