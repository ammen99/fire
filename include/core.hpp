#ifndef FIRE_H
#define FIRE_H

#include "commonincludes.hpp"
#include "window.hpp"
#include "glx.hpp"
#include "plugin.hpp"
#include <queue>


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

class Expo;

class Core {

    // used to enable proper work of move and resize when expo
    friend class Expo;

    private:
        std::vector<std::vector<FireWindow> > backgrounds;
        int damage;

        WinStack *wins;

        void handleEvent(XEvent xev);
        void wait(int timeout);
        void enableInputPass(Window win);
        void addExistingWindows(); // adds windows created before
                                   // we registered for SubstructureRedirect
        pollfd fd;

        int mousex, mousey; // pointer x, y

        int vwidth, vheight; // viewport size
        int vx, vy;          // viewport position

        std::unordered_map<uint, KeyBinding*> keys;
        std::unordered_map<uint, ButtonBinding*> buttons;
        std::unordered_map<uint, Hook*> hooks;

        template<class T>
        uint getFreeID(std::unordered_map<uint, T> *map);

        KeyCode switchWorkspaceBindings[4];

        std::vector<PluginPtr> plugins;
        void initDefaultPlugins();

        Window s0owner;

    public:
        Display *d;
        Window root;
        Window overlay;
        Window outputwin;

        int cntHooks;

        int width;
        int height;

        bool redraw = true; // should we redraw?
        bool terminate = false; // should main loop exit?

        float scaleX = 1, scaleY = 1; // used for operations which
                              // depend on mouse moving
                              // for ex. when using expo
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
        void addWindow(Window);
        FireWindow findWindow(Window win);
        FireWindow getActiveWindow();
        void destroyWindow(FireWindow win);
        std::vector<FireWindow> getWindowsOnViewport(int x, int y);

        static int onXError (Display* d, XErrorEvent* xev);
        static int onOtherWmDetected(Display *d, XErrorEvent *xev);
};
extern Core *core;
#endif
