#ifndef WM_H
#define WM_H
#include "plugin.hpp"
/* here are the
 * definitions for the wm actions(i.e close, move, resize) */
class Run : public Plugin {
    public:
        Run(Core *core);
};

class Exit {
    public:
        Exit(Core *core);
};

class Close{
    public:
        Close(Core *core);
};

class Focus;
class WindowOperation {
    protected:
        int sx, sy; // starting pointer x, y
        FireWindow win; // window we're operating on
        uint hid; // id of hook
    protected:
        ButtonBinding press;
        ButtonBinding release;
        Hook hook;
};

class Move : WindowOperation {
    public:
        Move(Core *core);
        void Initiate(Context*);
        void Intermediate();
        void Terminate(Context*);
};

class Resize : WindowOperation {
    public:
        Resize(Core *core);
        void Initiate(Context*);
        void Intermediate();
        void Terminate(Context*);
};

class WSSwitch {
    private:
        KeyBinding kbs[4];
        Hook hook;
        int stepNum;
        int dirx, diry;
        int dx, dy;
        int nx, ny;
        std::queue<std::tuple<int, int> >dirs; // series of moves we have to do
        void beginSwitch();
    public:
        WSSwitch(Core *core);
        void moveWorkspace(int dx, int dy);
        void handleSwitchWorkspace(Context *ctx);
        void moveStep();
};

class Expo {
    private:
        KeyBinding keys[4];
        KeyBinding toggle;
        ButtonBinding press, release;

        int stepNum;

        float offXtarget, offYtarget;
        float offXcurrent, offYcurrent;
        float sclXtarget, sclYtarget;
        float sclXcurrent, sclYcurrent;

        float stepoffX, stepoffY, stepsclX, stepsclY;

        Hook hook;
        bool active;
        std::function<FireWindow(Point)> save; // used to restore
    public:                                // WinStack::find...Position
        Expo(Core *core);
        void handleKey(Context *ctx);
        void Toggle(Context *ctx);
        FireWindow findWindow(Point p);
        void buttonRelease(Context *ctx);
        void buttonPress(Context *ctx);
        void recalc();
        void zoom();
        void finalizeZoom();
};

class ATSwitcher {
    KeyBinding initiate;
    KeyBinding forward;
    KeyBinding backward;
    KeyBinding terminate;
    std::vector<FireWindow> windows;
    FireWindow background;
    bool active;
    int index;
    Hook rnd; // used to render windows
    public:
        ATSwitcher(Core *core);
        void handleKey(Context *ctx);
        void moveLeft();
        void moveRight();
        void Initiate();
        void Terminate();
        void render();
        void reset();
        float getFactor(int x, int y, float percent);
};

class Grid{

    struct GridWindow {
        Window id;
        XWindowAttributes size;
        GridWindow(Window id, int x, int y, int w, int h);
    };

    std::vector<GridWindow> wins;
    KeyBinding keys[10];
    KeyCode codes[10];

    public:
        Grid(Core *core);
        void handleKey(Context*);
        void toggleMaxim(FireWindow win);
        void getSlot(int n, int &x, int &y, int &w, int &h);
};

class Close;