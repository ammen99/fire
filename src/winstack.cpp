#include "../include/winstack.hpp"
#include "../include/opengl.hpp"
#include <algorithm>


typedef std::list<FireWindow>::iterator StackIterator;

WinStack::WinStack() {
}

void WinStack::addWindow(FireWindow win) {
    if(win->type == WindowTypeDesktop) {
        wins.push_back(win);
        return;
    }

    wins.push_front(win);

    win->transientFor = WinUtil::getTransient(win);
    win->leader = WinUtil::getClientLeader(win);
    win->type = WinUtil::getWindowType(win);
    auto w = WinUtil::getAncestor(win);

    if(w)
        activeWin = w;

    WinUtil::setInputFocusToWindow(win->id);
    WinUtil::initWindow(win);
}

int WinStack::getNumOfWindows() {
    return wins.size();
}

StackIterator WinStack::getIteratorPositionForWindow(FireWindow win) {
    auto x = std::find_if(wins.begin(), wins.end(),
            [win] (FireWindow w) {
                return w->id == win->id;
            });
    return x;

}

StackIterator WinStack::getIteratorPositionForWindow(Window win) {
    auto x = std::find_if(wins.begin(), wins.end(),
            [win] (FireWindow w) {
                return w->id == win;
            });
    return x;
}

FireWindow WinStack::findWindow(Window win) {
    auto x = getIteratorPositionForWindow(win);

    if(x == wins.end()) {
        return nullptr;
    }
    return *x;
}

void WinStack::renderWindows() {
    int num = 0;
    for(auto w : wins)
        if(w && w->shouldBeDrawn())
            w->transform.stackID = num++, WinUtil::renderWindow(w);
}

void WinStack::removeWindow(FireWindow win, bool destroy) {
    auto x = std::find_if(wins.begin(), wins.end(),
            [win] (FireWindow w) {
                return w->id == win->id;
            });

    if(destroy)
        WinUtil::finishWindow(*x);

    wins.erase(x);
}

StackIterator WinStack::getTopmostTransientPosition(FireWindow win) {
    for(auto it = wins.begin(); it != wins.end(); it++ ) {
        auto w = (*it);
        if(WinUtil::getAncestor(w)->id == win->id ||
           WinUtil::isAncestorTo(win, w))
            return it;
    }
    return wins.end();
}

void WinStack::restackAbove(FireWindow above, FireWindow below) {

    err << "restackAbove begin";
    auto pos = getIteratorPositionForWindow(below);
    auto val = getIteratorPositionForWindow(above);

    if(pos == wins.end()) // if no window to restack above, add to the beginning
        pos = wins.begin();

    if(val == wins.end()) // if no window to restack, return
        return;

    auto t = WinUtil::getStackType(above, below);

    err << "Restack Above type " << t;
    if(t == StackTypeAncestor || t == StackTypeNoStacking)
        return;

    wins.splice(pos, wins, val);

    err << "RestackAbove end";
}

FireWindow WinStack::findTopmostStackingWindow(FireWindow win) {
    auto it = wins.begin();
    while(it != wins.end()) {
        auto type = WinUtil::getWindowType((*it));
        err << "window type is " << type;
        if(WinUtil::getWindowType((*it)) == WindowTypeNormal)
            return *it;
        ++it;
    }
    return nullptr;
}

void WinStack::restackTransients(FireWindow win) {

    std::vector<FireWindow> winsToRestack;
    for(auto w : wins)
        if(WinUtil::isAncestorTo(win, w))
            winsToRestack.push_back(w);

    for(auto w : winsToRestack)
        restackAbove(w, win);
}

void WinStack::updateTransientsAttrib(FireWindow win,
        int dx, int dy,
        int dw, int dh) {

    for(auto w : wins) {
        if(WinUtil::isAncestorTo(win, w)) {

            w->attrib.x += dx;
            w->attrib.y += dy;
            w->attrib.width += dw;
            w->attrib.height += dh;

            err << "Foudn ";

            glDeleteBuffers(1, &w->vbo);
            glDeleteVertexArrays(1, &w->vao);

            OpenGLWorker::generateVAOVBO(w->attrib.x, w->attrib.y,
                    w->attrib.width, w->attrib.height, w->vao, w->vbo);
        }
    }
}

void WinStack::focusWindow(FireWindow win) {

    if(win->type == WindowTypeDesktop)
        return;

    activeWin = WinUtil::getAncestor(win);

    auto w1 = findTopmostStackingWindow(activeWin);
    auto w2 = activeWin;

    if(w1 == nullptr)
        err << "caught nullptr";
    if(w1 && w1->id == w2->id)
        err << "no changes needed";

    if(w1 == nullptr || w1->id == w2->id)
        return;

    err << "Check passed, found window to lower";

    restackAbove(w2, w1);
    restackTransients(w1);
    restackTransients(w2);
//
    WinUtil::setInputFocusToWindow(activeWin->id);


    XWindowChanges xwc;
    xwc.stack_mode = Above;
    xwc.sibling = w1->id;
    XConfigureWindow(core->d, activeWin->id, CWSibling|CWStackMode, &xwc);

    core->redraw = true;
}
