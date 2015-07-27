#ifndef FIRE_H
#define FIRE_H

#include "commonincludes.hpp"
#include "window.hpp"
#include "glx.hpp"
#include "config.hpp"

class WinStack;

void print_trace();

struct Context{
    XEvent xev;
    Context(XEvent xev);
};

enum BindingType {
    BindingTypePress,
    BindingTypeRelease
};

struct Binding{
    bool active = false;
    BindingType type;
    uint mod;
    std::function<void(Context*)> action;
};

struct KeyBinding : Binding {
    KeyCode key;

    void enable();
    void disable();
};

struct ButtonBinding : Binding {
    uint button;

    void enable();
    void disable();
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
    virtual bool Run();
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
    bool run = true;
    bool restoretr = true;
    bool savetr; // used to restore transparency

    static int duration;

    Fade(FireWindow _win, Mode _mode = FadeIn);
    ~Fade();
    bool Step();
    bool Run();
};

/* render hooks are used to replace the renderAllWindows()
 * when necessary, i.e to render completely custom
 * image */

using RenderHook = std::function<void()>;

#define GetTuple(x,y,t) auto x = std::get<0>(t); \
                        auto y = std::get<1>(t)
class Core {

    // used to optimize idle time by counting hooks(cntHooks)
    friend struct Hook;

    private:
        WinStack *wins;
        pollfd fd;
        int cntHooks;
        int damage;
        Window s0owner;

        int mousex, mousey; // pointer x, y
        int width, height;
        int vwidth, vheight; // viewport size
        int vx, vy;          // viewport position

        std::vector<std::vector<FireWindow> > backgrounds;

        std::vector<PluginPtr> plugins;
        std::vector<KeyBinding*> keys;
        std::vector<ButtonBinding*> buttons;
        std::vector<Hook*> hooks;
        std::unordered_set<Ownership> owners;

        void handleEvent(XEvent xev);
        void wait(int timeout);
        void enableInputPass(Window win);
        void addExistingWindows(); // adds windows created before
                                   // we registered for SubstructureRedirect
                                   //
        void initDefaultPlugins();
        template<class T> PluginPtr createPlugin();
        PluginPtr loadPluginFromFile(std::string path, void **handle);
        void loadDynamicPlugins();

        void defaultRenderer();

        struct {
            RenderHook currentRenderer;
            bool replaced = true;
        } render;

    public:

        /* warning!!!
         * no plugin should change these! */
        Display *d;
        Window root;
        Window outputwin;
        Window overlay;

        bool terminate = false; // should main loop exit?
        bool mainrestart = false; // should main() restart us?

    public:
        Core(int vx, int vy);
        ~Core();
        void init();
        void loop();

        static int onXError (Display* d, XErrorEvent* xev);
        static int onOtherWmDetected(Display *d, XErrorEvent *xev);

        void run(char *command);
        FireWindow findWindow(Window win);
        FireWindow getActiveWindow();
        std::function<FireWindow(int,int)> getWindowAtPoint;

        void addWindow(XCreateWindowEvent);
        void addWindow(Window);
        void focusWindow(FireWindow win);
        void closeWindow(FireWindow win);
        void removeWindow(FireWindow win);
        void mapWindow(FireWindow win, bool xmap = true);
        void unmapWindow(FireWindow win);
        int getRefreshRate();

        void setBackground(const char *path);
        bool setRenderer(RenderHook rh);
        void setDefaultRenderer();

        void addKey (KeyBinding *kb, bool grab = false);
        void addBut (ButtonBinding *bb, bool grab = false);
        void addHook(Hook*);

        void regOwner(Ownership owner);
        bool checkKey(KeyBinding *kb, XKeyEvent xkey);
        bool checkButPress  (ButtonBinding *bb, XButtonEvent xb);
        bool checkButRelease(ButtonBinding *bb, XButtonEvent xb);
        bool activateOwner  (Ownership owner);
        bool deactivateOwner(Ownership owner);

        /* this function renders a viewport and
         * saves the image in texture which is returned */
        void getViewportTexture(std::tuple<int, int>, GLuint& fbuff,
                GLuint& tex);

        std::vector<FireWindow> getWindowsOnViewport(std::tuple<int, int>);
        void switchWorkspace(std::tuple<int, int>);

        std::tuple<int, int> getWorkspace ();
        std::tuple<int, int> getWorksize  ();
        std::tuple<int, int> getScreenSize();
        std::tuple<int, int> getMouseCoord();

        bool resetDMG;
        Region dmg;
        void damageRegion(Region r);
        void damageREGION(REGION r);
        Region getMaximisedRegion();
        Region getRegionFromRect(int tlx, int tly, int brx, int bry);
        REGION getREGIONFromRect(int tlx, int tly, int brx, int bry);
        /* use this function to draw all windows
         * but do not forget to turn it off
         * as it is extremely bad for performance */
        void setRedrawEverything(bool value);


        float scaleX = 1, scaleY = 1; // used for operations which
                              // depend on mouse moving
                              // for ex. when using expo
};
extern Core *core;
#endif
