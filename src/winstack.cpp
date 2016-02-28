//#include <winstack.hpp>
//#include <opengl.hpp>
//#include <algorithm>
//

//std::unordered_map<Window, View> windows;
//
//using StackIterator = std::list<View>::iterator;
//WinStack::WinStack() {
//    layers.resize(3);
//}
//
//void WinStack::addWindow(View win) {
//    if(findWindow(win->id) != nullptr &&
//            win->type != WindowTypeDesktop) {
//        return;
//    }
//
//    win->layer = getTargetLayerForWindow(win);
//
//    layers[win->layer].push_front(win);
//    restackTransients(win);
//
//    windows[win->id] = win;
//
//    checkAddClient(win);
//}
//
//Layer WinStack::getTargetLayerForWindow(View win) {
//    if(win->type == WindowTypeDesktop ||
//           win->state & WindowStateBelow)
//        return LayerBelow;
//
//    else if(win->type == WindowTypeDock ||
//            win->state & WindowStateAbove)
//
//        return LayerAbove;
//
//    else return LayerNormal;
//}
//
//void WinStack::recalcWindowLayer(View win) {
//    auto newLayer = getTargetLayerForWindow(win);
//    if(win->layer == newLayer)
//        return;
//
//    auto it = getIteratorPositionForWindow(win);
//    if(it != layers[win->layer].end())
//        layers[win->layer].erase(it),
//        layers[newLayer].push_front(win);
//    win->layer = newLayer;
//}
//
//StackIterator WinStack::getIteratorPositionForWindow(View win) {
//    return std::find_if(layers[win->layer].begin(), layers[win->layer].end(),
//            [win] (View w) {
//                return w && w->id == win->id;
//            });
//}
//
//View WinStack::findWindow(Window win) {
//    if(windows.find(win) != windows.end())
//        return windows[win];
//
//    return nullptr;
//}
//
//void WinStack::renderWindows() {
//    if(FireView::allDamaged) {
//        XDestroyRegion(core->dmg);
//        core->dmg = copyRegion(output);
//
//        for(int layer = layers.size() - 1; layer >= 0; layer--) {
//            auto it = layers[layer].rbegin();
//            while(it != layers[layer].rend()) {
//                auto w = *it;
//                if(w && w->shouldBeDrawn())
//                    w->render();
//
//                ++it;
//            }
//        }
//
//        return;
//    }
//
//    if(!core->dmg)
//        core->dmg = copyRegion(output);
//
//    std::vector<View> winsToDraw;
//
//    auto tmp = XCreateRegion();
//    for(auto wins : layers) {
//        for(auto w : wins) {
//            if(w && w->shouldBeDrawn()) {
//                if(w->transparent || w->attrib.depth == 32)
//                    w->transparent = true;
//                else
//                    XIntersectRegion(core->dmg, w->region, tmp),
//                        XXorRegion(core->dmg, tmp, core->dmg);
//
//                winsToDraw.push_back(w);
//            }
//        }
//    }
//
//    auto it = winsToDraw.rbegin();
//    while(it != winsToDraw.rend())
//        (*it)->render(), ++it;
//
//    XDestroyRegion(tmp);
//}
//
//void WinStack::removeWindow(View win) {
//    if(!win) return;
//    removeClient(win);
//
//    auto it = getIteratorPositionForWindow(win);
//    if(it != layers[win->layer].end()) {
//        windows.erase(win->id);
//        layers[win->layer].erase(it);
//    }
//}
//
//
//bool WinStack::recurseIsAncestor(View parent, FireViewdow win) {
//    if(parent->id == win->id)
//        return true;
//
//    bool mask = false;
//
//    if(win->transientFor)
//        mask |= recurseIsAncestor(parent, win->transientFor);
//
//    if(win->leader)
//        mask |= recurseIsAncestor(parent, win->leader);
//
//    return mask;
//}
//
//bool WinStack::isAncestorTo(View parent, FireViewdow win) {
//    if(win->id == parent->id) // a window is not its own ancestor
//        return false;
//
//    return recurseIsAncestor(parent, win);
//}
//
//StackType WinStack::getStackType(View win1, FireViewdow win2) {
//
//    if(win1->id == win2->id)
//        return StackTypeNoStacking;
//
//    if(isAncestorTo(win1, win2) || isTransientInGroup(win2, win1))
//        return StackTypeAncestor;
//
//    if(isAncestorTo(win2, win1) || isTransientInGroup(win1, win2))
//        return StackTypeChild;
//
//    if(win1->layer == win2->layer)
//        return StackTypeSibling;
//
//    return StackTypeNoStacking;
//}
//
//
//View WinStack::getAncestor(FireViewdow win) {
//
//    if(!win)
//        return nullptr;
//
//    if(win->type == WindowTypeNormal)
//        return win;
//
//    View w1, w2;
//    w1 = w2 = nullptr;
//
//    if(win->transientFor)
//        w1 = getAncestor(win->transientFor);
//
//    if(win->leader)
//        w2 = getAncestor(win->leader);
//
//    if(w1 == nullptr && w2 == nullptr)
//        return win;
//
//    else if(w1)
//        return w1;
//
//    else if(w2)
//        return w2;
//
//    else
//        return nullptr;
//}
//
//void WinStack::restackAbove(View above, FireViewdow below, bool rstTransients) {
//    if(above == nullptr || below == nullptr)
//        return;
//
//    auto pos = getIteratorPositionForWindow(below);
//    auto val = getIteratorPositionForWindow(above);
//
//    if(pos == layers[below->layer].end() ||
//       val == layers[above->layer].end())      // we should not touch
//        return;                                // windows we do not own
//
//    auto type = getStackType(above, below);
//    if(type == StackTypeAncestor ||
//       type == StackTypeNoStacking) {
//        return;
//    }
//
//    if(below->id == activeWin->id)
//        activeWin = above;
//
//    layers[above->layer].erase(val);
//    layers[below->layer].insert(pos, above);
//
//    if(!isAncestorTo(below, above) && rstTransients) {
//        restackTransients(above);
//        restackTransients(below);
//    }
//
//    above->addDamage();
//    below->addDamage();
//}
//
///* returns the topmost window that we can restack above */
//View WinStack::findTopmostStackingWindow(FireViewdow win) {
//    for(int layer = win->layer; layer < layers.size(); layer++) {
//        for(auto w : layers[layer]) {
//            if(win->id == w->id || win->type == WindowTypeDesktop)
//                return nullptr;
//
//            if(win->isVisible() && !isAncestorTo(win, w) &&
//                    w->type != WindowTypeWidget &&
//                    w->type != WindowTypeModal &&
//                    w->type != WindowTypeDock)
//                return w;
//        }
//    }
//    return nullptr;
//}
//
//bool WinStack::isTransientInGroup(View transient, FireViewdow parent) {
//    if(parent->leader != transient->leader && transient->leader != parent)
//        return false;
//
//    if(transient->type == WindowTypeWidget ||
//            transient->type == WindowTypeModal)
//        return true;
//
//    return false;
//}
//
//void WinStack::restackTransients(View win) {
//
//    if(win == nullptr)
//        return;
//
//    std::vector<View> winsToRestack;
//    for(auto w : layers[win->layer])
//        if(isAncestorTo(win, w) || isTransientInGroup(w, win))
//            winsToRestack.push_back(w);
//
//    for(auto w : winsToRestack)
//        restackAbove(w, win, false);
//}
//
//void WinStack::focusWindow(View win) {
//    if(win == nullptr || win->type == WindowTypeDesktop)
//        return;
//
//    if(win->attrib.c_class == InputOnly)
//        return;
//
//    activeWin = win;
//
//    if(win->type == WindowTypeWidget) {
//        /* ensure we are on the top of all siblings */
//        if(win->id != layers[win->layer].front()->id)
//            restackAbove(win, layers[win->layer].front());
//
//        if(win->transientFor)
//            win = win->transientFor;
//
//        else return;
//    }
//
//    if(win->type == WindowTypeModal) {
//        if(win->id != layers[win->layer].front()->id)
//            restackAbove(win, layers[win->layer].front());
//
//        win->getInputFocus();
//
//        if(win->transientFor)
//            win = win->transientFor;
//
//        else return;
//    }
//
//
//    /* window to be placed below the window being focused */
//    auto below = findTopmostStackingWindow(activeWin);
//
//    /* if we are already at the top of the stack
//     * just focus the window */
//    if(!below || below->id == activeWin->id){
//        /* ensure visibility */
//        if(layers[activeWin->layer].size() > 1)
//            restackAbove(activeWin, layers[activeWin->layer].front());
//        activeWin->getInputFocus();
//        return;
//    }
//
//    restackAbove(activeWin, below);
//
//    XWindowChanges xwc;
//    xwc.stack_mode = Above;
//    xwc.sibling = below->id;
//    XConfigureWindow(core->d, activeWin->id, CWSibling|CWStackMode, &xwc);
//
//    activeWin->getInputFocus();
//    below->addDamage();
//    activeWin->addDamage();
//}
//
//View WinStack::findWindowAtCursorPosition(int x, int y) {
//    for(auto wins : layers) {
//        for(auto w : wins)
//            if(w->attrib.map_state == IsViewable && // desktop and invisible
//               w->type != WindowTypeDesktop      && // windows should be
//               !w->norender                      && // ignored
//               !w->destroyed                     &&
//               w->region                         &&
//               XPointInRegion(w->region, x, y))
//                  return w;
//    }
//
//    return nullptr;
//}
//
//View WinStack::getTopmostToplevel() {
//    for(auto &wins : layers)
//        for(auto w : wins) {
//            if(w->isVisible() && !w->destroyed &&
//                    w->type != WindowTypeWidget    &&
//                    w->type != WindowTypeDesktop   &&
//                    w->type != WindowTypeDock)
//                  return w;
//        }
//
//    return nullptr;
//}
//
//void WinStack::forEachWindow(WindowProc proc) {
//    for(auto wins : layers)
//        for(auto w : wins)
//            proc(w);
//}
//
//void WinStack::setNetClientList() {
//    Window arr[clientList.size()];
//    int i = 0;
//    for(auto w : clientList)
//        arr[i++] = w;
//
//    XChangeProperty (core->d, core->root, clientListAtom,
//            XA_WINDOW, 32, PropModeReplace, (unsigned char *) arr, i);
//}
//
//bool WinStack::isClientWindow(View win) {
//    if(win->attrib.map_state != IsViewable)
//        return false;
//    if(win->attrib.override_redirect)
//        return false;
//
//    return true;
//}
//
//void WinStack::addClient(View win) {
//    clientList.insert(win->id);
//    setNetClientList();
//}
//void WinStack::removeClient(View win) {
//    if(clientList.find(win->id) != clientList.end())
//        clientList.erase(win->id);
//    setNetClientList();
//}
//
//void WinStack::checkAddClient(View win) {
//    if(isClientWindow(win) &&
//            clientList.find(win->id) == clientList.end())
//        addClient(win);
//}
//
//void WinStack::checkRemoveClient(View win) {
//    removeClient(win);
//}
