#include "../include/core.hpp"
#include "../include/winstack.hpp"
#include "../include/opengl.hpp"

ATSwitcher::ATSwitcher(Core *core) {
    using namespace std::placeholders;

    active = false;

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

    //rnd.action = std::bind(std::mem_fn(&ATSwitcher::render), this);
    //core->addHook(&rnd);

}

void ATSwitcher::handleKey(Context *ctx) {
    auto xev = ctx->xev.xkey;
    err << "In handle Key";

    if(xev.keycode == XKeysymToKeycode(core->d, XK_Tab)) {
        if(active)
            Terminate();
            //moveLeft();
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
    //rnd.enable();
    windows.clear();
    windows = core->getWindowsOnViewport(core->vx, core->vy);
    active = true;

    err << "got window list" << windows.size();
    for(auto w : windows)
        w->transform.scalation =
            glm::scale(glm::mat4(), glm::vec3(0.8, 0.8, 1)),
        OpenGLWorker::generateVAOVBO(w->attrib.width,
                                     w->attrib.height,
                                     w->vao, w->vbo);

    err << "searching for background";
    background = nullptr;
    auto it = windows.begin();
    while(it != windows.end()) { // find background window
        if((*it)->type == WindowTypeDesktop) {
            err << "found background";
            background = (*it),
            windows.erase(it);
            break;
        }
        ++it;
    }

    err << "After background";
    err << background->id << std::endl;

    if(background)
        background->transform.color = glm::vec4(0.5, 0.5, 0.5, 1.),
        background->transform.scalation = glm::mat4(),
        OpenGLWorker::generateVAOVBO(0,
                                    core->height,
                                    core->width,
                                    -core->height,
                                    background->vao, background->vbo);

    backward.active  = true;
    forward.active   = true;
    terminate.active = true;

    XGrabKeyboard(core->d, core->overlay, True,
            GrabModeAsync, GrabModeAsync, CurrentTime);

    index = 0;
    render();
    err << "end of init";
}

void ATSwitcher::Terminate() {

    active = false;
    for(auto w : windows)
        w->transform.scalation = glm::mat4(),
        w->transform.trnslation =
        w->transform.translation = glm::mat4(),
        OpenGLWorker::generateVAOVBO(w->attrib.x,
                w->attrib.y,
                w->attrib.width,
                w->attrib.height,
                w->vao, w->vbo);

    core->wins->focusWindow(windows[index]);
    XUngrabKeyboard(core->d, CurrentTime);

    backward.active  = false;
    forward.active   = false;
    terminate.active = false;

    background->transform.color = glm::vec4(1, 1, 1, 1);
}

void ATSwitcher::render() {
    auto size = windows.size();
    if(size < 1)
        return;

    err << "rendering";
    auto prev = (index + size - 1) % size;
    auto next = (index + size + 1) % size;

    if(prev == index) { // we have one window
        return; // so nothing more to render
    }
    core->redraw = true;

    windows[prev]->transform.translation =
        glm::translate(glm::mat4(), glm::vec3(-1.f, 0.f, -1));

    windows[next]->transform.translation =
        glm::translate(glm::mat4(), glm::vec3(+1.f, 0.f, -1));

//    windows[prev]->transform.translation =
//    windows[next]->transform.translation = glm::mat4();

}

void ATSwitcher::moveLeft() {
    index = (index - 1 + windows.size()) % windows.size();
    render();
    core->redraw = true;
}

void ATSwitcher::moveRight() {
    index = (index + 1) % windows.size();
    render();
    core->redraw = true;
}
