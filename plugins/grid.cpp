#include "../include/core.hpp"
#include <algorithm>

#define GetProgress(start,end,curstep,steps) (((end)*(curstep)+(start) \
                                            *((steps)-(curstep)))/(steps))

// TODO: allow grid plugin to use FireWin::data

struct GridWindow {
    FireWindow win;
    XWindowAttributes size;
    GridWindow(){}
    GridWindow(FireWindow id, int x, int y, int w, int h) {
        this->win = id;
        size.x = x;
        size.y = y;
        size.width  = w;
        size.height = h;
    }
};

class Grid : public Plugin {

    std::vector<GridWindow> wins;
    KeyBinding keys[10];
    KeyCode codes[10];

    Hook rnd;
    int steps;
    int curstep;
    GridWindow currentWin;

    public:
    Grid() {
    }

    void init() {
        options.insert(newIntOption("duration", 200));

        codes[1] = XKeysymToKeycode(core->d, XK_KP_End);
        codes[2] = XKeysymToKeycode(core->d, XK_KP_Down);
        codes[3] = XKeysymToKeycode(core->d, XK_KP_Page_Down);
        codes[4] = XKeysymToKeycode(core->d, XK_KP_Left);
        codes[5] = XKeysymToKeycode(core->d, XK_KP_Begin);
        codes[6] = XKeysymToKeycode(core->d, XK_KP_Right);
        codes[7] = XKeysymToKeycode(core->d, XK_KP_Home);
        codes[8] = XKeysymToKeycode(core->d, XK_KP_Up);
        codes[9] = XKeysymToKeycode(core->d, XK_KP_Page_Up);

        using namespace std::placeholders;

        for(int i = 1; i < 10; i++) {
            keys[i].key    = codes[i];
            keys[i].mod    = ControlMask | Mod1Mask;
            keys[i].action = std::bind(std::mem_fn(&Grid::handleKey), this, _1);
            keys[i].type   = BindingTypePress;
            core->addKey(&keys[i], true);
        }

        rnd.action = std::bind(std::mem_fn(&Grid::step), this);
        core->addHook(&rnd);
    }

    void initOwnership(){
        owner->name = "grid";
        owner->compatAll = false;
    }

    void updateConfiguration() {
        steps = getSteps(options["duration"]->data.ival);
    }

    void step() {

        GetTuple(sw, sh, core->getScreenSize());
        float offx = GetProgress(0,
                currentWin.win->attrib.x - currentWin.size.x, curstep, steps);

        float offy = GetProgress(0,
                currentWin.win->attrib.y - currentWin.size.y, curstep, steps);

        offx /= (sw / 2.);
        offy /= (sh / 2.);

        float sclx = GetProgress(currentWin.win->attrib.width,
                           currentWin.size.width, curstep, steps);

        float scly = GetProgress(currentWin.win->attrib.height,
                           currentWin.size.height, curstep, steps);

        sclx /= currentWin.win->attrib.width;
        scly /= currentWin.win->attrib.height;


        float w2 = float(sw) / 2.;
        float h2 = float(sh) / 2.;

        float tlx = float(currentWin.win->attrib.x) - w2,
              tly = h2 - float(currentWin.win->attrib.y);

        float ntlx = sclx * tlx;
        float ntly = scly * tly;

        offx -= float(tlx - ntlx) / w2;
        offy += float(tly - ntly) / h2;

        currentWin.win->transform.translation =
            glm::translate(glm::mat4(), glm::vec3(-offx, offy, 0));
        currentWin.win->transform.scalation =
            glm::scale(glm::mat4(), glm::vec3(sclx, scly, 0));

        curstep++;
        if(curstep == steps) {
            currentWin.win->transform.translation = glm::mat4();
            currentWin.win->transform.scalation = glm::mat4();

            currentWin.win->move(currentWin.size.x, currentWin.size.y);
            currentWin.win->resize(currentWin.size.width,
                    currentWin.size.height);

            core->setRedrawEverything(false);
            core->dmg = core->getMaximisedRegion();
            rnd.disable();
        }
    }

    void toggleMaxim(FireWindow win, int &x, int &y, int &w, int &h) {
        auto it = std::find_if(wins.begin(), wins.end(),
                [win](GridWindow w) {
                return win->id == w.win->id;
                });

        if(it == wins.end()) { // we haven't maximized this window, so
            // maximize it and add it to window list
            std::cout << "maximizing window";
            wins.push_back(GridWindow(win, win->attrib.x, win->attrib.y,
                        win->attrib.width, win->attrib.height));

            GetTuple(sw, sh, core->getScreenSize());

            x = y = 0, w = sw, h = sh;
            return;
        }

        // we have already maximized window, so restore its previous state
        GridWindow gwin = *it;
        x = gwin.size.x, y = gwin.size.y;
        w = gwin.size.width, h = gwin.size.height;
        wins.erase(it); // and remove it so we can maximize it again
    }

    void getSlot(int n, int &x, int &y, int &w, int &h) {

        auto t = core->getScreenSize();
        auto width = std::get<0>(t);
        auto height= std::get<1>(t);

        int w2 = width  / 2;
        int h2 = height / 2;
        if(n == 7)
            x = 0, y = 0, w = w2, h = h2;
        if(n == 8)
            x = 0, y = 0, w = width, h = h2;
        if(n == 9)
            x = w2, y = 0, w = w2, h = h2;
        if(n == 4)
            x = 0, y = 0, w = w2, h = height;
        if(n == 6)
            x = w2, y = 0, w = w2, h = height;
        if(n == 1)
            x = 0, y = h2, w = w2, h = h2;
        if(n == 2)
            x = 0, y = h2, w = width, h = h2;
        if(n == 3)
            x = w2, y = h2, w = w2, h = h2;
    }

    void handleKey(Context *ctx) {
        auto win = core->getActiveWindow();
        if(!win)
            return;

        int x, y, w, h;
        for(int i = 1; i < 10; i++) {
            if(ctx->xev.xkey.keycode == codes[i]) {
                if(i == 5)
                    toggleMaxim(win, x, y, w, h);
                else
                    getSlot(i, x, y, w, h);
            }
        }

        currentWin.win = win;
        currentWin.size.x = x;
        currentWin.size.y = y;
        currentWin.size.width = w;
        currentWin.size.height = h;
        curstep = 0;
        rnd.enable();
        core->setRedrawEverything(true);
    }
};

extern "C" {
    Plugin *newInstance() {
        return new Grid();
    }
}
