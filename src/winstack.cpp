#include "../include/winstack.hpp"
#include "../include/opengl.hpp"
#include <algorithm>


std::unordered_map<Window, FireWindow> windows;


using StackIterator = std::list<FireWindow>::iterator;
WinStack::WinStack() {
    layers.resize(3);
}

void WinStack::addWindow(FireWindow win) {
    std::cout << "Start add window" << std::endl;

    if(findWindow(win->id) != nullptr && win->type != WindowTypeDesktop) {
        focusWindow(win);
        return;
    }

    win->layer = getTargetLayerForWindow(win);
    if(win->layer == LayerAbove)
        std::cout << "Adding to top layer" << std::endl;

    layers[win->layer].push_front(win);
    restackTransients(win);

    windows[win->id] = win;
    std::cout << "Start end window" << std::endl;
}

Layer WinStack::getTargetLayerForWindow(FireWindow win) {
    if(win->type == WindowTypeDesktop ||
           win->state & WindowStateBelow)
        return LayerBelow;

    else if(win->type == WindowTypeDock ||
            win->type == WindowTypeModal ||
            win->state & WindowStateAbove ||
            win->state & WindowStateFullscreen)

        return LayerAbove;

    else return LayerNormal;
}

void WinStack::recalcWindowLayer(FireWindow win) {
    std::cout << "recalcWindowLayer" << std::endl;
    auto newLayer = getTargetLayerForWindow(win);
    std::cout << "1" << std::endl;
    if(win->layer == newLayer)
        return;

    std::cout << "2" << std::endl;
    auto it = getIteratorPositionForWindow(win);
    std::cout << "3" << std::endl;
    if(it != layers[win->layer].end())
        std::cout << "333" << std::endl,
        layers[win->layer].erase(it),
        layers[newLayer].push_front(win);
    std::cout << "4" << std::endl;
    win->layer = newLayer;
}

StackIterator WinStack::getIteratorPositionForWindow(FireWindow win) {
    auto x = std::find_if(layers[win->layer].begin(), layers[win->layer].end(),
            [win] (FireWindow w) {
                return w->id == win->id;
            });
    return x;
}

FireWindow WinStack::findWindow(Window win) {

    if(windows.find(win) != windows.end())
        return windows[win];

    return nullptr;
//
//    FireWindow tmp = std::make_shared<FireWin>(win, false);
//
//    for(int layer = 0; layer < layers.size(); layer++) {
//        tmp->layer = (Layer)layer;
//        auto it1 = getIteratorPositionForWindow(tmp);
//
//        if(it1 != layers[layer].end())
//            return *it1;
//    }
//    return nullptr;
}

void WinStack::renderWindows() {
    int num = 0;

    if(FireWin::allDamaged) {
        XDestroyRegion(core->dmg);
        core->dmg = copyRegion(output);

        for(int layer = layers.size() - 1; layer >= 0; layer--) {
            auto it = layers[layer].rbegin();
            while(it != layers[layer].rend()) {
                auto w = *it;
                if(w && w->shouldBeDrawn())
                    w->transform.stackID = num--,
                    w->render();

                ++it;
            }
        }

        return;
    }

    if(!core->dmg)
        core->dmg = copyRegion(output);

    std::vector<FireWindow> winsToDraw;

    auto tmp = XCreateRegion();
    for(auto wins : layers) {
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
    }

    auto it = winsToDraw.rbegin();
    while(it != winsToDraw.rend())
        (*it)->render(), ++it;

    XDestroyRegion(tmp);
}

void WinStack::removeWindow(FireWindow win) {
    for(auto &wins : layers) {
        auto x = std::find_if(wins.begin(), wins.end(),
                [win] (FireWindow w) {
                return w->id == win->id;
                });

        if(x != wins.end()) {
            wins.erase(x);
            std::cout << "found!!" << std::endl;
        }
    }
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

    if(win1->layer == win2->layer)
        return StackTypeSibling;

    return StackTypeNoStacking;
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

void WinStack::restackAbove(FireWindow above, FireWindow below, bool x) {


    if(above == nullptr || below == nullptr)
        return;

    std::cout << "restack above" << std::endl;

    auto pos = getIteratorPositionForWindow(below);
    auto val = getIteratorPositionForWindow(above);
    // TODO : check if really necessary

    if(pos == layers[below->layer].end() ||
       val == layers[above->layer].end()) // we should not touch
        return;                                // windows we do not own

    auto t = getStackType(above, below);
    std::cout << "got stack type" << std::endl;
    if(t == StackTypeAncestor || t == StackTypeNoStacking ||
            isAncestorTo(above, below) ||
            below->type == WindowTypeWidget) // we should not restack
        return;                              // a widget below

    std::cout << "check" << std::endl;

    std::cout << above->layer << " " << below->layer << std::endl;

    layers[above->layer].erase(val);

    std::cout << "erased" << std::endl;

    if(!above->type)
        std::cout << "here is the crash" << std::endl;
    else
        std::cout << "told u" << std::endl;

    std::cout << "after if" << std::endl;
    layers[below->layer].insert(pos, above);

    std::cout << "did magic" << std::endl;

    if(!isAncestorTo(below, above) && x) {
        std::cout << "wtfff" << std::endl;
        restackTransients(above);
        restackTransients(below);
    }

    std::cout << "lololo" << std::endl;
}

/* returns the topmost window that we can restack above */
FireWindow WinStack::findTopmostStackingWindow(FireWindow win) {
    for(auto w : layers[win->layer]) {
        if(win->id == w->id || win->type == WindowTypeDesktop)
            return nullptr;
        if(win->shouldBeDrawn() && !isAncestorTo(win, w) &&
           w->type != WindowTypeWidget && w->type != WindowTypeModal)
            return w;
    }
    return nullptr;
}

bool WinStack::isTransientInGroup(FireWindow transient, FireWindow parent) {
    if(parent->leader != transient->leader && transient->leader != parent)
        return false;

    if(transient->type == WindowTypeWidget ||
            transient->type == WindowTypeModal)
        return true;

    return false;
}

void WinStack::restackTransients(FireWindow win) {
    if(win == nullptr)
        return;

    std::cout << "rst begin" << std::endl;

    std::vector<FireWindow> winsToRestack;
    for(auto w : layers[win->layer])
        if(isAncestorTo(win, w) || isTransientInGroup(w, win))
            winsToRestack.push_back(w);

    std::cout << "got windows" << winsToRestack.size() << std::endl;

    for(auto w : winsToRestack)
        restackAbove(w, win, false);

    std::cout << "rst end" << std::endl;
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

    if(win->type == WindowTypeWidget && layers[win->layer].size()) {
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
    if(w2->id == (*layers[win->layer].begin())->id || w1 == nullptr){
        WinUtil::setInputFocusToWindow(w2->id);
        return;
    }

    restackAbove(w2, w1);

    XWindowChanges xwc;
    xwc.stack_mode = Above;
    xwc.sibling = w1->id;
    XConfigureWindow(core->d, activeWin->id, CWSibling|CWStackMode, &xwc);

    WinUtil::setInputFocusToWindow(activeWin->id);
    w1->addDamage();
    w2->addDamage();
}

FireWindow WinStack::findWindowAtCursorPosition(int x, int y) {
    for(auto wins : layers) {
        for(auto w : wins)
            if(w->attrib.map_state == IsViewable && // desktop and invisible
               w->type != WindowTypeDesktop      && // windows should be
               !w->norender                      && // ignored
               !w->destroyed                     &&
               w->region                         &&
               XPointInRegion(w->region, x, y))
                  return w;
    }

    return nullptr;
}

FireWindow WinStack::getTopmostToplevel() {
    for(auto wins : layers)
        for(auto w : wins)
            if(w->shouldBeDrawn() &&
               w->type != WindowTypeDesktop &&
               w->type != WindowTypeWidget)
                  return w;

    return nullptr;
}

void WinStack::forEachWindow(WindowProc proc) {
    for(auto wins : layers)
        for(auto w : wins)
            proc(w);
}
