#ifndef FIRE_H
#define FIRE_H

#include "commonincludes.hpp"
#include "window.hpp"
#include "glx.hpp"


extern bool inRenderWindow;
class WinStack;

struct Context{
    XEvent xev;
    Context(XEvent xev);
};


enum BindingType {
    BindingTypePress,
    BindingTypeRelease
};

struct Binding{
    bool active;
    BindingType type;
    uint mod;

    std::function<void(Context*)> action;

};

// keybinding
struct KeyBinding : Binding {
    KeyCode key;
};

// button binding

struct ButtonBinding : Binding {
    uint button;
};

// hooks are done once a redraw cycle
struct Hook {
    private:
        bool active;
    public:
        std::function<void(void)> action;
        void enable();
        void disable();
        bool getState();
        Hook();
};

class Core {
    private:
        std::vector<std::vector<FireWindow> > backgrounds;
        int damage;

        static WinStack *wins;
        void handleEvent(XEvent xev);
        void wait(int timeout);

        pollfd fd;

        int mousex, mousey; // pointer x, y

        int vwidth, vheight; // viewport size
        int vx, vy;          // viewport position

        std::unordered_map<uint, KeyBinding*> keys;
        std::unordered_map<uint, ButtonBinding*> buttons;
        std::unordered_map<uint, Hook*> hooks;

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
        }*move;

        class Resize : WindowOperation {
            public:
                Resize(Core *core);
                void Initiate(Context*);
                void Intermediate();
                void Terminate(Context*);
        }*resize;

        template<class T>
        uint getFreeID(std::unordered_map<uint, T> *map);

        KeyCode switchWorkspaceBindings[4];
        class WSSwitch {
            private:
                KeyBinding kbs[4];
                Hook hook;
                int stepNum;
                int dirx, diry;
                int dx, dy;
                int nx, ny;
            public:
                WSSwitch(Core *core);
                void moveWorkspace(int dx, int dy);
                void handleSwitchWorkspace(Context *ctx);
                void moveStep();
        }*wsswitch;

    public:
        Display *d;
        Window root;
        Window overlay;

        int cntHooks;

        int width;
        int height;

        bool redraw = true; // should we redraw?

        void setBackground(const char *path);

        void switchWorkspace(std::tuple<int, int>);
        std::tuple<int, int> getWorkspace();

        uint addKey(KeyBinding *kb, bool grab = false);
        void remKey(uint id);

        uint addBut(ButtonBinding *bb, bool grab = false);
        void remBut(uint id);

        uint addHook(Hook*);
        void remHook(uint);

    public:
        ~Core();
        void loop();
        Core();

        void run(char *command);

        void renderAllWindows();
        void addWindow(XCreateWindowEvent);
        FireWindow findWindow(Window win);

        static int onXError (Display* d, XErrorEvent* xev);
        static int onOtherWmDetected(Display *d, XErrorEvent *xev);
};
extern Core *core;

#endif
