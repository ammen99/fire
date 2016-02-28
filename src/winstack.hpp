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
//using WindowProc = std::function<void(View)>;
//
//class WinStack {
//    private:
//
//        /* LayerAbove is for dialogs & docks
//         * LayerNormal is for normal windows
//         * and LayerBelow is for desktop windows */
//
//        std::vector<std::list<View> >layers;
//        std::unordered_set<Window> clientList;
//
//        typedef std::list<View>::iterator StackIterator;
//
//        StackIterator getIteratorPositionForWindow(View win);
//        bool recurseIsAncestor(View parent, FireViewdow win);
//
//        void addClient(View win);
//        void removeClient(View win);
//        void setNetClientList();
//        bool isClientWindow(View win);
//
//    public:
//        View activeWin;
//
//        WinStack();
//        void addWindow(View win);
//        void removeWindow(View win);
//        void recalcWindowLayer(View win);
//        Layer getTargetLayerForWindow(View win);
//        View findWindow(Window win);
//        View getTopmostToplevel();
//        void renderWindows();
//        void forEachWindow(WindowProc proc);
//
//        void checkAddClient(View win);
//        void checkRemoveClient(View win);
//
//        void focusWindow(View win);
//        /* rstr shows whether we have to restack transients also */
//        void restackAbove(View above, FireViewdow below, bool rstr = true);
//        View findWindowAtCursorPosition(int x, int y);
//
//        void restackTransients(View win);
//        View findTopmostStackingWindow(FireViewdow win);
//
//        View getAncestor(FireViewdow win);
//        bool isAncestorTo(View parent, FireViewdow win);
//        StackType getStackType(View win1, FireViewdow win2);
//        bool isTransientInGroup(View win, FireViewdow parent);
//};
#endif
