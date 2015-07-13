#ifndef FIRE_H
#define FIRE_H

#include "commonincludes.hpp"
#include "window.hpp"
#include "glx.hpp"
#include "plugin.hpp"
#include <queue>
#include <unordered_set>

#define InitialAge 50


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

struct Animation {
    virtual bool Step() = 0; // return true if continue, false otherwise
    virtual ~Animation();
};

struct AnimationHook {
    private:
        Animation *anim;
        Hook hook;
        void step();

    public:
        AnimationHook(Animation*, Core*); // anim is destroyed at the end
                                          // so should not be used anymore
                                          // in caller
        ~AnimationHook();
};

struct Fade : public Animation {

    enum Mode { FadeIn = 1, FadeOut = -1 };
    FireWindow win;
    Mode mode;
    int progress = 0;
    int maxstep = 0;
    int target = 0;
    bool destroy;
    bool savetr; // used to restore transparency

    Fade(FireWindow _win, Mode _mode = FadeIn,
            int durationms = 1000);
    bool Step();
};


class Expo;

#define GetTuple(x,y,t) auto x = std::get<0>(t); \
                        auto y = std::get<1>(t)
class Core {

    // used to enable proper work of move and resize when expo
    friend class Expo;
    // used to optimize idle time by counting hooks(cntHooks)
    friend struct Hook;

    private:
        std::vector<std::vector<FireWindow> > backgrounds;

        int mousex, mousey; // pointer x, y
        int cntHooks;

        int width;
        int height;
        int vwidth, vheight; // viewport size
        int vx, vy;          // viewport position

        std::unordered_map<uint, KeyBinding*> keys;
        std::unordered_map<uint, ButtonBinding*> buttons;
        std::unordered_map<uint, Hook*> hooks;
        std::unordered_set<Ownership> owners;

        std::vector<PluginPtr> plugins;
        void initDefaultPlugins();
        template<class T>
        PluginPtr createPlugin();
    public:

        /* warning!!!
         * no plugin should change these! */
        Display *d;
        Window root;
        Window overlay;
        Window outputwin;
        Window s0owner;
        int damage;


        bool redraw = true; // should we redraw?
        bool terminate = false; // should main loop exit?
        bool mainrestart = false; // should main() restart us?
        bool resetDMG;
        Region dmg;

        float scaleX = 1, scaleY = 1; // used for operations which
                              // depend on mouse moving
                              // for ex. when using expo

        void setBackground(const char *path);

        template<class T>
        uint getFreeID(std::unordered_map<uint, T> *map);

        uint addKey(KeyBinding *kb, bool grab = false);
        void remKey(uint id);

        uint addBut(ButtonBinding *bb, bool grab = false);
        void remBut(uint id);

        uint addHook(Hook*);
        void remHook(uint);

        void regOwner(Ownership owner);
        bool checkKey(KeyBinding *kb, XKeyEvent xkey);
        bool checkButPress  (ButtonBinding *bb, XButtonEvent xb);
        bool checkButRelease(ButtonBinding *bb, XButtonEvent xb);
        bool activateOwner  (Ownership owner);
        bool deactivateOwner(Ownership owner);

    private:
        void handleEvent(XEvent xev);
        void wait(int timeout);
        void enableInputPass(Window win);
        void addExistingWindows(); // adds windows created before
                                   // we registered for SubstructureRedirect
        WinStack *wins;
        pollfd fd;

    public:
        ~Core();
        void loop();
        Core(int vx, int vy); // initial viewport position

        void run(char *command);
        void renderAllWindows();

        void addWindow(XCreateWindowEvent);
        void addWindow(Window);
        void focusWindow(FireWindow win);
        void closeWindow(FireWindow win);
        void removeWindow(FireWindow win);
        void mapWindow(FireWindow win);
        void unmapWindow(FireWindow win);
        void damageWindow(FireWindow win);
        int getRefreshRate();


        FireWindow findWindow(Window win);
        FireWindow getActiveWindow();
        FireWindow getWindowAtPoint(int x, int y);

        Region getMaximisedRegion();
        Region getRegionFromRect(int tlx, int tly, int brx, int bry);
        /* use this function to draw all windows
         * but do not forget to turn it off
         * as it is extremely bad for performance */
        void setRedrawEverything(bool value);

        std::vector<FireWindow> getWindowsOnViewport(std::tuple<int, int>);
        void switchWorkspace(std::tuple<int, int>);
        std::tuple<int, int> getWorkspace ();
        std::tuple<int, int> getWorksize  ();
        std::tuple<int, int> getScreenSize();

        std::tuple<int, int> getMouseCoord();


        static int onXError (Display* d, XErrorEvent* xev);
        static int onOtherWmDetected(Display *d, XErrorEvent *xev);
};
extern Core *core;
#endif
