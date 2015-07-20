#include "../include/winstack.hpp"
#include "../include/opengl.hpp"
#include <algorithm>



typedef std::list<FireWindow>::iterator StackIterator;
WinStack::WinStack() { }

void WinStack::addWindow(FireWindow win) {
    if(win->type == WindowTypeDesktop) {
        wins.push_back(win);
        return;
    }

    if(getIteratorPositionForWindow(win->id) != wins.end()) {
        focusWindow(win);
        return;
    }

    wins.push_front(win);
    restackTransients(win);
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

    if(__FireWindow::allDamaged) {
        XDestroyRegion(core->dmg);
        core->dmg = copyRegion(output);

        auto it = wins.rbegin();
        while(it != wins.rend()) {
            auto w = *it;
            if(w && w->shouldBeDrawn()) {
                w->transform.stackID = num--;
                WinUtil::renderWindow(w);
            }
            ++it;
        }
        return;
    }

    if(!core->dmg)
        core->dmg = copyRegion(output);

    std::vector<FireWindow> winsToDraw;

    auto tmp = XCreateRegion();
    for(auto w : wins) {
        if(w && w->shouldBeDrawn()) {

            w->transform.stackID = num--;
            if(w->transparent || w->attrib.depth == 32)
                w->transparent = true;
            else
                XIntersectRegion(core->dmg, w->region, tmp),
                XXorRegion(core->dmg, tmp, core->dmg);

            winsToDraw.push_back(w);
        }
    }

    auto it = winsToDraw.rbegin();
    while(it != winsToDraw.rend())
        WinUtil::renderWindow(*it), ++it;

    XDestroyRegion(tmp);
}

void WinStack::removeWindow(FireWindow win) {
    auto x = std::find_if(wins.begin(), wins.end(),
            [win] (FireWindow w) {
                return w->id == win->id;
            });

    if(x != wins.end())
        wins.erase(x);
}


bool WinStack::recurseIsAncestor(FireWindow parent, FireWindow win) {
    if(parent->id == win->id)
        return true;

    bool mask = false;

    if(win->transientFor)
        mask |= recurseIsAncestor(parent, win->transientFor);

    if(win->leader)
        mask |= recurseIsAncestor(parent, win->leader);

    return mask;
}

bool WinStack::isAncestorTo(FireWindow parent, FireWindow win) {
    if(win->id == parent->id) // a window is not its own ancestor
        return false;

    return recurseIsAncestor(parent, win);
}

StackType WinStack::getStackType(FireWindow win1, FireWindow win2) {
    if(win1->id == win2->id)
        return StackTypeNoStacking;

    if(isAncestorTo(win1, win2))
        return StackTypeAncestor;

    if(isAncestorTo(win2, win1))
        return StackTypeChild;

    return StackTypeSibling;
}


FireWindow WinStack::getAncestor(FireWindow win) {

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
StackIterator WinStack::getTopmostTransientPosition(FireWindow win) {
    for(auto it = wins.begin(); it != wins.end(); it++ ) {
        auto w = (*it);
        if(getAncestor(w)->id == win->id ||
           isAncestorTo(win, w))
            return it;
    }
    return wins.end();
}

void WinStack::restackAbove(FireWindow above, FireWindow below, bool x) {

    if(above == nullptr || below == nullptr)
        return;

    auto pos = getIteratorPositionForWindow(below);
    auto val = getIteratorPositionForWindow(above);

    if(pos == wins.end() || val == wins.end()) // we should not touch
        return;                                // windows we do not own

    auto t = getStackType(above, below);
    if(t == StackTypeAncestor || t == StackTypeNoStacking ||
            isAncestorTo(above, below) ||
            below->type == WindowTypeWidget) // we should not restack
        return;                              // a widget below

    wins.erase(val);
    wins.insert(pos, above);

    if(!isAncestorTo(below, above) && x) {
        restackTransients(above);
        restackTransients(below);
    }
}

/* returns the topmost window that we can restack above */
FireWindow WinStack::findTopmostStackingWindow(FireWindow win) {
    for(auto w : wins) {
        if(win->id == w->id || win->type == WindowTypeDesktop)
            return nullptr;
        if(win->shouldBeDrawn() && !isAncestorTo(win, w) &&
           w->type != WindowTypeWidget && w->type != WindowTypeModal)
            return w;
    }
    return nullptr;
}

namespace {
    bool isTransientInGroup(FireWindow transient, FireWindow parent) {
        if(parent->leader != transient->leader)
            return false;

        if(transient->type == WindowTypeWidget ||
           transient->type == WindowTypeModal)
            return true;

        return false;
    }
}

void WinStack::restackTransients(FireWindow win) {

    std::cout << "restack transients true" << std::endl;
    if(win == nullptr)
        return;

    std::cout << "wrrrr" << std::endl;

    std::vector<FireWindow> winsToRestack;
    for(auto w : wins)
        if(isAncestorTo(win, w) || isTransientInGroup(w, win))
            winsToRestack.push_back(w);

    std::cout << "ttrrrrr" << std::endl;

    for(auto w : winsToRestack)
        restackAbove(w, win, false);

    std::cout << "restack Transients end" << std::endl;
}

void WinStack::updateTransientsAttrib(FireWindow win,
        int dx, int dy,
        int dw, int dh) {

    for(auto w : wins) {
        if(isAncestorTo(win, w)) {

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
    if(win == nullptr)
        return;

    if(win->type == WindowTypeDesktop)
        return;

    if(win->attrib.c_class == InputOnly)
        return;

    if(!win->shouldBeDrawn())
        return;

    if(win->type == WindowTypeWidget && wins.size()) {
        if(win->transientFor)
            restackAbove(win, win->transientFor),
            win = win->transientFor;
        else
            return;
    }

    if(win->type == WindowTypeModal) {
        if(win->transientFor)
            restackAbove(win, win->transientFor);
        WinUtil::setInputFocusToWindow(win->id);
        activeWin = win;
        return;
    }

    activeWin = win;

    auto w1 = findTopmostStackingWindow(activeWin); // window to restack below
    auto w2 = activeWin; // window to restack above(our window)

    if(!w2)
        return;


    /* if we are already at the top of the stack
     * then just focus the window */
    if(w2->id == (*wins.begin())->id || w1 == nullptr){
        WinUtil::setInputFocusToWindow(w2->id);
        return;
    }

    restackAbove(w2, w1);

    XWindowChanges xwc;
    xwc.stack_mode = Above;
    xwc.sibling = w1->id;
    XConfigureWindow(core->d, activeWin->id, CWSibling|CWStackMode, &xwc);

    WinUtil::setInputFocusToWindow(activeWin->id);
    core->damageWindow(w1);
    core->damageWindow(w2);
}

FireWindow WinStack::findWindowAtCursorPosition(int x, int y) {
    for(auto w : wins)
        if(w->attrib.map_state == IsViewable && // desktop and invisible
           w->type != WindowTypeDesktop      && // windows should be
           !w->norender                      && // ignored
           !w->destroyed                     &&
           w->region                         &&
           XPointInRegion(w->region, x, y))
               return w;

    return nullptr;
}

FireWindow WinStack::getTopmostToplevel() {
    for(auto w : wins)
        if(w->shouldBeDrawn() &&
           w->type != WindowTypeDesktop &&
           w->type != WindowTypeWidget)
            return w;

    return nullptr;
}
