#include "../include/commonincludes.hpp"
#include "../include/opengl.hpp"
#include "../include/winstack.hpp"
#include "../include/wm.hpp"

#include <sstream>
#include <memory>

bool wmDetected;

Core *core;
int refreshrate;

class CorePlugin : public Plugin {
    public:
        void init() {
            options.insert(newIntOption("rrate", 100));
            options.insert(newIntOption("vwidth", 3));
            options.insert(newIntOption("vheight", 3));
            options.insert(newIntOption("fadeduration", 150));
            options.insert(newStringOption("background", ""));
            options.insert(newStringOption("shadersrc", ""));
            options.insert(newStringOption("pluginpath", ""));
            options.insert(newStringOption("plugins", ""));
        }
        void initOwnership() {
            owner->name = "core";
            owner->compatAll = true;
        }
        void updateConfiguration() {
            refreshrate = options["rrate"]->data.ival;
            Fade::duration = options["fadeduration"]->data.ival;
        }
};

PluginPtr plug;


Core::Core(int vx, int vy) {
    this->vx = vx;
    this->vy = vy;
}

void Core::init() {

    d = XOpenDisplay(NULL);

    if ( d == nullptr )
        std::cout << "Failed to open display!" << std::endl;

    root = DefaultRootWindow(d);
    fd.fd = ConnectionNumber(d);
    fd.events = POLLIN;

    XSelectInput(d, root, SubstructureNotifyMask);

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
       std::cout << "Another WM already running!\n", std::exit(-1);

    wins = new WinStack();

    using namespace std::placeholders;
    this->getWindowAtPoint =
        std::bind(std::mem_fn(&WinStack::findWindowAtCursorPosition),
                wins, _1, _2);

    XSetErrorHandler(&Core::onXError);
    overlay = XCompositeGetOverlayWindow(d, root);
    outputwin = GLXUtils::createNewWindowWithContext(overlay);

    enableInputPass(overlay);
    enableInputPass(outputwin);

    int dummy;
    XDamageQueryExtension(d, &damage, &dummy);

    cntHooks = 0;
    output = getMaximisedRegion();
    // enable compositing to be recognized by other programs
    Atom a;
    s0owner = XCreateSimpleWindow (d, root, 0, 0, 1, 1, 0, None, None);
    Xutf8SetWMProperties (d, s0owner,
            "xcompmgr", "xcompmgr",
            NULL, 0, NULL, NULL, NULL);

    a = XInternAtom (d, "_NET_WM_CM_S0", False);
    XSetSelectionOwner (d, a, s0owner, 0);

    run(const_cast<char*>("setxkbmap -model pc104 -layout us,bg -variant ,phonetic -option grp:alt_shift_toggle"));

    std::cout << "Made it to here" << std::endl;
    initDefaultPlugins();

    /* load core options */
    plug->owner = std::make_shared<_Ownership>();
    plug->initOwnership();
    plug->init();
    config->setOptionsForPlugin(plug);
    plug->updateConfiguration();

    std::cout << "loaded default plugins" << std::endl;
    loadDynamicPlugins();
    std::cout << "loaded dynamic plugins" << std::endl;

    for(auto p : plugins) {
        p->owner = std::make_shared<_Ownership>();
        p->initOwnership();
        regOwner(p->owner);
        p->init();
        config->setOptionsForPlugin(p);
        p->updateConfiguration();
    }
    vwidth = plug->options["vwidth"]->data.ival;
    vheight= plug->options["vheight"]->data.ival;

    WinUtil::init();
    GLXUtils::initGLX();
    OpenGLWorker::initOpenGL((*plug->options["shadersrc"]->data.sval).c_str());

    dmg = getMaximisedRegion();
    resetDMG = true;
    addExistingWindows();

    core->setBackground((*plug->options["background"]->data.sval).c_str());
}

void Core::enableInputPass(Window win) {
    XserverRegion region;
    region = XFixesCreateRegion(d, NULL, 0);
    XFixesSetWindowShapeRegion(d, win, ShapeBounding, 0, 0, 0);
    XFixesSetWindowShapeRegion(d, win, ShapeInput, 0, 0, region);
    XFixesDestroyRegion(d, region);
}

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
           children[i] != s0owner   &&
           children[i] != root       )

            addWindow(children[i]);

    for(int i = 0; i < size; i++) {
        auto w = findWindow(children[i]);
        if(w) mapWindow(w);
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

Region Core::getMaximisedRegion() {
    REGION r;
    r.rects = &r.extents;
    r.numRects = r.size = 1;
    r.extents.x1 = r.extents.y1 = 0;
    r.extents.x2 = width;
    r.extents.y2 = height;

    Region ret;
    ret = XCreateRegion();
    XUnionRegion(&r, ret, ret);
    return ret;
}

Region Core::getRegionFromRect(int tlx, int tly, int brx, int bry) {
    REGION r;
    r.rects = &r.extents;
    r.numRects = r.size = 1;
    r.extents.x1 = tlx;
    r.extents.y1 = tly;
    r.extents.x2 = brx;
    r.extents.y2 = bry;

    Region ret;
    ret = XCreateRegion();
    XUnionRegion(&r, ret, ret);
    return ret;
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

            backgrounds[i][j]->attrib.x = (j - vx) * width;
            backgrounds[i][j]->attrib.y = (i - vy) * height;
            backgrounds[i][j]->attrib.width  = width;
            backgrounds[i][j]->attrib.height = height;

            backgrounds[i][j]->type = WindowTypeDesktop;
            backgrounds[i][j]->updateVBO();
            backgrounds[i][j]->transform.color = glm::vec4(1,1,1,1);
            backgrounds[i][j]->updateRegion();
            wins->addWindow(backgrounds[i][j]);
        }
    }
    std::cout << "dort" << std::endl;

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
    auto status = XGetWMProtocols(d, win->id, &atoms, &cnt);
    Atom wmdelete = XInternAtom(d, "WM_DELETE_WINDOW", 0);
    Atom wmproto  = XInternAtom(d, "WM_PROTOCOLS", 0);

    if(status != 0) {
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
    } else
        XKillClient(d, win->id);

    wins->focusWindow(wins->getTopmostToplevel());
}

void Core::renderAllWindows() {
    XIntersectRegion(dmg, output, dmg);

    OpenGLWorker::preStage();
    wins->renderWindows();
    GLXUtils::endFrame(outputwin);

    if(resetDMG)
        XDestroyRegion(dmg),
        dmg = getRegionFromRect(0, 0, 0, 0);
}

void Core::wait(int timeout) {
    poll(&fd, 1, timeout / 1000); // convert from usec to msec
}

void Core::mapWindow(FireWindow win) {
    win->norender = false;
    win->damaged = true;
    new AnimationHook(new Fade(win), this);
    win->attrib.map_state = IsViewable;
    damageWindow(win);

    WinUtil::syncWindowAttrib(win);
    if(win->transientFor)
        wins->restackTransients(win->transientFor);
}

void Core::unmapWindow(FireWindow win) {
    win->attrib.map_state = IsUnmapped;
    new AnimationHook(new Fade(win, Fade::FadeOut), this);
}

void Core::regOwner(Ownership owner) {
    owners.insert(owner);
}

bool Core::activateOwner(Ownership owner) {

    if(!owner) {
        std::cout << "Error detected ?? calling with nullptr!!1" << std::endl;
        return false;
    }

    if(owner->active || owner->special) {
        owner->active = true;
        return true;
    }

    for(auto o : owners)
        if(o && o->active)
            if(o->compat.find(owner->name) ==
               o->compat.end() && !o->compatAll)
                return false;

    owner->active = true;
    return true;
}

bool Core::deactivateOwner(Ownership owner) {
    owner->ungrab();
    owner->active = false;
    return true;
}

bool Core::checkKey(KeyBinding *kb, XKeyEvent xkey) {
    if(!kb->active)
        return false;

    if(kb->key != xkey.keycode)
        return false;

    if(kb->mod != xkey.state)
        return false;

    return true;
}

bool Core::checkButPress(ButtonBinding *bb, XButtonEvent xb) {
    if(!bb->active)
        return false;

    if(bb->type != BindingTypePress)
        return false;

    if(!(bb->mod & xb.state) && !(bb->mod == NoMods && xb.state == 0))
        return false;

    if(bb->button != xb.button)
        return false;

    return true;
}

bool Core::checkButRelease(ButtonBinding *bb, XButtonEvent kb) {
    if(!bb->active)
        return false;

    if(bb->type != BindingTypeRelease)
        return false;

    return true;
}

void Core::handleEvent(XEvent xev){
    switch(xev.type) {
        case Expose:
            dmg = getMaximisedRegion();
        case KeyPress: {
            // check keybindings
            for(auto kb : keys)
                if(checkKey(kb.second, xev.xkey))
                   kb.second->action(new Context(xev));
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
            focusWindow(findWindow(xev.xcreatewindow.window));
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
                if(checkButPress(bb.second, xev.xbutton)) {
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
                if(checkButRelease(bb.second, xev.xbutton))
                    bb.second->action(new Context(xev));

            XAllowEvents(d, ReplayPointer, xev.xbutton.time);
            break;

        case MotionNotify:
            mousex = xev.xmotion.x_root;
            mousey = xev.xmotion.y_root;
            break;

        case ConfigureRequest: {
            auto w = findWindow(xev.xconfigurerequest.window);
            if(!w) { // from compiz window manager
                XWindowChanges xwc;
                std::memset(&xwc, 0, sizeof(xwc));

                auto xwcm = xev.xconfigurerequest.value_mask &
                    (CWX | CWY | CWWidth | CWHeight | CWBorderWidth);

                xwc.x        = xev.xconfigurerequest.x;
                xwc.y        = xev.xconfigurerequest.y;
                xwc.width        = xev.xconfigurerequest.width;
                xwc.height       = xev.xconfigurerequest.height;
                xwc.border_width = xev.xconfigurerequest.border_width;

                XConfigureWindow (d, xev.xconfigurerequest.window,
                        xwcm, &xwc);
                break;
            }

            int width = w->attrib.width;
            int height = w->attrib.height;
            int x = w->attrib.x;
            int y = w->attrib.y;

            if(xev.xconfigurerequest.value_mask & CWWidth)
                width = xev.xconfigurerequest.width;
            if(xev.xconfigurerequest.value_mask & CWHeight)
                height = xev.xconfigurerequest.height;
            if(xev.xconfigurerequest.value_mask & CWX)
                x = xev.xconfigurerequest.x;
            if(xev.xconfigurerequest.value_mask & CWY)
                y = xev.xconfigurerequest.y;

            WinUtil::moveWindow(w, x, y);
            WinUtil::resizeWindow(w, width, height);

            if(xev.xconfigurerequest.value_mask & CWStackMode) {
                if(xev.xconfigurerequest.above) {
                    auto below = findWindow(xev.xconfigurerequest.above);
                    if(below) {
                        if(xev.xconfigurerequest.detail == Above)
                            wins->restackAbove(w, below);
                        else
                            wins->restackAbove(below, w);
                    }
                }
                else
                    if(xev.xconfigurerequest.detail == Above)
                        focusWindow(w);
            }

        }

        case PropertyNotify: {
            auto w = findWindow(xev.xproperty.window);
            if(!w)
                break;

            if(xev.xproperty.atom == winTypeAtom)
                w->type = WinUtil::getWindowType(w),
                wins->restackTransients(w);

            if(xev.xproperty.atom == XA_WM_TRANSIENT_FOR)
                w->transientFor = WinUtil::getTransient(w),
                wins->restackTransients(w);

            if(xev.xproperty.atom == wmClientLeaderAtom)
                w->leader = WinUtil::getClientLeader(w),
                wins->restackTransients(w);

            break;
        }

        case ConfigureNotify:
            if(xev.xconfigure.window == root)
                terminate = true, mainrestart = true;
        case EnterNotify:       // we don't handle
        case FocusIn:           // any of these
        case CirculateRequest:
        case CirculateNotify:
        case MappingNotify:
            break;

        default:
            if(xev.type == damage + XDamageNotify) {
                XDamageNotifyEvent *x =
                    reinterpret_cast<XDamageNotifyEvent*> (&xev);

                auto w = findWindow(x->drawable);
                if(!w)
                    return;

                w->damaged = true;

                Region damagedArea = getRegionFromRect(
                        x->area.x + w->attrib.x,
                        x->area.y + w->attrib.y,
                        x->area.x + w->attrib.x + x->area.width,
                        x->area.y + w->attrib.y + x->area.height);


                if(!dmg)
                    std::cout << "dmg is null!!!!" << std::endl;

                XUnionRegion(damagedArea, dmg, dmg);
                XDestroyRegion(damagedArea);
            break;
            }
    }
}

#define RefreshRate 61
#define Second 1000000
#define MaxDelay 1000
#define MinRR 61

void Core::loop(){

    int currentCycle = Second / plug->options["rrate"]->data.ival;
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

            if(__FireWindow::allDamaged || !XEmptyRegion(dmg))
                renderAllWindows();   // we just redraw

            /* optimisation when too slow,
             * so we can update more rarely,
             * i.e reduce lagging */
            if(diff - currentCycle > MaxDelay &&
                    Second / MinRR <= currentCycle)
                currentCycle += 2000; // 2ms slower redraws

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

    auto view = getRegionFromRect((x - vx) * width, (y - vy) * height,
                       (x - vx + 1) * width, (y - vy + 1) * height);

    std::vector<FireWindow> ret;
    Region tmp = XCreateRegion();
    for(auto w : wins->wins) {
        if(!w->region)
            continue;
        if(!view)
            continue;

        XIntersectRegion(view, w->region, tmp);
        if(tmp && !XEmptyRegion(tmp) && !w->norender)
            ret.push_back(w);
    }
    XDestroyRegion(view);
    XDestroyRegion(tmp);

    return ret;
}

void Core::damageWindow(FireWindow win) {
    if(!win)
        return;

    if(win->norender)
        return;

    if(!win->region) {

        XserverRegion reg =
            XFixesCreateRegionFromWindow(core->d, win->id,
                    WindowRegionBounding);

        XDamageAdd(core->d, win->id, reg);
        XFixesDestroyRegion(core->d, reg);
        return;
    }
    if(!dmg)
        dmg = getRegionFromRect(0, 0, 0, 0);

    XUnionRegion(dmg, win->region, dmg);
}

namespace {
    int fullRedraw = 0;
}

void Core::setRedrawEverything(bool val) {
    if(val) {
        fullRedraw++;
        __FireWindow::allDamaged = true;
        core->resetDMG = false;
    }
    else if(--fullRedraw == 0)
        __FireWindow::allDamaged = false,
        core->resetDMG = true;
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

template<class T>
PluginPtr Core::createPlugin() {
    return std::static_pointer_cast<Plugin>(std::make_shared<T>());
}

int Core::getRefreshRate() {
    return refreshrate;
}

namespace {
    template<class A, class B> B unionCast(A object) {
        union {
            A x;
            B y;
        } helper;
        helper.x = object;
        return helper.y;
    }
}

PluginPtr Core::loadPluginFromFile(std::string path) {
    void *handle = dlopen(path.c_str(), RTLD_LAZY);
    if(handle == NULL){
        std::cout << "Error loading plugin " << path << std::endl;
        std::cout << dlerror() << std::endl;
        return nullptr;
    }

    auto initptr = dlsym(handle, "newInstance");
    if(initptr == NULL) {
        std::cout << "Failed to load newInstance from file " <<
            path << std::endl;
        std::cout << dlerror();
        return nullptr;
    }
    LoadFunction init = unionCast<void*, LoadFunction>(initptr);
    return std::shared_ptr<Plugin>(init());
}

void Core::loadDynamicPlugins() {
    std::stringstream stream(*plug->options["plugins"]->data.sval);
    auto path = *plug->options["pluginpath"]->data.sval;

    std::string plugin;
    while(stream >> plugin){
        if(plugin != "") {
            auto ptr = loadPluginFromFile(path + "/" + plugin + ".so");
            if(ptr) plugins.push_back(ptr);
        }
    }
}

void Core::initDefaultPlugins() {
    plug = createPlugin<CorePlugin>();
    plugins.push_back(createPlugin<Focus>());
    plugins.push_back(createPlugin<Exit>());
    plugins.push_back(createPlugin<Run>());
    plugins.push_back(createPlugin<Close>());
    plugins.push_back(createPlugin<Refresh>());
}
