#include "../include/core.hpp"
#include "../include/winstack.hpp"

ATSwitcher::ATSwitcher(Core *core) {
    using namespace std::placeholders;

    initiate.mod = Mod1Mask;
    initiate.key = XKeysymToKeycode(core->d, XK_Tab);
    initiate.type = BindingTypePress;
    initiate.action = std::bind(std::mem_fn(&ATSwitcher::handleKey), this, _1);
    initiate.active = true;
    core->addKey(&initiate, true);

    forward.mod = 0;
    forward.key = XKeysymToKeycode(core->d, XK_Right);
    forward.type = BindingTypePress;
    forward.action = initiate.action;
    forward.active = false;
    core->addKey(&forward);

    backward.mod = 0;
    backward.key = XKeysymToKeycode(core->d, XK_Left);
    backward.type = BindingTypePress;
    backward.action = initiate.action;
    backward.active = false;
    core->addKey(&backward);

    terminate.mod = 0;
    terminate.key = XKeysymToKeycode(core->d, XK_KP_Enter);
    terminate.type = BindingTypePress;
    terminate.action = initiate.action;
    terminate.active = false;
    core->addKey(&terminate);

    rnd.action = std::bind(std::mem_fn(&ATSwitcher::render), this);
    core->addHook(&rnd);

}

void ATSwitcher::handleKey(Context *ctx) {
    auto xev = ctx->xev.xkey;

    if(xev.keycode == XKeysymToKeycode(core->d, XK_Tab)) {
        if(active)
            moveLeft();
        else
            Initiate();
    }

    if(xev.keycode == XKeysymToKeycode(core->d, XK_Left))
        moveLeft();

    if(xev.keycode == XKeysymToKeycode(core->d, XK_Right))
        moveRight();

    if(xev.keycode == XKeysymToKeycode(core->d, XK_KP_Enter))
        Terminate();
}

void ATSwitcher::Initiate() {
    rnd.enable();
    windows.clear();
    windows = core->getWindowsOnViewport(core->vx, core->vy);
    active = true;
    for(auto w : windows)
        w->norender = true,
        w->transform.scalation =
            glm::scale(glm::mat4(), glm::vec3(0.5, 0.5, 0.5));


    background = nullptr;
    auto it = windows.begin();
    while(it != windows.end()) { // find background window
        if((*it)->type == WindowTypeDesktop) {
            background = (*it),
            (*it)->norender = false, // we don't render background
            windows.erase(it);
            break;
        }
        ++it;
    }

    if(background)
        background->transform.color = glm::vec4(0.5, 0.5, 0.5, 1.);

    index = 0;
}

void ATSwitcher::Terminate() {
    if(background)
        windows.push_back(background);

    active = false;
    rnd.disable();
    for(auto w : windows)
        w->norender = false,
        w->transform.scalation = glm::mat4();

    core->wins->focusWindow(windows[index]);
}

void ATSwitcher::render() {
    auto size = windows.size();
    auto prev = (index + size - 1) % size;
    auto next = (index + size + 1) % size;

    WinUtil::renderWindow(windows[index]);

    if(prev == index) { // we have one window
        return; // so nothing more to render
    }

    windows[prev]->transform.translation =
        glm::translate(glm::mat4(), glm::vec3(-1.f, 0.f, -1));
    WinUtil::renderWindow(windows[prev]);

    windows[next]->transform.translation =
        glm::translate(glm::mat4(), glm::vec3(+1.f, 0.f, -1));
    WinUtil::renderWindow(windows[next]);

    windows[prev]->transform.translation =
    windows[next]->transform.translation = glm::mat4();
}

void ATSwitcher::moveLeft() {
    index = (index - 1 + windows.size()) % windows.size();
    core->redraw = true;
}

void ATSwitcher::moveRight() {
    index = (index + 1) % windows.size();
    core->redraw = true;
}
