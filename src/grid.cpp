#include "../include/wm.hpp"
#include <algorithm>

Grid::GridWindow::GridWindow(Window win, int x, int y, int w, int h):id(win) {
    size.x = x;
    size.y = y;
    size.width  = w;
    size.height = h;
}

void Grid::init() {
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
        keys[i].active = true;
        keys[i].type   = BindingTypePress;
        core->addKey(&keys[i], true);
    }
}

void Grid::toggleMaxim(FireWindow win) {
    auto it = std::find_if(wins.begin(), wins.end(),
            [win](GridWindow w) {
                return win->id == w.id;
            });

    if(it == wins.end()) { // we haven't maximized this window, so
                           // maximize it and add it to window list
        std::cout << "maximizing window";
        wins.push_back(GridWindow(win->id, win->attrib.x, win->attrib.y,
                    win->attrib.width, win->attrib.height));

        auto t = core->getScreenSize();
        auto w = std::get<0> (t);
        auto h = std::get<1> (t);

        WinUtil::moveWindow(win, 0, 0);
        WinUtil::resizeWindow(win, w, h);
        return;
    }

    // we have already maximized window, so restore its previous state
    GridWindow w = *it;
    WinUtil::moveWindow(win, w.size.x, w.size.y);
    WinUtil::resizeWindow(win, w.size.width, w.size.height);
    wins.erase(it); // and remove it so we can maximize it again
}

void Grid::getSlot(int n, int &x, int &y, int &w, int &h) {

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

void Grid::handleKey(Context *ctx) {
    auto win = core->getActiveWindow();
    if(!win)
        return;

    for(int i = 1; i < 10; i++) {
        if(ctx->xev.xkey.keycode == codes[i]) {
            if(i == 5)
                toggleMaxim(win);
            else {
                int x, y, w, h;
                getSlot(i, x, y, w, h);
                WinUtil::moveWindow  (win, x, y);
                WinUtil::resizeWindow(win, w, h);
            }
        }
    }
}
