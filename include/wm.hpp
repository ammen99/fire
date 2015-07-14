#ifndef WM_H
#define WM_H
#include "plugin.hpp"
#include "core.hpp"
/* here are the definitions for
 * the wm actions(i.e close, move, resize) */

// The following plugins
// are very, very simple(they just register a keybinding)

class Run : public Plugin {
    public:
        void init();
};

class Exit : public Plugin {
    public:
        void init();
};

class Close : public Plugin {
    public:
        void init();
};

class Refresh : public Plugin { // keybinding to restart window manager
    KeyBinding ref;
    public:
        void init();
};

class Focus : public Plugin {
    private:
        ButtonBinding focus;
    public:
        void init();
};



/* specialized class for operations on window(for ex. Move and Resize) */
class WindowOperation : public Plugin {
    protected:
        int sx, sy; // starting pointer x, y
        FireWindow win; // window we're operating on

    protected:
        ButtonBinding press;
        ButtonBinding release;
        Hook hook;
};

/* typically plugins don't have any other public methods
 * except for init(), because they are to be "generic" */

class Move : public WindowOperation {
    void Initiate(Context*);
    void Intermediate();
    void Terminate(Context*);

    public:
    void init();
    void initOwnership();
};

class Resize : public WindowOperation {
    void Initiate(Context*);
    void Intermediate();
    void Terminate(Context*);

    public:
    void init();
    void initOwnership();
};

class WSSwitch : public Plugin {
    private:
        KeyBinding kbs[4];
        KeyCode switchWorkspaceBindings[4];

        Hook hook;
        int stepNum;
        int dirx, diry;
        int dx, dy;
        int nx, ny;
        std::queue<std::tuple<int, int> >dirs; // series of moves we have to do
        void beginSwitch();
        void moveWorkspace(int dx, int dy);
        void handleSwitchWorkspace(Context *ctx);
        void moveStep();

    public:
        void init();
        void initOwnership();
};

class Expo : public Plugin {
    private:
        KeyBinding keys[4];
        KeyCode switchWorkspaceBindings[4];
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
        std::function<FireWindow(int, int)> save; // used to restore

        void handleKey(Context *ctx);
        void Toggle(Context *ctx);
        FireWindow findWindow(int x, int y);
        void buttonRelease(Context *ctx);
        void buttonPress(Context *ctx);
        void recalc();
        void zoom();
        void finalizeZoom();

    public:
        void init();
        void initOwnership();
};

class ATSwitcher : public Plugin {
    KeyBinding initiate;
    KeyBinding forward;
    KeyBinding backward;
    KeyBinding terminate;
    std::vector<FireWindow> windows;
    FireWindow background;
    bool active;
    int index;
    Hook rnd; // used to render windows

    void handleKey(Context *ctx);
    void moveLeft();
    void moveRight();
    void Initiate();
    void Terminate();
    void render();
    void reset();
    float getFactor(int x, int y, float percent);

    public:
        void init();
        void initOwnership();
};

class Grid : public Plugin {

    struct GridWindow {
        Window id;
        XWindowAttributes size;
        GridWindow(Window id, int x, int y, int w, int h);
    };

    std::vector<GridWindow> wins;
    KeyBinding keys[10];
    KeyCode codes[10];

    void handleKey(Context*);
    void toggleMaxim(FireWindow win);
    void getSlot(int n, int &x, int &y, int &w, int &h);

    public:
        void init();
};
#endif
