#include "../include/core.hpp"
#include "../include/opengl.hpp"
#include <X11/extensions/XTest.h>


glm::mat4 Transform::proj;
glm::mat4 Transform::view;
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

    std::cout << "Before crash" << std::endl;
    Region newreg = XCreateRegion();

    newreg->rects = new BOX[reg->numRects];
    newreg->size = reg->size;
    newreg->numRects = reg->numRects;
    newreg->extents = reg->extents;
    std::cout << "Before memcpy" << std::endl;
    std::memcpy(newreg->rects, reg->rects, reg->numRects * sizeof(BOX));

    std::cout << "After crahs" << std::endl;
    return newreg;
}

Transform::Transform() {
    this->translation = glm::mat4();
    this->rotation = glm::mat4();
    this->scalation = glm::mat4();
    this->stackID = 0.f;
}

glm::mat4 Transform::compose() {
    float c = -1;
    if(OpenGLWorker::transformed)
        c = 1;
    auto trans =
        glm::translate(translation,
                glm::vec3(0.f, 0.f, c * stackID * 1e-2));

    return proj * view * (gtrs * trans) *
        (grot * rotation) * (gscl * scalation);
}

Region output;
bool __FireWindow::allDamaged = false;

bool __FireWindow::shouldBeDrawn() {

    if(keepCount)
        return true;
    else if(destroyed)
        return false;

    if(norender)
        return false;

    if(attrib.c_class == InputOnly)
        return false;

    if(attrib.width < 11 && attrib.height < 11) {
        norender = true;
        return false;
    }

    if(allDamaged) {
        if(!region)
            return false;

        // TODO: XXX:
        // workaround, no time to investigate
        return true;

        std::cout << "All damaged" << std::endl;
        if(!region)
            std::cout << "Failed region" << std::endl;
        if(!output)
            std::cout << "Failed output" << std::endl;
        REGION tmp;
        XIntersectRegion(region, output, &tmp);

        std::cout << "jetzt hire" << std::endl;
        if(!XEmptyRegion(&tmp))
            return true;
        else
            return false;
    }

    return true;
}

void __FireWindow::updateVBO() {
    if(type == WindowTypeDesktop)
        OpenGLWorker::generateVAOVBO(attrib.x,
            attrib.y + attrib.height,
            attrib.width, -attrib.height,
            vao, vbo);
    else
        OpenGLWorker::generateVAOVBO(attrib.x, attrib.y,
            attrib.width, attrib.height,
            vao, vbo);
}

void __FireWindow::updateRegion() {
    if(region)
        XDestroyRegion(region);

    region = core->getRegionFromRect( attrib.x, attrib.y,
            attrib.x + attrib.width, attrib.y + attrib.height);
}

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
Atom winOpacityAtom;

namespace WinUtil {
void init(Core *core) {

    activeWinAtom  = XInternAtom(core->d, "_NET_ACTIVE_WINDOW", 0);
    wmNameAtom     = XInternAtom(core->d, "WM_NAME", 0);
    winOpacityAtom = XInternAtom(core->d, "_NET_WM_WINDOW_OPACITY", 0);

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


}


int setWindowTexture(FireWindow win) {
    XGrabServer(core->d);

    if(win->mapTryNum --> 0) {        // we try five times
        XMapWindow(core->d, win->id); // to map a window
        syncWindowAttrib(win);        // in order to get a
        XSync(core->d, 0);            // pixmap
    }
    if(win->attrib.map_state != IsViewable && !win->keepCount) {
        err << "Invisible window";
        win->norender = true;
        XUngrabServer(core->d);
        return 0;
    }

    Pixmap pix = win->pixmap;
    if(!win->destroyed)
        pix = XCompositeNameWindowPixmap(core->d, win->id);

    if(win->pixmap == pix ||
            (win->keepCount && win->attrib.map_state != IsViewable)) {
        glBindTexture(GL_TEXTURE_2D, win->texture);
        XUngrabServer(core->d);
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
        XUngrabServer(core->d);
        return 0;
    }

    win->texture = GLXUtils::textureFromPixmap(win->pixmap,
            win->attrib.width, win->attrib.height, win->xvi);

    XUngrabServer(core->d);
    return 1;
}

void initWindow(FireWindow win) {

    XGetWindowAttributes(core->d, win->id, &win->attrib);
    XSizeHints hints;
    long flags;
    XGetWMNormalHints(core->d, win->id, &hints, &flags);

    if(flags & USPosition)
        win->attrib.x = hints.x,
        win->attrib.y = hints.y;

    if(flags & USSize)
        win->attrib.width = hints.width,
        win->attrib.height= hints.height;

    else if(flags & PBaseSize)
        win->attrib.width = hints.base_width,
        win->attrib.height = hints.base_height;

    win->updateRegion();

    if(win->attrib.c_class != InputOnly)
        win->damage =
            XDamageCreate(core->d, win->id, XDamageReportRawRectangles);
    else
        win->attrib.map_state = IsUnmapped,
        win->damage = None;

    win->transientFor = WinUtil::getTransient(win);
    win->leader = WinUtil::getClientLeader(win);
    win->type = WinUtil::getWindowType(win);

    XSelectInput(core->d, win->id,   FocusChangeMask  |
                PropertyChangeMask | EnterWindowMask);

    XGrabButton (core->d, AnyButton, AnyModifier, win->id, TRUE,
            ButtonPressMask | ButtonReleaseMask | Button1MotionMask,
            GrabModeSync, GrabModeSync, None, None);

    win->transform.color = glm::vec4(1., 1., 1., 1.);
    win->transform.color[3] = readProp(win->id,
            winOpacityAtom, 0xffff) / 0xffff;
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
}

void renderWindow(FireWindow win) {


    //std::cout << "begin render window" << std::endl;
    OpenGLWorker::color = win->transform.color;
    //win->remDamage();

    if(win->type == WindowTypeDesktop){
      //  std::cout << "desktop window" << std::endl;
        OpenGLWorker::renderTransformedTexture(win->texture,
                win->vao, win->vbo,
                win->transform.compose());

     //   std::cout << "drawn desktop" << std::endl;
        return;
    }

    //std::cout << "setting window texture" << std::endl;
    if(!setWindowTexture(win)) {
        err <<"failed to paint window " << win->id << " (no texture avail)";
        return;
    }

    //std::cout << "set texture" << std::endl;

    if(win->vbo == -1 || win->vao == -1)
        win->updateVBO();

   // std::cout << "looking for visual info" << std::endl;
    if(!win->xvi) {
        //std::cout << "no visual info, trying to get new one" << std::endl;
        win->xvi = getVisualInfoForWindow(win->id);
        if(!win->xvi) {
        //if(win->type == WindowTypeDesktop) {
        //    std::cout << "problem is a desktop window" << std::endl;
        //}
        //std::cout << "fail " << win->id << std::endl;
        return;
        }
        //std::cout << "got new" << std::endl;
    }
    OpenGLWorker::depth = win->xvi->depth;

//    std::cout << "rendering" << std::endl;
    OpenGLWorker::renderTransformedTexture(win->texture,
            win->vao, win->vbo, win->transform.compose());

//    std::cout << "rendered window" << std::endl;
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
    if(dumm == 0 || !xvi)
        err << "Cannot get default visual!\n";
    return xvi;
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
            return WindowTypeModal;
        else if ( a == winTypeTooltipAtom )
            return WindowTypeWidget;
        else if ( a == winTypeNotificationAtom )
            return WindowTypeWidget;
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

    bool existPreviousRegion = !(win->region == nullptr);

    Region prevRegion = nullptr;
    if(existPreviousRegion)
        prevRegion = copyRegion(win->region);

    win->attrib.x = x;
    win->attrib.y = y;
    win->updateRegion();

    if(existPreviousRegion)
        XUnionRegion(core->dmg, prevRegion,  core->dmg);
    XUnionRegion(core->dmg, win->region, core->dmg);

    if(win->type == WindowTypeDesktop) {
        glDeleteBuffers(1, &win->vbo);
        glDeleteVertexArrays(1, &win->vao);
        win->updateVBO();
        return;
    }

    XWindowChanges xwc;
    xwc.x = x;
    xwc.y = y;

    glDeleteBuffers(1, &win->vbo);
    glDeleteVertexArrays(1, &win->vao);

    XConfigureWindow(core->d, win->id, CWX | CWY, &xwc);
    win->updateVBO();
}

void resizeWindow(FireWindow win, int w, int h) {

    bool existPreviousRegion = !(win->region == nullptr);
    Region prevRegion = nullptr;
    if(existPreviousRegion)
         prevRegion = copyRegion(win->region);

    win->attrib.width  = w;
    win->attrib.height = h;
    win->updateRegion();

    if(existPreviousRegion)
        XUnionRegion(core->dmg, prevRegion,  core->dmg);
    XUnionRegion(core->dmg, win->region, core->dmg);

    XWindowChanges xwc;
    xwc.width  = w;
    xwc.height = h;

    glDeleteBuffers(1, &win->vbo);
    glDeleteVertexArrays(1, &win->vao);

    XConfigureWindow(core->d, win->id, CWWidth | CWHeight, &xwc);
    win->updateVBO();

}
void syncWindowAttrib(FireWindow win) {
    XWindowAttributes xwa;
    XGetWindowAttributes(core->d, win->id, &xwa);

    bool mask = false;

    mask |= (xwa.x != win->attrib.x);
    mask |= (xwa.y != win->attrib.y);
    mask |= (xwa.width != win->attrib.width);
    mask |= (xwa.height != win->attrib.height);

    win->attrib.map_state = xwa.map_state;
    win->attrib.c_class   = xwa.c_class;

    if(!mask)
        return;

    win->attrib = xwa;

    glDeleteBuffers(1, &win->vbo);
    glDeleteVertexArrays(1, &win->vao);
    win->updateVBO();
    win->updateRegion();
    core->damageWindow(win);
}
}
