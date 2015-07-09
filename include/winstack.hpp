#ifndef WINSTACK_H
#define WINSTACK_H
#include "core.hpp"
#include <list>


enum StackType{
    StackTypeSibling    = 1,
    StackTypeAncestor   = 2,
    StackTypeChild      = 4,
    StackTypeNoStacking = 8
};

class WinStack {
    friend class Core;
    private:
        std::list<FireWindow> wins;
        typedef std::list<FireWindow>::iterator StackIterator;

        StackIterator getIteratorPositionForWindow(FireWindow win);
        StackIterator getIteratorPositionForWindow(Window win);
        FireWindow __findWindowAtCursorPosition(int x, int y);
        bool recurseIsAncestor(FireWindow parent, FireWindow win);

    public:
        FireWindow activeWin;

        WinStack();
        void addWindow(FireWindow win);
        void removeWindow(FireWindow win);
        FireWindow findWindow(Window win);
        FireWindow getTopmostToplevel();
        void renderWindows();
        int getNumOfWindows();

        void focusWindow(FireWindow win);
        /* rstr shows whether we have to restack transients also */
        void restackAbove(FireWindow above, FireWindow below, bool rstr = true);
        std::function<FireWindow(int, int)> findWindowAtCursorPosition;

        void updateTransientsAttrib(FireWindow win, int, int, int, int);

        void restackTransients(FireWindow win);
        FireWindow findTopmostStackingWindow(FireWindow win);
        StackIterator getTopmostTransientPosition(FireWindow win);

        FireWindow getAncestor(FireWindow win);
        bool isAncestorTo(FireWindow parent, FireWindow win);
        StackType getStackType(FireWindow win1, FireWindow win2);
};
#endif
