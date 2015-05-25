#include "../include/winstack.hpp"
#include "../include/opengl.hpp"
#include <algorithm>

Focus::Focus(Core *core) {
    focus.type = BindingTypePress;
    focus.button = Button1;
    focus.mod = NoMods;
    focus.active = true;

    focus.action = [core] (Context *ctx){
        auto xev = ctx->xev.xbutton;
        auto w =
            core->wins->findWindowAtCursorPosition
            (Point(xev.x_root, xev.y_root));

        if(w)
            core->wins->focusWindow(w);
    };
    core->addBut(&focus);
}

typedef std::list<FireWindow>::iterator StackIterator;
WinStack::WinStack() {
    using namespace std::placeholders;
    findWindowAtCursorPosition =
        std::bind(std::mem_fn(&WinStack::__findWindowAtCursorPosition),
                this, _1);
}

void WinStack::addWindow(FireWindow win) {
    if(win->type == WindowTypeDesktop) {
        wins.push_back(win);
        return;
    }

    if(getIteratorPositionForWindow(win->id) != wins.end())
        return;

    wins.push_front(win);
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
    auto it = wins.rbegin();
    while(it != wins.rend()) {
        auto w = *it;
        if(w && w->shouldBeDrawn())
            w->transform.stackID = num++,
            WinUtil::renderWindow(w);
        ++it;
    }
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

    auto pos = getIteratorPositionForWindow(below);
    auto val = getIteratorPositionForWindow(above);

    if(pos == wins.end())  // if no window to restack above
        pos = wins.begin();// add to the beginning of the stack

    if(val == wins.end()) // if no window to restack, return
        return;

    auto t = WinUtil::getStackType(above, below);
    if(t == StackTypeAncestor || t == StackTypeNoStacking)
        return;

    wins.splice(pos, wins, val);
}

FireWindow WinStack::findTopmostStackingWindow(FireWindow win) {
    for(auto w : wins)
        if(WinUtil::getWindowType(w) == WindowTypeNormal)
            return w;
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

    activeWin = win;

    auto w1 = findTopmostStackingWindow(activeWin);
    auto w2 = activeWin;

    if(w2->id == (*wins.begin())->id){
        WinUtil::setInputFocusToWindow(w2->id);
        return;
    }

    if(w1 == nullptr) {
        restackAbove(w1, (*wins.begin()));
        restackTransients(w1);
        return;
    }

    if(w1 == nullptr || w1->id == w2->id)
        return;

    restackAbove(w2, w1);
    restackTransients(w1);
    restackTransients(w2);


    XWindowChanges xwc;
    xwc.stack_mode = Above;
    xwc.sibling = w1->id;
    XConfigureWindow(core->d, activeWin->id, CWSibling|CWStackMode, &xwc);

    WinUtil::setInputFocusToWindow(activeWin->id);
    core->redraw = true;
}

FireWindow WinStack::__findWindowAtCursorPosition(Point p) {
    for(auto w : wins)
        if(w->attrib.map_state == IsViewable && // desktop and invisible
           w->type != WindowTypeDesktop      && // windows should be
           !w->norender                      && // ignored
           (w->getRect() & p))

               return w;

    return nullptr;
}
