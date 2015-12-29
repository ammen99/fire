#ifndef WINSTACK_H
#define WINSTACK_H
#include "core.hpp"
#include <list>
//
//
//enum StackType{
//    StackTypeSibling    = 1,
//    StackTypeAncestor   = 2,
//    StackTypeChild      = 4,
//    StackTypeNoStacking = 8
//};
//
//using WindowProc = std::function<void(FireWindow)>;
//
//class WinStack {
//    private:
//
//        /* LayerAbove is for dialogs & docks
//         * LayerNormal is for normal windows
//         * and LayerBelow is for desktop windows */
//
//        std::vector<std::list<FireWindow> >layers;
//        std::unordered_set<Window> clientList;
//
//        typedef std::list<FireWindow>::iterator StackIterator;
//
//        StackIterator getIteratorPositionForWindow(FireWindow win);
//        bool recurseIsAncestor(FireWindow parent, FireWindow win);
//
//        void addClient(FireWindow win);
//        void removeClient(FireWindow win);
//        void setNetClientList();
//        bool isClientWindow(FireWindow win);
//
//    public:
//        FireWindow activeWin;
//
//        WinStack();
//        void addWindow(FireWindow win);
//        void removeWindow(FireWindow win);
//        void recalcWindowLayer(FireWindow win);
//        Layer getTargetLayerForWindow(FireWindow win);
//        FireWindow findWindow(Window win);
//        FireWindow getTopmostToplevel();
//        void renderWindows();
//        void forEachWindow(WindowProc proc);
//
//        void checkAddClient(FireWindow win);
//        void checkRemoveClient(FireWindow win);
//
//        void focusWindow(FireWindow win);
//        /* rstr shows whether we have to restack transients also */
//        void restackAbove(FireWindow above, FireWindow below, bool rstr = true);
//        FireWindow findWindowAtCursorPosition(int x, int y);
//
//        void restackTransients(FireWindow win);
//        FireWindow findTopmostStackingWindow(FireWindow win);
//
//        FireWindow getAncestor(FireWindow win);
//        bool isAncestorTo(FireWindow parent, FireWindow win);
//        StackType getStackType(FireWindow win1, FireWindow win2);
//        bool isTransientInGroup(FireWindow win, FireWindow parent);
//};
#endif
