#ifndef FIRE_H
#define FIRE_H

#include "commonincludes.hpp"
#include "window.hpp"
#include "glx.hpp"

class WinStack;

class Core {
    private:
        int damage;

        static WinStack *wins;
        void handleEvent(XEvent xev);
        void wait(int timeout);

        pollfd fd;
    
        bool moving; // are we moving a window?
        FireWindow movingWin; // window to be moved
        int sx, sy; // startint x & y

        int lastMoveX, lastMoveY; // last pointer pos when actual move 
        float accel; // acceleration
        timeval lastMove; // time when lastly moved

    public:
        bool redraw = true; // should we redraw?


    public:
        ~Core();
        void loop();
        Core();
        
        Display *d;
        Window root;
        Window overlay;

        int width;
        int height;

        void renderAllWindows();
        void addWindow(XCreateWindowEvent);
        FireWindow findWindow(Window win);

        void moveInitiate(XButtonPressedEvent);
        void moveIntermediate(XMotionEvent);
        void moveTerminate(XButtonPressedEvent);

        static int onXError (Display* d, XErrorEvent* xev);
        static int onOtherWmDetected(Display *d, XErrorEvent *xev);
};
extern Core *core;

#endif
