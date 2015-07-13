#include "../include/wm.hpp"
#include "../include/opengl.hpp"

void ATSwitcher::init(Core *core) {
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

    if(xev.keycode == XKeysymToKeycode(core->d, XK_Tab)) {
        if(active)
            Terminate();
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

    windows.clear();
    windows = core->getWindowsOnViewport(core->getWorkspace());
    active = true;

    background = nullptr;
    auto it = windows.begin();
    while(it != windows.end()) { // find background window
        if((*it)->type == WindowTypeDesktop) {
            background = (*it),
            windows.erase(it);
            break;
        }
        ++it;
    }

    for(auto w : windows)
        w->norender = true,
        w->transform.scalation =
            glm::scale(glm::mat4(), glm::vec3(0.5, 0.5, 1)),
        OpenGLWorker::generateVAOVBO(w->attrib.width,
                                     w->attrib.height,
                                     w->vao, w->vbo);


    OpenGLWorker::transformed = true;
    if(background) {
        background->transform.color = glm::vec4(0.5, 0.5, 0.5, 1.);
        background->transform.translation = glm::translate(glm::mat4(),
                glm::vec3(0.f, 0.f, -1.0f));
        background->transform.scalation   = glm::scale(glm::mat4(),
                glm::vec3(1.49f, 1.49f, 1.f));
    }

    backward.active  = true;
    forward.active   = true;
    terminate.active = true;

    XGrabKeyboard(core->d, core->overlay, True,
            GrabModeAsync, GrabModeAsync, CurrentTime);

    index = 0;
    core->setRedrawEverything(true);
    render();
}

void ATSwitcher::reset() {

    auto size = windows.size();
    auto prev = (index + size - 1) % size;
    auto next = (index + size + 1) % size;

    windows[prev]->transform.translation  = glm::mat4();
    windows[next]->transform.translation  = glm::mat4();
    windows[index]->transform.translation = glm::mat4();

    windows[prev]->transform.rotation     = glm::mat4();
    windows[next]->transform.rotation     = glm::mat4();
    windows[index]->transform.rotation    = glm::mat4();

    windows[prev]->transform.scalation    = glm::mat4();
    windows[next]->transform.scalation    = glm::mat4();
    windows[index]->transform.scalation   = glm::mat4();

    windows[index]->norender = true;
    windows[next ]->norender = true;
    windows[prev ]->norender = true;
}

void ATSwitcher::Terminate() {

    active = false;
    reset();
    for(auto w : windows)
        w->norender = false,
        w->transform.scalation   =
        w->transform.translation =
        w->transform.rotation    = glm::mat4(),
        OpenGLWorker::generateVAOVBO(w->attrib.x,
                w->attrib.y,
                w->attrib.width,
                w->attrib.height,
                w->vao, w->vbo);

    XUngrabKeyboard(core->d, CurrentTime);
    OpenGLWorker::transformed = false;

    backward.active  = false;
    forward.active   = false;
    terminate.active = false;


    if(background)
        background->transform.color = glm::vec4(1, 1, 1, 1),
        background->transform.translation = glm::mat4(),
        background->transform.scalation   = glm::mat4();
    core->setRedrawEverything(false);
    core->dmg = core->getMaximisedRegion();
}

float ATSwitcher::getFactor(int x, int y, float percent) {
    GetTuple(width, height, core->getScreenSize());

    float d = std::sqrt(x * x + y * y);
    float d1 = std::sqrt(width * width +
                         height *height);
    if(d != 0)
        return percent * d1 / d;
    else
        return 0;
}

void ATSwitcher::render() {
    auto size = windows.size();
    if(size < 1)
        return;

    auto prev = (index + size - 1) % size;
    auto next = (index + size + 1) % size;

    windows[index]->norender = false;
    windows[next ]->norender = false;
    windows[prev ]->norender = false;


    if(prev == index) { // we have one window
        return; // so nothing more to render
    }
    core->redraw = true;

    windows[prev]->transform.translation =
        glm::translate(glm::mat4(), glm::vec3(-.6f, 0.f, -0.3f));
    windows[prev]->transform.rotation    =
        glm::rotate(glm::mat4(), float(M_PI / 6.), glm::vec3(0, 1, 0));

    windows[next]->transform.translation =
        glm::translate(glm::mat4(), glm::vec3( .6f, 0.f, -0.3f));
    windows[next]->transform.rotation    =
        glm::rotate(glm::mat4(), float(-M_PI / 6.), glm::vec3(0, 1, 0));


    float c1 = getFactor(windows[index]->attrib.width,
                         windows[index]->attrib.height,
                         0.5);

    float c2 = getFactor(windows[prev ]->attrib.width,
                         windows[prev ]->attrib.height,
                         0.4);

    float c3 = getFactor(windows[next ]->attrib.width,
                         windows[next ]->attrib.height,
                         0.4);

    windows[index]->transform.scalation = glm::scale(glm::mat4(),
            glm::vec3(c1, c1, 1.0));
    windows[prev ]->transform.scalation = glm::scale(glm::mat4(),
            glm::vec3(c2, c2, 1.0));
    windows[next ]->transform.scalation = glm::scale(glm::mat4(),
            glm::vec3(c3, c3, 1.0));

    core->focusWindow(windows[index]);
}

void ATSwitcher::moveLeft() {
    reset();
    index = (index - 1 + windows.size()) % windows.size();
    render();
    core->redraw = true;
}

void ATSwitcher::moveRight() {
    reset();
    index = (index + 1) % windows.size();
    render();
    core->redraw = true;
}
