#include "../include/commonincludes.hpp"
#include "../include/core.hpp"
#include "../include/opengl.hpp"
#include "../include/winstack.hpp"

#include <glog/logging.h>

bool wmDetected;
std::mutex wmMutex;

WinStack *Core::wins;


Core::Core() {

    err << "init start";
    d = XOpenDisplay(NULL);

    if ( d == nullptr ) 
        err << "Failed to open display!";


    //XSynchronize(d, True);
    root = DefaultRootWindow(d);
    fd.fd = ConnectionNumber(d);
    fd.events = POLLIN;

    this->moving = false;

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

    XGrabKey(d, XKeysymToKeycode(d, XK_a), ControlMask,
            root, FALSE,
            GrabModeAsync, GrabModeAsync);

    XGrabKey(d, XKeysymToKeycode(d, XK_s), ControlMask,
            root, FALSE,
            GrabModeAsync, GrabModeAsync);

    XGrabKey(d, XKeysymToKeycode(d, XK_q), ControlMask,
            root, FALSE,
            GrabModeAsync, GrabModeAsync);

    err << "Init ended";
}
 
Core::~Core(){
    XCompositeReleaseOverlayWindow(d, overlay);
    XCloseDisplay(d);
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
            if(xev.xkey.keycode == XKeysymToKeycode(d, XK_q))
                err << "Bye!!", std::exit(0);
            if(xev.xkey.keycode == XKeysymToKeycode(d, XK_s))
                OpenGLWorker::transformed = !OpenGLWorker::transformed; 
            if(xev.xkey.keycode == XKeysymToKeycode(d, XK_a)){
                if(wins->activeWin)
                    rotate(wins->activeWin);
            }

            err << "KeyPress";
            redraw = true;
            break;
        }


        case CreateNotify: {
            if (xev.xcreatewindow.window == overlay)
                break;

            err << "Creating windows";
            XMapWindow(core->d, xev.xcreatewindow.window);
            addWindow(xev.xcreatewindow);

            redraw = true;
            break;
        }
        case DestroyNotify: {
            auto w = wins->findWindow(xev.xdestroywindow.window);
            if ( w == nullptr )
                break;

            err << "Destroy Notify";
            wins->removeWindow(w, true);
            redraw = true;
            break;
        }
        case MapNotify: {
            err << "MapNotify";
            auto w = wins->findWindow(xev.xmap.window);
            if(w == nullptr)
                break;

            w->norender = false;
            WindowWorker::syncWindowAttrib(w);
            redraw = true;
            break;
        }
        case UnmapNotify: {
            err << "UnmapNotify";
            auto w = wins->findWindow(xev.xunmap.window);
            if(w == nullptr)
                break;

            w->norender = true;
            redraw = true;
            break;
        }

        case ButtonPress: {
            if(xev.xbutton.button == Button1 &&
               xev.xbutton.state  == Mod1Mask ){
                moveInitiate(xev.xbutton);    
                break;
            }

            if(xev.xbutton.button == Button1 &&
                    xev.xbutton.state  == ControlMask ){
                resizeInitiate(xev.xbutton);
                break;
            }

            else if(xev.xbutton.button == Button1 ||
                    xev.xbutton.button == Button2 ||
                    xev.xbutton.button == Button3 ){

                auto w = wins->findWindow(xev.xbutton.window);                  
                if(w != nullptr)
                    wins->focusWindow(w);
            }
            XAllowEvents(d, ReplayPointer, xev.xbutton.time);
            break;
        }
        case ButtonRelease:

            if(moving)
                moveTerminate(xev.xbutton);

            if(resizing)
                resizeTerminate(xev.xbutton);

            XAllowEvents(d, ReplayPointer, xev.xbutton.time);
            break;

        case MotionNotify:
            if(moving)
                moveIntermediate(xev.xmotion);
            if(resizing)
                resizeIntermediate(xev.xmotion);
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
            if(redraw)
                renderAllWindows();
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
    GLXWorker::endFrame(overlay);
}

void Core::wait(int timeout) {
    poll(&fd, 1, timeout);
}


void Core::moveInitiate(XButtonPressedEvent xev) {
    auto w = WindowWorker::getAncestor(wins->findWindow(xev.window));
    if(w){
        wins->focusWindow(w);
        this->moving = true;
        operatingWin = w;

        this->sx = xev.x_root;
        this->sy = xev.y_root;

        XGrabPointer(d, overlay, TRUE,
                ButtonPressMask | ButtonReleaseMask |
                PointerMotionMask,
                GrabModeAsync, GrabModeAsync,
                root, None, CurrentTime);
    }
}

void Core::moveTerminate(XButtonPressedEvent xev) {
    moving = false;
    operatingWin->transform.translation = glm::mat4();
    err << xev.x_root << " " << xev.y_root;
    int dx = xev.x_root - sx;
    int dy = xev.y_root - sy;

    int nx = operatingWin->attrib.x + dx;
    int ny = operatingWin->attrib.y + dy;
    WindowWorker::moveWindow(operatingWin, nx, ny);
    XUngrabPointer(core->d, CurrentTime);

    err << dx << " " << dy;
    wins->focusWindow(operatingWin);
    redraw = true; 
}

void Core::moveIntermediate(XMotionEvent xev) {
    operatingWin->transform.translation =
        glm::translate(glm::mat4(),
                glm::vec3(float(xev.x_root - sx) / float(width / 2.0),
                    float(sy - xev.y_root) / float(height / 2.0),
                    0.f));
    redraw = true;
}



void Core::resizeInitiate(XButtonPressedEvent xev) {
    auto w = WindowWorker::getAncestor(wins->findWindow(xev.window));
    if(w){
        wins->focusWindow(w);
        this->resizing = true;
        operatingWin = w;
        if(w->attrib.width == 0)
            w->attrib.width = 1;
        if(w->attrib.height == 0)
            w->attrib.height = 1;

        this->sx = xev.x_root;
        this->sy = xev.y_root;

        XGrabPointer(d, overlay, TRUE,
                ButtonPressMask | ButtonReleaseMask |
                PointerMotionMask,
                GrabModeAsync, GrabModeAsync,
                root, None, CurrentTime);
    }
}

void Core::resizeTerminate(XButtonPressedEvent xev) {
    resizing = false;

    operatingWin->transform.scalation = glm::mat4();

    int dw = xev.x_root - sx;
    int dh = xev.y_root - sy;

    int nw = operatingWin->attrib.width  + dw;
    int nh = operatingWin->attrib.height + dh;
    WindowWorker::resizeWindow(operatingWin, nw, nh);

    XUngrabPointer(core->d, CurrentTime);

    wins->focusWindow(operatingWin);
    redraw = true; 
}

void Core::resizeIntermediate(XMotionEvent xev) {

    int dw = xev.x_root - sx;
    int dh = xev.y_root - sy;

    int nw = operatingWin->attrib.width  + dw;
    int nh = operatingWin->attrib.height + dh;

    operatingWin->transform.scalation = 
        glm::scale(glm::mat4(), glm::vec3(
                   float(nw) / float(operatingWin->attrib.width ),
                   float(nh) / float(operatingWin->attrib.height),
                   1.f));
    redraw = true;
}
Core *core;

