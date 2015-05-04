#ifndef WINSTACK_H
#define WINSTACK_H
#include "core.hpp"
#include <list>




class WinStack {
    friend class Core;
    private:
        std::list<FireWindow> wins;
    public:
        FireWindow activeWin;
    private:
        typedef std::list<FireWindow>::iterator StackIterator;

    private:
        void restackTransients(FireWindow win);
        FireWindow findTopmostStackingWindow(FireWindow win);
        StackIterator getTopmostTransientPosition(FireWindow win);

        StackIterator getIteratorPositionForWindow(FireWindow win);
        StackIterator getIteratorPositionForWindow(Window win);

    public:
        WinStack();
        void addWindow(FireWindow win);
        void removeWindow(FireWindow win, bool destroy);
        FireWindow findWindow(Window win);
        void renderWindows();
        int getNumOfWindows();

        void focusWindow(FireWindow win);
        void restackAbove(FireWindow win, FireWindow above);

        void updateTransientsAttrib(FireWindow win, int, int, int, int);
};


#endif
