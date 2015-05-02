#ifndef FIRE_H
#define FIRE_H

#include "commonincludes.hpp"
#include "window.hpp"
#include "glx.hpp"

class WinStack;

class Core {
    private:
        FireWindow background;
        int damage;

        static WinStack *wins;
        void handleEvent(XEvent xev);
        void wait(int timeout);

        pollfd fd;

        bool moving; // are we moving a window?

        FireWindow operatingWin; // window which we operate with

        bool resizing;
        int sx, sy; // starting x & y

        int vwidth, vheight; // viewport size
        int vx, vy;          // viewport position

    public:
        Display *d;
        Window root;
        Window overlay;

        int width;
        int height;

        bool redraw = true; // should we redraw?

        void setBackground(const char *path);

        void switchWorkspace(std::tuple<int, int>);
        std::tuple<int, int> getWorkspace();

    public:
        ~Core();
        void loop();
        Core();

        void renderAllWindows();
        void addWindow(XCreateWindowEvent);
        FireWindow findWindow(Window win);

        void moveInitiate(XButtonPressedEvent);
        void moveIntermediate(XMotionEvent);
        void moveTerminate(XButtonPressedEvent);

        void resizeInitiate(XButtonPressedEvent);
        void resizeIntermediate(XMotionEvent);
        void resizeTerminate(XButtonPressedEvent);


        static int onXError (Display* d, XErrorEvent* xev);
        static int onOtherWmDetected(Display *d, XErrorEvent *xev);
};
extern Core *core;

#endif
