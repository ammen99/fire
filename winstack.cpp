#include "winstack.hpp"
#include "opengl.hpp"
#include <algorithm>


typedef std::list<FireWindow>::iterator StackIterator;

WinStack::WinStack() {
}

void WinStack::addWindow(FireWindow win) {
    wins.push_front(win);

    win->transientFor = WindowWorker::getTransient(win);
    win->leader = WindowWorker::getClientLeader(win);
    win->type = WindowWorker::getWindowType(win);

    auto w = WindowWorker::getAncestor(win);
    if(w)
        activeWin = w;

    WindowWorker::setInputFocusToWindow(win->id);
    WindowWorker::initWindow(win);
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
        if(!w->norender)
            w->transform.stackID = num++, WindowWorker::renderWindow(w);
}

void WinStack::removeWindow(FireWindow win, bool destroy) {
    auto x = std::find_if(wins.begin(), wins.end(), 
            [win] (FireWindow w) {
                return w->id == win->id;
            });

    if(destroy)
        WindowWorker::finishWindow(*x);

    wins.erase(x);
}

StackIterator WinStack::getTopmostTransientPosition(FireWindow win) {
    for(auto it = wins.begin(); it != wins.end(); it++ ) {
        auto w = (*it);
        if(WindowWorker::getAncestor(w)->id == win->id ||
           WindowWorker::isAncestorTo(win, w))
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

    auto t = WindowWorker::getStackType(above, below);

    err << "Restack Above type " << t;
    if(t == StackTypeAncestor || t == StackTypeNoStacking)
        return;

    wins.splice(pos, wins, val);

    err << "RestackAbove end";
}

FireWindow WinStack::findTopmostStackingWindow(FireWindow win) {
    auto it = wins.begin();
    while(it != wins.end()) {
        auto type = WindowWorker::getWindowType((*it));
        err << "window type is " << type;
        if(WindowWorker::getWindowType((*it)) == WindowTypeNormal)
            return *it;
        ++it;
    }
    return nullptr;
}

void WinStack::restackTransients(FireWindow win) {

    std::vector<FireWindow> winsToRestack;
    for(auto w : wins)
        if(WindowWorker::isAncestorTo(win, w))
            winsToRestack.push_back(w);

    for(auto w : winsToRestack)
        restackAbove(w, win);
}

void WinStack::updateTransientsAttrib(FireWindow win,
        int dx, int dy,
        int dw, int dh) {

    for(auto w : wins) {
        if(WindowWorker::isAncestorTo(win, w)) {
            
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


    auto winToFocus = WindowWorker::getAncestor(win);

    auto w1 = findTopmostStackingWindow(winToFocus);
    auto w2 = winToFocus;

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
    WindowWorker::setInputFocusToWindow(winToFocus->id);

   err << "Event Send";
    
    XWindowChanges xwc;
    xwc.stack_mode = Above;
    xwc.sibling = w1->id;
    XConfigureWindow(core->d, winToFocus->id, CWSibling|CWStackMode, &xwc);

    err << "Xlib calls done. Rendering Windows";
    core->redraw = true;
    err << "FocusWindow end";
}
