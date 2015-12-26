#include <core.hpp>
#include <opengl.hpp>


/* misc definitions */

Atom winTypeAtom, winTypeDesktopAtom, winTypeDockAtom,
     winTypeToolbarAtom, winTypeMenuAtom, winTypeUtilAtom,
     winTypeSplashAtom, winTypeDialogAtom, winTypeNormalAtom,
     winTypeDropdownMenuAtom, winTypePopupMenuAtom, winTypeDndAtom,
     winTypeTooltipAtom, winTypeNotificationAtom, winTypeComboAtom;

Atom winStateAtom, winStateModalAtom, winStateStickyAtom,
     winStateMaximizedVertAtom, winStateMaximizedHorzAtom,
     winStateShadedAtom, winStateSkipTaskbarAtom,
     winStateSkipPagerAtom, winStateHiddenAtom,
     winStateFullscreenAtom, winStateAboveAtom,
     winStateBelowAtom, winStateDemandsAttentionAtom,
     winStateDisplayModalAtom;

Atom activeWinAtom;
Atom wmTakeFocusAtom;
Atom wmProtocolsAtom;
Atom wmClientLeaderAtom;
Atom wmNameAtom;
Atom winOpacityAtom;
Atom clientListAtom;

glm::mat4 Transform::grot;
glm::mat4 Transform::gscl;
glm::mat4 Transform::gtrs;

Region copyRegion(Region reg) {
    if(!reg) {
        auto tmp = XCreateRegion();
        std::memset(tmp, 0, sizeof(REGION));
        tmp->numRects = 1;
        return tmp;
    }

    Region newreg = XCreateRegion();
    newreg->rects = new BOX[reg->numRects];
    newreg->size = reg->size;
    newreg->numRects = reg->numRects;
    newreg->extents = reg->extents;
    std::memcpy(newreg->rects, reg->rects, reg->numRects * sizeof(BOX));
    return newreg;
}

Transform::Transform() {
    this->translation = glm::mat4();
    this->rotation = glm::mat4();
    this->scalation = glm::mat4();
}

glm::mat4 Transform::compose() {
    return (gtrs*translation)*(grot*rotation)*(gscl*scalation);
}

Region output;
bool FireWin::allDamaged = false;


FireWin::FireWin(Window id, bool init) {
    this->id = id;
    if(!init) return;

    auto status = XGetWindowAttributes(core->d, id, &attrib);
    if(status != Success)
        norender = true;

    XSizeHints hints;
    long flags;
    XGetWMNormalHints(core->d, id, &hints, &flags);

    if(flags & USPosition)
        attrib.x = hints.x,
        attrib.y = hints.y;

    if(flags & USSize)
        attrib.width = hints.width,
        attrib.height= hints.height;

    else if(flags & PBaseSize)
        attrib.width = hints.base_width,
        attrib.height = hints.base_height;

    updateRegion();

    if(attrib.c_class == InputOutput)
        damagehnd = XDamageCreate(core->d, id, XDamageReportRawRectangles);
    else
        attrib.map_state = IsUnmapped,
        damagehnd = None;

    type         = WinUtil::getWindowType(id);
    transientFor = WinUtil::getTransient(id);
    leader       = WinUtil::getClientLeader(id);
    state        = WinUtil::getWindowState(id);
    updateState();

    XSelectInput(core->d, id,   FocusChangeMask  |
                PropertyChangeMask | EnterWindowMask);

    XGrabButton (core->d, AnyButton, AnyModifier, id, TRUE,
            ButtonPressMask | ButtonReleaseMask | Button1MotionMask,
            GrabModeSync, GrabModeSync, None, None);

    transform.color = glm::vec4(1., 1., 1., 1.);
    transform.color[3] =
        WinUtil::readProp(id, winOpacityAtom, 0xffff) / 0xffff;

    glGenTextures(1, &texture);
    keepCount = 0;
}

FireWin::~FireWin() {
    OpenGL::API.glDeleteTextures(1, &texture);
    OpenGL::API.glDeleteBuffers(1, &vbo);
    OpenGL::API.glDeleteVertexArrays(1, &vao);

    for(auto d : data)
        delete d.second;
    data.clear();
}

#define Mod(x,m) (((x)%(m)+(m))%(m))


void FireWin::updateVBO() {
    if(this->disableVBOChange)
        return;

    if(type == WindowTypeDesktop)
        OpenGL::generateVAOVBO(attrib.x, attrib.y + attrib.height,
            attrib.width, -attrib.height, vao, vbo);
    else
        OpenGL::generateVAOVBO(attrib.x, attrib.y, attrib.width, attrib.height, vao, vbo);
}

void FireWin::updateRegion() {
    if(region)
        XDestroyRegion(region);

    region = core->getRegionFromRect( attrib.x, attrib.y,
            attrib.x + attrib.width, attrib.y + attrib.height);
}

void FireWin::updateState() {
    GetTuple(sw, sh, core->getScreenSize());
    if(state & WindowStateMaxH) {
        if(attrib.width != sw)
            this->resize(sw, attrib.height, true);

        if(attrib.x != 0)
            this->move(0, attrib.y, true);
    }

    if(state & WindowStateMaxV) {
        if(attrib.height != sh)
            this->resize(attrib.width, sh, true);

        if(attrib.y != 0)
            this->move(attrib.x, 0, true);
    }

    if(state & WindowStateFullscreen){
        if(attrib.width != sw || attrib.height != sh)
            resize(sw, sh, true);
        if(attrib.x != 0 || attrib.y != 0)
            move(0, 0, true);
    }
}

void FireWin::syncAttrib() {
    XWindowAttributes xwa;
    XGetWindowAttributes(core->d, id, &xwa);

    bool mask = false;

    mask |= (xwa.x != attrib.x);
    mask |= (xwa.y != attrib.y);
    mask |= (xwa.width != attrib.width);
    mask |= (xwa.height != attrib.height);

    attrib.map_state = xwa.map_state;
    attrib.c_class   = xwa.c_class;

    if(attrib.map_state == IsViewable &&
            attrib.c_class != InputOnly)
        this->norender = false;

    if(!mask) return;

    attrib = xwa;

    OpenGL::API.glDeleteBuffers(1, &vbo);
    OpenGL::API.glDeleteVertexArrays(1, &vao);
    updateVBO();
    updateRegion();
    addDamage();
}

bool FireWin::isVisible() {
    if(destroyed && !keepCount)
        return false;

    if(!visible && !allDamaged)
        return false;

    if(norender || (state & WindowStateHidden))
        return false;

    if(attrib.c_class == InputOnly)
        return false;
    return true;
}

bool FireWin::shouldBeDrawn() {
    if(!isVisible())
        return false;

    auto result = XRectInRegion(core->dmg, attrib.x, attrib.y,
            attrib.width, attrib.height);

    if(result == RectangleOut)
        return false;
    else
        return true;
}

int FireWin::setTexture() {
    XGrabServer(core->d);

    if(attrib.map_state != IsViewable && !keepCount) {
        std::cout << "Invisible window " << id << std::endl;
        norender = true;
        XUngrabServer(core->d);
        return 0;
    }

    if(!damaged)  {
        glBindTexture(GL_TEXTURE_2D, texture);
        XUngrabServer(core->d);
        return 1;
    }

    glDeleteTextures(1, &texture);

    if(pixmap == 0)
        pixmap = XCompositeNameWindowPixmap(core->d, id);

    texture = GLXUtils::textureFromPixmap(pixmap,
            attrib.width, attrib.height, &shared);

    XUngrabServer(core->d);
    return 1;
}

void FireWin::render() {
    OpenGL::color = transform.color;
    if(type == WindowTypeDesktop){
        OpenGL::renderTransformedTexture(texture, vao, vbo,
                transform.compose());
        return;
    }

    if(!setTexture()) {
        std::cout <<"failed to paint window " <<
            id << " (no texture avail)" << std::endl;
        return;
    }
//
    if(vbo == -1 || vao == -1)
        updateVBO();

    OpenGL::depth = attrib.depth;
    OpenGL::renderTransformedTexture(texture, vao, vbo,
            transform.compose());

    std::vector<EffectHook*> hooksToRun;
    for(auto h : effects)
        if(h.second->getState())
            hooksToRun.push_back(h.second);

    for(auto h : hooksToRun)
        h->action();
}

void FireWin::move(int x, int y, bool configure) {
    bool existPreviousRegion = !(region == nullptr);

    Region prevRegion = nullptr;
    if(existPreviousRegion)
        prevRegion = copyRegion(region);

    int dx = x - attrib.x,
        dy = y - attrib.y;

    SignalListenerData d;
    d.push_back((void*)(this));
    d.push_back((void*)(&dx));
    d.push_back((void*)(&dy));
    core->triggerSignal("move-window", d);

    attrib.x = x;
    attrib.y = y;
    updateRegion();

    if(existPreviousRegion)
        core->damageRegion(prevRegion);
    core->damageRegion(region);

    if(type == WindowTypeDesktop) {
        OpenGL::API.glDeleteBuffers(1, &vbo);
        OpenGL::API.glDeleteVertexArrays(1, &vao);
        updateVBO();
        return;
    }

    if(!disableVBOChange)
        OpenGL::API.glDeleteBuffers(1, &vbo),
        OpenGL::API.glDeleteVertexArrays(1, &vao);

    if(configure) {
        XWindowChanges xwc;
        xwc.x = x;
        xwc.y = y;
        XConfigureWindow(core->d, id, CWX | CWY, &xwc);
    }
    if(!disableVBOChange)
        updateVBO();
}

void FireWin::resize(int w, int h, bool configure) {

    if(w <= 0 || h <= 0) return;

    bool existPreviousRegion = !(region == nullptr);
    Region prevRegion = nullptr;
    if(existPreviousRegion)
         prevRegion = copyRegion(region);

    attrib.width  = w;
    attrib.height = h;
    updateRegion();

    if(existPreviousRegion)
        core->damageRegion(prevRegion);
    core->damageRegion(region);

    if(!disableVBOChange)
        OpenGL::API.glDeleteBuffers(1, &vbo),
        OpenGL::API.glDeleteVertexArrays(1, &vao);

    if(configure) {
        XWindowChanges xwc;
        xwc.width  = w;
        xwc.height = h;
        XConfigureWindow(core->d, id, CWWidth | CWHeight, &xwc);
    }

    if(!disableVBOChange)
        updateVBO();

    if(shared.existing) {
        XShmDetach(core->d, &shared.shminfo);
        XDestroyImage(shared.image);
        shmdt(shared.shminfo.shmaddr);
        shmctl(shared.shminfo.shmid, IPC_RMID, NULL);
        shared.existing = false;
        shared.init = true;
    }
    if(pixmap)
        XFreePixmap(core->d, pixmap);
    if(attrib.map_state == IsViewable)
        pixmap = XCompositeNameWindowPixmap(core->d, id);
}

void FireWin::moveResize(int x, int y, int w, int h) {
    /* dirty hack for windows which *want* to be resized
     * even when maximized(for ex. terminator)
     * and don't show up properly if don't get configured */
    int nx = x, ny = y;
    if(visible && WinUtil::constrainNewWindowPosition(nx, ny))
        x = nx, y = ny;

    move(x, y);
    resize(w, h);
    updateState();
}

void FireWin::addDamage() {
    if(norender)
        return;

    damaged = true;
    if(!region) {
        XserverRegion reg =
        XFixesCreateRegionFromWindow(core->d, id, WindowRegionBounding);

        XDamageAdd(core->d, id, reg);
        XFixesDestroyRegion(core->d, reg);
        return;
    }

    core->damageRegion(region);
}

void FireWin::getInputFocus() {

    if(attrib.map_state != IsViewable)
        return;

    XSetInputFocus(core->d, id, RevertToPointerRoot, CurrentTime);
    XChangeProperty ( core->d, core->root, activeWinAtom,
            XA_WINDOW, 32, PropModeReplace,
            (unsigned char *) &id, 1 );

    XEvent ev;
    ev.type = ClientMessage;
    ev.xclient.window       = id;
    ev.xclient.message_type = wmProtocolsAtom;
    ev.xclient.format       = 32;
    ev.xclient.data.l[0]    = wmTakeFocusAtom;
    ev.xclient.data.l[1]    = CurrentTime;
    ev.xclient.data.l[2]    = 0;
    ev.xclient.data.l[3]    = 0;
    ev.xclient.data.l[4]    = 0;

    XSendEvent ( core->d, id, FALSE, NoEventMask, &ev );
}

namespace WinUtil {

    /* ensure that new window is on current viewport */
    bool constrainNewWindowPosition(int &x, int &y) {
        GetTuple(sw, sh, core->getScreenSize());
        auto nx = (x % sw + sw) % sw;
        auto ny = (y % sh + sh) % sh;

        if(nx != x || ny != y){
            x = nx, y = ny;
            return true;
        }
        return false;
    }

#define uchar unsigned char
    int readProp(Window win, Atom prop, int def) {
        Atom      actual;
        int       result, format;
        ulong n, left;
        uchar *data;

        result = XGetWindowProperty (core->d, win, prop,
                0L, 1L, FALSE, XA_CARDINAL, &actual, &format,
                &n, &left, &data);

        if (result == Success && data) {
            if (n)
                result = *(int *)data,
                       result >>= 16;
            XFree(data);
        }
        else
            result = def;
        return result;
    }

    FireWindow getClientLeader(Window win) {
        unsigned long nitems = 0;
        unsigned char* data = 0;
        Atom actual_type;
        int actual_format;
        unsigned long bytes;

        int status = XGetWindowProperty (core->d, win,
                wmClientLeaderAtom, 0L, 1L, False,
                XA_WINDOW, &actual_type, &actual_format,
                &nitems, &bytes, &data);
        if (status != Success || actual_type == None || data == NULL)
            return nullptr;
        auto x = *reinterpret_cast<Window*> (data);
        if ( x == 0 || x == win )
            return nullptr;

        return core->findWindow(x);
    }

    FireWindow getTransient(Window win) {

        Window dumm;
        auto status = XGetTransientForHint(core->d, win, &dumm);
        if ( status == 0 || dumm == 0 )
            return nullptr;
        else
            return core->findWindow(dumm);
    }

    void getWindowName(Window win, char *name) {
        Atom type;
        int form;
        unsigned long remain, len;

        if (XGetWindowProperty(core->d, win, wmNameAtom,
                    0, 1024, False, XA_STRING,
                    &type, &form, &len, &remain,
                    (unsigned char**)&name)) {
        }
    }

    WindowType getWindowType(Window win) {
        Atom      actual, a = None;
        int       result, format;
        ulong n, left;
        unsigned char *data;

        result = XGetWindowProperty (core->d, win, winTypeAtom,
                0L, 1L, FALSE, XA_ATOM, &actual, &format,
                &n, &left, &data);

        if(result == Success && data) {
            if(n)
                std::memcpy (&a, data, sizeof (Atom));
            XFree((void*) data);
        }
        else return WindowTypeUnknown;

        if (a) {
            if (a == winTypeNormalAtom)
                return WindowTypeNormal;
            else if (a == winTypeMenuAtom)
                return WindowTypeWidget;
            else if (a == winTypeDesktopAtom)
                return WindowTypeDesktop;
            else if (a == winTypeDockAtom)
                return WindowTypeDock;
            else if (a == winTypeToolbarAtom)
                return WindowTypeWidget;
            else if (a == winTypeUtilAtom)
                return WindowTypeWidget;
            else if (a == winTypeSplashAtom)
                return WindowTypeNormal;
            else if (a == winTypeDialogAtom)
                return WindowTypeModal;
            else if (a == winTypeDropdownMenuAtom)
                return WindowTypeWidget;
            else if (a == winTypePopupMenuAtom)
                return WindowTypeWidget;
            else if (a == winTypeTooltipAtom)
                return WindowTypeWidget;
            else if (a == winTypeNotificationAtom)
                return WindowTypeWidget;
            else if (a == winTypeComboAtom)
                return WindowTypeWidget;
            else if (a == winTypeDndAtom)
                return WindowTypeWidget;
        }
        return WindowTypeUnknown;
    }

    WindowState getStateMask(Atom state) {
        if ( state == winStateModalAtom )
            return WindowStateAbove;
        else if ( state == winStateStickyAtom )
            return WindowStateSticky;
        else if ( state == winStateMaximizedVertAtom )
            return WindowStateMaxV;
        else if ( state == winStateMaximizedHorzAtom )
            return WindowStateMaxH;
        else if ( state == winStateShadedAtom )
            return WindowStateBase;
        else if ( state == winStateSkipTaskbarAtom )
            return WindowStateSkipTaskbar;
        else if ( state == winStateSkipPagerAtom )
            return WindowStateSkipTaskbar;
        else if ( state == winStateHiddenAtom )
            return WindowStateHidden;
        else if ( state == winStateFullscreenAtom )
            return WindowStateFullscreen;
        else if ( state == winStateAboveAtom )
            return WindowStateAbove;
        else if ( state == winStateBelowAtom ) {
            return WindowStateBelow;
        }
        else if ( state == winStateDemandsAttentionAtom )
            return WindowStateBase;
        else if ( state == winStateDisplayModalAtom )
            return WindowStateBase;
        return WindowStateBase;
    }

    uint getWindowState(Window win) {
        Atom      actual;
        int       result, format;
        ulong n, left;
        unsigned char *data;
        uint  state = WindowStateBase;

        result = XGetWindowProperty (core->d, win, winStateAtom,
                0L, 1024L, FALSE, XA_ATOM, &actual, &format,
                &n, &left, &data);

        if (result == Success && data) {
            Atom *a = (Atom*)data;

            while(n--)
                state |= getStateMask(*a++);

            XFree((void*)data);
        }
        std::cout << "Returning state" << state << std::endl;
        return state;
    }

    void init() {

        activeWinAtom  = XInternAtom(core->d, "_NET_ACTIVE_WINDOW", 0);
        wmNameAtom     = XInternAtom(core->d, "WM_NAME", 0);
        winOpacityAtom = XInternAtom(core->d, "_NET_WM_WINDOW_OPACITY", 0);
        clientListAtom = XInternAtom(core->d, "_NET_CLIENT_LIST", 0);

        wmProtocolsAtom    = XInternAtom (core->d, "WM_PROTOCOLS", 0);
        wmTakeFocusAtom    = XInternAtom (core->d, "WM_TAKE_FOCUS", 0);
        wmClientLeaderAtom = XInternAtom (core->d, "WM_CLIENT_LEADER", 0);

        winTypeAtom       =XInternAtom (core->d, "_NET_WM_WINDOW_TYPE", 0);
        winTypeDesktopAtom=XInternAtom (core->d, "_NET_WM_WINDOW_TYPE_DESKTOP", 0);
        winTypeDockAtom   =XInternAtom (core->d, "_NET_WM_WINDOW_TYPE_DOCK", 0);
        winTypeToolbarAtom=XInternAtom (core->d, "_NET_WM_WINDOW_TYPE_TOOLBAR", 0);
        winTypeMenuAtom   =XInternAtom (core->d, "_NET_WM_WINDOW_TYPE_MENU", 0);
        winTypeUtilAtom   =XInternAtom (core->d, "_NET_WM_WINDOW_TYPE_UTILITY", 0);
        winTypeSplashAtom =XInternAtom (core->d, "_NET_WM_WINDOW_TYPE_SPLASH", 0);
        winTypeDialogAtom =XInternAtom (core->d, "_NET_WM_WINDOW_TYPE_DIALOG", 0);
        winTypeNormalAtom =XInternAtom (core->d, "_NET_WM_WINDOW_TYPE_NORMAL", 0);

        winTypeDropdownMenuAtom =
            XInternAtom (core->d, "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU", 0);
        winTypePopupMenuAtom    =
            XInternAtom (core->d, "_NET_WM_WINDOW_TYPE_POPUP_MENU", 0);
        winTypeTooltipAtom      =
            XInternAtom (core->d, "_NET_WM_WINDOW_TYPE_TOOLTIP", 0);
        winTypeNotificationAtom =
            XInternAtom (core->d, "_NET_WM_WINDOW_TYPE_NOTIFICATION", 0);
        winTypeComboAtom        =
            XInternAtom (core->d, "_NET_WM_WINDOW_TYPE_COMBO", 0);
        winTypeDndAtom          =
            XInternAtom (core->d, "_NET_WM_WINDOW_TYPE_DND", 0);

        winStateAtom         = XInternAtom (core->d, "_NET_WM_STATE", 0);
        winStateModalAtom    = XInternAtom (core->d, "_NET_WM_STATE_MODAL", 0);
        winStateStickyAtom   = XInternAtom (core->d, "_NET_WM_STATE_STICKY", 0);
        winStateHiddenAtom   = XInternAtom (core->d, "_NET_WM_STATE_HIDDEN", 0);
        winStateAboveAtom    = XInternAtom (core->d, "_NET_WM_STATE_ABOVE", 0);
        winStateBelowAtom    = XInternAtom (core->d, "_NET_WM_STATE_BELOW", 0);
        winStateShadedAtom   = XInternAtom (core->d, "_NET_WM_STATE_SHADED", 0);

        winStateMaximizedVertAtom    =
            XInternAtom (core->d, "_NET_WM_STATE_MAXIMIZED_VERT", 0);
        winStateMaximizedHorzAtom    =
            XInternAtom (core->d, "_NET_WM_STATE_MAXIMIZED_HORZ", 0);
        winStateSkipTaskbarAtom      =
            XInternAtom (core->d, "_NET_WM_STATE_SKIP_TASKBAR", 0);
        winStateSkipPagerAtom        =
            XInternAtom (core->d, "_NET_WM_STATE_SKIP_PAGER", 0);
        winStateFullscreenAtom       =
            XInternAtom (core->d, "_NET_WM_STATE_FULLSCREEN", 0);
        winStateDemandsAttentionAtom =
            XInternAtom (core->d, "_NET_WM_STATE_DEMANDS_ATTENTION", 0);
        winStateDisplayModalAtom     =
            XInternAtom (core->d, "_NET_WM_STATE_DISPLAY_MODAL", 0);
    }
}
