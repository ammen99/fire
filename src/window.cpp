#include "../include/core.hpp"
#include "../include/opengl.hpp"


Transform::Transform() {
    this->translation = glm::mat4();
    this->rotation = glm::mat4();
    this->scalation = glm::mat4();
    this->stackID = 0.f;
}

glm::mat4 Transform::compose() {
    auto trans =
        glm::translate(translation,
                glm::vec3(0.f, 0.f, stackID * 1e-2));

    return trans * rotation * scalation;
}

__FireWindow::Rect::Rect() : Rect(0, 0, 0, 0){}
__FireWindow::Rect::Rect(int tx, int ty, int bx, int by):
    tlx(tx), tly(ty), brx(bx), bry(by){}

bool __FireWindow::Rect::operator&(const __FireWindow::Rect &other) const {
    bool result = true;

    if(this->tly > other.bry || this->bry < other.tly)
        result = false;
    else if(this->brx < other.tlx || this->tlx > other.brx)
        result = false;

    return result;
}

std::ostream& operator<<(std::ostream &stream,
        const __FireWindow::Rect& rect) {

    stream << "Debug rect\n";
    stream << rect.tlx << " " << rect.tly << " ";
    stream << rect.brx << " " << rect.bry;
    stream << "\n";

    return stream;
}

bool __FireWindow::shouldBeDrawn() {
    if(norender)
        return false;

    if(type == WindowTypeOther)
        return false;

    //return true;

    auto pos = core->getWorkspace();
    Rect r1(attrib.x, attrib.y,
            attrib.x + attrib.width,
            attrib.y + attrib.height);

    auto vx = std::get<0>(pos) * core->width;
    auto vy = std::get<1>(pos) * core->height;

    Rect r2(vx, vy, vx + core->width, vy + core->height);

    if(r1 & r2)
        return true;
    else
        return false;
}



static int attrListVisual[] = {
    GLX_RGBA, GLX_DOUBLEBUFFER,
    GLX_RED_SIZE, 8,
    GLX_BLUE_SIZE, 8,
    GLX_GREEN_SIZE, 8,
    GLX_DEPTH_SIZE, 8,
    GLX_STENCIL_SIZE, 8,
    GLX_ALPHA_SIZE, 8,
    None
};

Atom winTypeAtom, winTypeDesktopAtom, winTypeDockAtom,
     winTypeToolbarAtom, winTypeMenuAtom, winTypeUtilAtom,
     winTypeSplashAtom, winTypeDialogAtom, winTypeNormalAtom,
     winTypeDropdownMenuAtom, winTypePopupMenuAtom, winTypeDndAtom,
     winTypeTooltipAtom, winTypeNotificationAtom, winTypeComboAtom;

Atom activeWinAtom;
Atom wmTakeFocusAtom;
Atom wmProtocolsAtom;
Atom wmClientLeaderAtom;
Atom wmNameAtom;

namespace WinUtil {
void init() {

    activeWinAtom = XInternAtom(core->d, "_NET_ACTIVE_WINDOW", 0);
    wmNameAtom    = XInternAtom(core->d, "WM_NAME", 0),

    wmProtocolsAtom    = XInternAtom (core->d, "WM_PROTOCOLS", 0);
    wmTakeFocusAtom    = XInternAtom (core->d, "WM_TAKE_FOCUS", 0);
    wmClientLeaderAtom = XInternAtom (core->d, "WM_CLIENT_LEADER", 0);

    winTypeAtom        = XInternAtom (core->d, "_NET_WM_WINDOW_TYPE", 0);
    winTypeDesktopAtom = XInternAtom (core->d, "_NET_WM_WINDOW_TYPE_DESKTOP", 0);
    winTypeDockAtom    = XInternAtom (core->d, "_NET_WM_WINDOW_TYPE_DOCK", 0);
    winTypeToolbarAtom = XInternAtom (core->d, "_NET_WM_WINDOW_TYPE_TOOLBAR", 0);
    winTypeMenuAtom    = XInternAtom (core->d, "_NET_WM_WINDOW_TYPE_MENU", 0);
    winTypeUtilAtom    = XInternAtom (core->d, "_NET_WM_WINDOW_TYPE_UTILITY", 0);
    winTypeSplashAtom  = XInternAtom (core->d, "_NET_WM_WINDOW_TYPE_SPLASH", 0);
    winTypeDialogAtom  = XInternAtom (core->d, "_NET_WM_WINDOW_TYPE_DIALOG", 0);
    winTypeNormalAtom  = XInternAtom (core->d, "_NET_WM_WINDOW_TYPE_NORMAL", 0);

    winTypeDropdownMenuAtom = XInternAtom (core->d, "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU", 0);
    winTypePopupMenuAtom    = XInternAtom (core->d, "_NET_WM_WINDOW_TYPE_POPUP_MENU", 0);
    winTypeTooltipAtom      = XInternAtom (core->d, "_NET_WM_WINDOW_TYPE_TOOLTIP", 0);
    winTypeNotificationAtom = XInternAtom (core->d, "_NET_WM_WINDOW_TYPE_NOTIFICATION", 0);
    winTypeComboAtom        = XInternAtom (core->d, "_NET_WM_WINDOW_TYPE_COMBO", 0);
    winTypeDndAtom          = XInternAtom (core->d, "_NET_WM_WINDOW_TYPE_DND", 0);


}


int setWindowTexture(FireWindow win) {

    if(win->attrib.map_state != IsViewable) {
        err << "Invisible window, deny rendering";
        win->norender = true;
        return 0;
    }

    auto pix = XCompositeNameWindowPixmap(core->d, win->id);

    if(win->pixmap == pix) {
        glBindTexture(GL_TEXTURE_2D, win->texture);
        return 1;
    }

    if(win->pixmap != 0) {
        XFreePixmap(core->d, win->pixmap);
        glDeleteTextures(1, &win->texture);
    }

    win->pixmap = pix;

    if(win->xvi == nullptr)
        win->xvi = getVisualInfoForWindow(win->id);
    if (win->xvi == nullptr) {
        err << "No visual info, deny rendering";
        win->norender = true;
        return 0;
    }

    win->texture = GLXUtils::textureFromPixmap(win->pixmap,
            win->attrib.width, win->attrib.height, win->xvi);
    return 1;
}

void initWindow(FireWindow win) {

    XSelectInput(core->d, win->id,   FocusChangeMask  |
                PropertyChangeMask | EnterWindowMask);

    XGrabButton ( core->d, AnyButton, AnyModifier, win->id, TRUE,
            ButtonPressMask | ButtonReleaseMask | Button1MotionMask,
            GrabModeSync, GrabModeSync, None, None );

    win->damage = XDamageCreate(core->d, win->id, XDamageReportRawRectangles);
    XGetWindowAttributes(core->d, win->id, &win->attrib);
}

void finishWindow(FireWindow win) {
    if(win->pixmap != 0)
        XFreePixmap(core->d, win->pixmap);

    GLboolean check;
    glAreTexturesResidentEXT(1, &win->texture, &check);
    if(check)
        glDeleteTextures(1, &win->texture);

    bool res = glIsBufferResidentNV(win->vbo);
    if(res)
        glDeleteBuffers(1, &win->vbo);

    res = glIsVertexArray(win->vao);
    if(res)
        glDeleteVertexArrays(1, &win->vao);

    XDamageDestroy(core->d, win->damage);

    err << "window deleted";
}



void renderWindow(FireWindow win) {
    if(win->type == WindowTypeDesktop){
        OpenGLWorker::renderTransformedTexture(win->texture,
                win->vao, win->vbo,
                win->transform.compose());
        return;
    }

    if(!setWindowTexture(win)) {
        err <<"failed to paint window " << win->id << " (no texture avail)";
        win->norender = true;
        return;
    }

    if(win->vbo == -1 || win->vao == -1)
        OpenGLWorker::generateVAOVBO(win->attrib.x, win->attrib.y,
                win->attrib.width, win->attrib.height,
                win->vao, win->vbo);

    OpenGLWorker::renderTransformedTexture(win->texture,
            win->vao, win->vbo, win->transform.compose());

}
FireWindow createWindow(int x, int y, int w, int h) {
    XVisualInfo *vi;
    Window win;

    Colormap cmap;
    XSetWindowAttributes winAttr;

    vi = glXChooseVisual ( core->d, DefaultScreen(core->d), attrListVisual );
    if ( vi == NULL )
        printf ( "Couldn't get visual!\n" ), std::exit ( 1 );
    cmap = XCreateColormap ( core->d, RootWindow ( core->d, vi->screen ),
                             vi->visual, AllocNone );

    winAttr.colormap = cmap;
    winAttr.border_pixel = 0;
    win = XCreateWindow ( core->d, RootWindow ( core->d, vi->screen ),
                   x, y, w, h, 0, vi->depth, InputOutput, vi->visual,
                   CWBorderPixel | CWColormap, &winAttr );
    XReparentWindow(core->d, win, core->overlay, 0, 0);
    XMapRaised ( core->d, win );

    FireWindow window = std::make_shared<__FireWindow>();
    window->id = win;
    return window;
}

XVisualInfo *getVisualInfoForWindow(Window win) {
    XVisualInfo *xvi, dummy;
    XWindowAttributes xwa;
    auto stat = XGetWindowAttributes(core->d, win, &xwa);
    if ( stat == 0 ){
        err << "attempting to get visual info failed!" << win;
        return nullptr;
    }
    dummy.visualid = XVisualIDFromVisual(xwa.visual);

    int dumm;
    xvi = XGetVisualInfo(core->d, VisualIDMask, &dummy, &dumm);
    if(dumm == 0)
        err << "Cannot get default visual!\n";
    return xvi;
}


bool isTopLevelWindow(FireWindow win) {
    if(win->transientFor) return false;
    if(win->leader) return false;

    Window client = XmuClientWindow(core->d, win->id);
    if(!client || client != win->id)
        return false;

    return true;
}

FireWindow getClientLeader(FireWindow win) {
    unsigned long nitems = 0;
    unsigned char* data = 0;
    Atom actual_type;
    int actual_format;
    unsigned long bytes;

    int status = XGetWindowProperty (core->d, win->id,
                  wmClientLeaderAtom, 0L, 1L, False,
                  XA_WINDOW, &actual_type, &actual_format,
                  &nitems, &bytes, &data);
    if (status != Success || actual_type == None || data == NULL)
        return nullptr;
    auto x = *reinterpret_cast<Window*> (data);
    if ( x == 0 || x == win->id )
        return nullptr;

    return core->findWindow(x);
}


FireWindow getTransient(FireWindow win) {

    Window dumm;
    auto status = XGetTransientForHint(core->d, win->id, &dumm);
    if ( status == 0 || dumm == 0 )
        return nullptr;
    else
        return core->findWindow(dumm);
}


void getWindowName(FireWindow win, char *name) {
    Atom type;
    int form;
    unsigned long remain, len;

    if (XGetWindowProperty(core->d, win->id, wmNameAtom,
                0, 1024, False, XA_STRING,
                &type, &form, &len, &remain, (unsigned char**)&name)) {
        }
}

FireWindow getAncestor(FireWindow win) {

    if(!win)
        return nullptr;

    if(win->type == WindowTypeNormal)
        return win;

    FireWindow w1, w2;
    w1 = w2 = nullptr;

    if(win->transientFor)
        w1 = getAncestor(win->transientFor);

    if(win->leader)
        w2 = getAncestor(win->leader);

    if(w1 == nullptr && w2 == nullptr)
        return win;

    else if(w1)
        return w1;

    else if(w2)
        return w2;

    else
        return nullptr;
}

bool recurseIsAncestor(FireWindow parent, FireWindow win) {
    if(parent->id == win->id)
        return true;

    bool mask = false;

    if(win->transientFor)
        mask |= recurseIsAncestor(parent, win->transientFor);

    if(win->leader)
        mask |= recurseIsAncestor(parent, win->leader);

    return mask;
}

bool isAncestorTo(FireWindow parent, FireWindow win) {
    if(win->id == parent->id) // a window is not its own ancestor
        return false;

    return recurseIsAncestor(parent, win);
}

StackType getStackType(FireWindow win1, FireWindow win2) {
    if(win1->id == win2->id)
        return StackTypeNoStacking;

    if(isAncestorTo(win1, win2))
        return StackTypeAncestor;

    if(isAncestorTo(win2, win1))
        return StackTypeChild;

    return StackTypeSibling;
}

WindowType getWindowType(FireWindow win) {
    Atom      actual, a = None;
    int       result, format;
    ulong n, left;
    unsigned char *data;

    result = XGetWindowProperty ( core->d, win->id, winTypeAtom,
            0L, 1L, FALSE, XA_ATOM, &actual, &format,
            &n, &left, &data );

    if ( result == Success && data ) {
        if ( n )
            std::memcpy (&a, data, sizeof (Atom));
        XFree ( ( void * ) data );
    }
    else
        return WindowTypeOther;

    if ( a ) {
        if ( a == winTypeNormalAtom )
            return WindowTypeNormal;
        else if ( a == winTypeMenuAtom )
            return WindowTypeWidget;
        else if ( a == winTypeDesktopAtom )
            return WindowTypeDesktop;
        else if ( a == winTypeDockAtom )
            return WindowTypeDock;
        else if ( a == winTypeToolbarAtom )
            return WindowTypeWidget;
        else if ( a == winTypeUtilAtom )
            return WindowTypeOther;
        else if ( a == winTypeSplashAtom )
            return WindowTypeNormal;
        else if ( a == winTypeDialogAtom )
            return WindowTypeModal;
        else if ( a == winTypeDropdownMenuAtom )
            return WindowTypeWidget;
        else if ( a == winTypePopupMenuAtom )
            return WindowTypeWidget;
        else if ( a == winTypeTooltipAtom )
            return WindowTypeWidget;
        else if ( a == winTypeNotificationAtom )
            return WindowTypeModal;
        else if ( a == winTypeComboAtom )
            return WindowTypeOther;
        else if ( a == winTypeDndAtom )
            return WindowTypeOther;
    }
    return WindowTypeOther;
}

void setInputFocusToWindow(Window win) {

    XSetInputFocus(core->d, win, RevertToPointerRoot, CurrentTime);
    XChangeProperty ( core->d, core->root, activeWinAtom,
            XA_WINDOW, 32, PropModeReplace,
            ( unsigned char * ) &win, 1 );

    err << "First Xlib calls";

    XEvent ev;

    ev.type = ClientMessage;
    ev.xclient.window       = win;
    ev.xclient.message_type = wmProtocolsAtom;
    ev.xclient.format       = 32;
    ev.xclient.data.l[0]    = wmTakeFocusAtom;
    ev.xclient.data.l[1]    = CurrentTime;
    ev.xclient.data.l[2]    = 0;
    ev.xclient.data.l[3]    = 0;
    ev.xclient.data.l[4]    = 0;

    XSendEvent ( core->d, win, FALSE, NoEventMask, &ev );

}

void moveWindow(FireWindow win, int x, int y) {
    win->attrib.x = x;
    win->attrib.y = y;
    XWindowChanges xwc;
    xwc.x = x;
    xwc.y = y;

    glDeleteBuffers(1, &win->vbo);
    glDeleteVertexArrays(1, &win->vao);

    XConfigureWindow(core->d, win->id, CWX | CWY, &xwc);
    OpenGLWorker::generateVAOVBO(x, y,
            win->attrib.width, win->attrib.height,
            win->vao, win->vbo);
}

void resizeWindow(FireWindow win, int w, int h) {
    win->attrib.width  = w;
    win->attrib.height = h;

    XWindowChanges xwc;
    xwc.width  = w;
    xwc.height = h;

    glDeleteBuffers(1, &win->vbo);
    glDeleteVertexArrays(1, &win->vao);

    XConfigureWindow(core->d, win->id, CWWidth | CWHeight, &xwc);
    OpenGLWorker::generateVAOVBO(win->attrib.x, win->attrib.y,
            win->attrib.width, win->attrib.height,
            win->vao, win->vbo);
}
void syncWindowAttrib(FireWindow win) {
    XWindowAttributes xwa;
    XGetWindowAttributes(core->d, win->id, &xwa);

    bool mask = false;

    mask |= (xwa.x != win->attrib.x);
    mask |= (xwa.y != win->attrib.y);
    mask |= (xwa.width != win->attrib.width);
    mask |= (xwa.height != win->attrib.height);

    if(!mask)
        return;


    err << "setting new attribs";
    win->attrib = xwa;

    glDeleteBuffers(1, &win->vbo);
    glDeleteVertexArrays(1, &win->vao);

    OpenGLWorker::generateVAOVBO(win->attrib.x, win->attrib.y,
            win->attrib.width, win->attrib.height,
            win->vao, win->vbo);
}
}
