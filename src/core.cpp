#include "../include/commonincludes.hpp"
#include "../include/opengl.hpp"
#include "../include/winstack.hpp"
#include "../include/core.hpp"
#include "../include/wm.hpp"

#include <sstream>
#include <memory>
#include "jpeg.hpp"

bool wmDetected;

Core *core;
int refreshrate;

class CorePlugin : public Plugin {
    public:
        void init() {
            options.insert(newIntOption("rrate", 100));
            options.insert(newIntOption("vwidth", 3));
            options.insert(newIntOption("vheight", 3));
            options.insert(newStringOption("background", ""));
            options.insert(newStringOption("shadersrc", "/usr/local/share/"));
            options.insert(newStringOption("pluginpath", "/usr/local/lib/"));
            options.insert(newStringOption("plugins", ""));
        }
        void initOwnership() {
            owner->name = "core";
            owner->compatAll = true;
        }
        void updateConfiguration() {
            refreshrate = options["rrate"]->data.ival;
        }
};
PluginPtr plug; // used to get core options

Core::Core(int vx, int vy) {
    config = new Config();

    this->vx = vx;
    this->vy = vy;

    width = 1366;
    height = 768;

}
void Core::init() {

    initDefaultPlugins();

    /* load core options */
    plug->owner = std::make_shared<_Ownership>();
    plug->initOwnership();
    plug->init();
    config->setOptionsForPlugin(plug);
    plug->updateConfiguration();

    vwidth = plug->options["vwidth"]->data.ival;
    vheight= plug->options["vheight"]->data.ival;

    loadDynamicPlugins();

    //OpenGL::initOpenGL((*plug->options["shadersrc"]->data.sval).c_str());
    //core->setBackground((*plug->options["background"]->data.sval).c_str());

    for(auto p : plugins) {
        p->owner = std::make_shared<_Ownership>();
        p->initOwnership();
        regOwner(p->owner);
        p->init();
        config->setOptionsForPlugin(p);
        p->updateConfiguration();
    }

    addDefaultSignals();
}

Core::~Core(){
    for(auto p : plugins) {
        p->fini();
        if(p->dynamic)
            dlclose(p->handle);
        p.reset();
    }
}

void Core::run(const char *command) {
    auto pid = fork();

    if(!pid) {
        std::exit(execl("/bin/sh", "/bin/sh", "-c", command, NULL));
    }
}

Hook::Hook() : active(false) {}

void Core::add_hook(Hook *hook){
    if(hook)
        hook->id = nextID++,
        hooks.push_back(hook);
}

void Core::rem_hook(uint key) {
    auto it = std::remove_if(hooks.begin(), hooks.end(), [key] (Hook *hook) {
                if(hook && hook->id == key) {
                        hook->disable();
                        return true;
                }
                else
                    return false;
            });

    hooks.erase(it, hooks.end());
}

void Core::run_hooks() {
    for(auto h : hooks) {
        if(h->getState())
            h->action();
    }
}

void Core::add_effect(EffectHook *hook){
    if(!hook) return;

    hook->id = nextID++;

    if(hook->type == EFFECT_OVERLAY)
        effects.push_back(hook);
    else if(hook->type == EFFECT_WINDOW)
        hook->win->effects[hook->id] = hook;
}

void Core::rem_effect(uint key, FireWindow w) {
    if(w == nullptr) {
        auto it = std::remove_if(effects.begin(), effects.end(),
                [key] (EffectHook *hook) {
            if(hook && hook->id == key) {
                hook->disable();
                return true;
            }
            else
                return false;
        });

        effects.erase(it, effects.end());
    }
    else {
        auto it = w->effects.find(key);
        it->second->disable();
        w->effects.erase(it);
    }
}

bool Hook::getState() { return this->active; }

void Hook::enable() {
    if(active)
        return;
    active = true;
    core->cntHooks++;
}

void Hook::disable() {
    if(!active)
        return;

    active = false;
    core->cntHooks--;
}

void KeyBinding::enable() {
    if(active) return;
    active = true;
}

void KeyBinding::disable() {
    if(!active) return;

    active = false;
}

void Core::add_key(KeyBinding *kb, bool grab) {
    if(!kb) return;
    keys.push_back(kb);
    if(grab) kb->enable();
}

void Core::rem_key(uint key) {
    auto it = std::remove_if(keys.begin(), keys.end(), [key] (KeyBinding *kb) {
                if(kb) return kb->id == key;
                else return true;
            });

    keys.erase(it, keys.end());
}

void ButtonBinding::enable() {
    if(active) return;
    active = true;
}

void ButtonBinding::disable() {
    if(!active) return;
    active = false;
}

void Core::add_but(ButtonBinding *bb, bool grab) {
    if(!bb) return;

    buttons.push_back(bb);
    if(grab) bb->enable();
}

void Core::rem_but(uint key) {
    auto it = std::remove_if(buttons.begin(), buttons.end(), [key] (ButtonBinding *bb) {
                if(bb) return bb->id == key;
                else return true;
            });
    buttons.erase(it, buttons.end());
}

void Core::regOwner(Ownership owner) {
    owners.insert(owner);
}

bool Core::activateOwner(Ownership owner) {

    if(!owner) {
        std::cout << "Error detected ?? calling with nullptr!!1" << std::endl;
        return false;
    }

    if(owner->active || owner->special) {
        owner->active = true;
        return true;
    }

    for(auto act_owner : owners)
        if(act_owner && act_owner->active) {
            bool owner_in_act_owner_compat =
                act_owner->compat.find(owner->name) != act_owner->compat.end();

            bool act_owner_in_owner_compat =
                owner->compat.find(act_owner->name) != owner->compat.end();


            if(!owner_in_act_owner_compat && !act_owner->compatAll)
                return false;

            if(!act_owner_in_owner_compat && !owner->compatAll)
                return false;
        }

    owner->active = true;
    return true;
}

bool Core::deactivateOwner(Ownership owner) {
    owner->ungrab();
    owner->active = false;
    return true;
}

//TODO: implement grab/ungrab keyboard and pointer
void Core::grab_pointer() {}
void Core::ungrab_pointer() {}
void Core::grab_keyboard() {}
void Core::ungrad_keyboard() {}

void Core::addSignal(std::string name) {
    if(signals.find(name) == signals.end())
        signals[name] = std::vector<SignalListener*>();
}

void Core::triggerSignal(std::string name, SignalListenerData data) {
    if(signals.find(name) != signals.end()) {
        std::vector<SignalListener*> toTrigger;
        for(auto proc : signals[name])
            toTrigger.push_back(proc);

        for(auto proc : signals[name])
            proc->action(data);
    }
}

void Core::connectSignal(std::string name, SignalListener *callback){
    addSignal(name);
    callback->id = nextID++;
    signals[name].push_back(callback);
}

void Core::disconnectSignal(std::string name, uint id) {
    auto it = std::remove_if(signals[name].begin(), signals[name].end(),
            [id](SignalListener *sigl){
            return sigl->id == id;
            });

    signals[name].erase(it, signals[name].end());
}

void Core::addDefaultSignals() {

    /* single element: the window */
    addSignal("create-window");
    addSignal("destroy-window");

    /* move-window is triggered when a window is moved
     * Data contains 3 elements:
     * 1. raw pointer to FireWin
     * 2. dx
     * 3. dy */

    addSignal("move-window");

    addSignal("move-request");
    addSignal("resize-request");

}

void Core::add_window(wlc_handle view) {
    FireWindow w = std::make_shared<FireWin>(view);
    windows[view] = w;
    printf("add window \n");

    wlc_view_bring_to_front(view);
    wlc_view_focus(view);

    wlc_view_set_mask(view, 1);
}

void Core::focus_window(FireWindow win) {
    if(!win) return;
    auto id = win->get_id();
    wlc_view_focus(id);
}

FireWindow Core::find_window(wlc_handle handle) {
    auto it = windows.find(handle);
    if(it == windows.end()) return nullptr;
    return it->second;
}

FireWindow Core::get_active_window() {
    return find_window(get_top_window(wlc_get_focused_output(), 0));
}

wlc_handle Core::get_top_window(wlc_handle output, size_t offset) {
    size_t memb;
    const wlc_handle *views = wlc_output_get_views(output, &memb);

    for(int i = memb - 1; i >= 0; i--) {
        auto w = find_window(views[i]);
        if(w && w->is_visible()) return w->get_id();
    }

    return 0;
}

void Core::for_each_window(WindowCallbackProc call) {
    size_t num;
    const wlc_handle* views = wlc_output_get_views(wlc_get_focused_output(), &num);

    for(int i = num - 1; i >= 0; i--) {
        auto w = find_window(views[i]);
        if(w) call(w);
    }
}


void Core::close_window(FireWindow win) {
    assert(win);

    printf("close windows\n");
    wlc_view_close(win->get_id());
    focus_window(get_active_window());
}

void Core::remove_window(FireWindow win) {
    assert(win);

    windows.erase(win->get_id());
    win.reset();
}

bool Core::check_key(KeyBinding *kb, uint32_t key, uint32_t mod) {
    if(!kb->active)
        return false;

    if(kb->key != key)
        return false;

    if(kb->mod != mod)
        return false;

    return true;
}

bool Core::check_but_press(ButtonBinding *bb, uint32_t button, uint32_t mod) {

    //printf("check but press %d\n", mod == WLC_BIT_MOD_ALT);
    if(!bb->active)
        return false;


    if(bb->type != BindingTypePress)
        return false;

    if(bb->mod != mod)
        return false;

    if(bb->button != button)
        return false;

    return true;
}

bool Core::check_but_release(ButtonBinding *bb, uint32_t button, uint32_t mod) {
    if(!bb->active)
        return false;

    if(bb->type != BindingTypeRelease)
        return false;

    if(bb->button != button)
        return false;

    return true;
}

bool Core::process_key_event(uint32_t key_in, uint32_t mod, wlc_key_state state) {
    if(state == WLC_KEY_STATE_RELEASED) return false;

    if(key_in == XKB_KEY_r && (mod & WLC_BIT_MOD_ALT)) {
        run("dmenu_run");
        //wlc_exec("weston-terminal", t);
    }

    for(auto key : keys) {
        if(check_key(key, key_in, mod)) {
            key->action(Context(0, 0, key_in, mod));
            return true;
        }
    }

    return false;
}


bool Core::process_button_event(uint32_t button, uint32_t mod,
        wlc_button_state state, wlc_point point) {

    mousex = point.x;
    mousey = point.y;

    for(auto but : buttons) {
        if(state == WLC_BUTTON_STATE_PRESSED && check_but_press(but, button, mod)) {
            but->action(Context(mousex, mousey, 0, 0));
            if(mod != 0) /* disable grabbing of pointer button without mod */
                return true;
            else
                break;
        }

        if(state == WLC_BUTTON_STATE_RELEASED && check_but_release(but, button, mod)) {
            but->action(Context(mousex, mousey, 0, 0));
            return true;
        }
    }

    return false;
}

bool Core::process_pointer_motion_event(wlc_point point) {

    mousex = point.x;
    mousey = point.y;

    return false;
}

#define Second 1000000
#define MaxDelay 1000
#define MinRR 61

std::tuple<int, int> Core::getWorkspace() {
    return std::make_tuple(vx, vy);
}

std::tuple<int, int> Core::getWorksize() {
    return std::make_tuple(vwidth, vheight);
}

std::tuple<int, int> Core::getScreenSize() {
    return std::make_tuple(1366, 768);
    return std::make_tuple(width, height);
}

std::tuple<int, int> Core::getMouseCoord() {
    return std::make_tuple(mousex, mousey);
}

FireWindow Core::getWindowAtPoint(int x, int y) {

    FireWindow chosen = nullptr;

    for_each_window([x,y, &chosen] (FireWindow w) {
            if(w->is_visible() && point_inside({x, y}, w->attrib)) {
                if(chosen == nullptr) chosen = w;
            }
    });

    return chosen;
}

void Core::switchWorkspace(std::tuple<int, int> nPos) {
    auto nx = std::get<0> (nPos);
    auto ny = std::get<1> (nPos);

    if(nx >= vwidth || ny >= vheight || nx < 0 || ny < 0)
        return;

    auto dx = (vx - nx) * width;
    auto dy = (vy - ny) * height;

    wlc_geometry view = {{0, 0}, {(uint)width, (uint) height}};

    //glm::mat4 translate_matrix = glm::translate(glm::mat4(), glm::vec3(2 * dx / float(sw),))

    using namespace std::placeholders;
    auto proc = [=] (FireWindow w) {
        //if(w->state & WindowStateSticky)
        //    return;


        bool has_been_before = rect_inside(view, w->attrib);

        w->attrib.origin.x += dx;
        w->attrib.origin.y -= dy;


        if(has_been_before && rect_inside(view, w->attrib))
            w->move(w->attrib.origin.x, w->attrib.origin.y);

        else
            w->transform.translation = glm::translate(w->transform.translation,
                    glm::vec3(2 * dx / float(width), -2 * dy / float(height), 0.0));
    };

    for_each_window(proc);

    vx = nx;
    vy = ny;

    auto ws = getWindowsOnViewport(this->getWorkspace());

    for(int i = ws.size() - 1; i >= 0; i--)
        focus_window(ws[i]);
}

std::vector<FireWindow> Core::getWindowsOnViewport(std::tuple<int, int> vp) {
    auto x = std::get<0>(vp);
    auto y = std::get<1>(vp);

    wlc_geometry view;

    view.origin.x = (x - vx) * width;
    view.origin.y = (y - vy) * height;
    view.size.w = width;
    view.size.h = height;

    std::vector<FireWindow> ret;

    auto proc = [view, &ret] (FireWindow w) {
        if(rect_inside(view, w->attrib))
            ret.push_back(w);
    };

    for_each_window(proc);

    return ret;
}

void Core::getViewportTexture(std::tuple<int, int> vp,
        GLuint &fbuff, GLuint &texture) {

//    OpenGL::useDefaultProgram();
//    if(fbuff == -1 || texture == -1)
//        OpenGL::prepareFramebuffer(fbuff, texture);
//    OpenGL::preStage(fbuff);
//    glClear(GL_COLOR_BUFFER_BIT);
//
//    GetTuple(x, y, vp);
//    FireWin::allDamaged = true;
//
//    auto view = getRegionFromRect((x - vx) * width, (y - vy) * height,
//                       (x - vx + 1) * width, (y - vy + 1) * height);
//
//    glm::mat4 off = glm::translate(glm::mat4(),
//            glm::vec3(2 * (vx - x), 2 * (y - vy), 0));
//    glm::mat4 save = Transform::gtrs;
//    Transform::gtrs *= off;
//
//    Region tmp = XCreateRegion();
//    std::vector<FireWindow> winsToDraw;
//
//    auto proc = [view, tmp, &winsToDraw](FireWindow win) {
//        if(!win->isVisible() || !win->region)
//            return;
//
//        XIntersectRegion(view, win->region, tmp);
//        if(!XEmptyRegion(tmp))
//            winsToDraw.push_back(win);
//    };
//    wins->forEachWindow(proc);
//
//    auto it = winsToDraw.rbegin();
//    while(it != winsToDraw.rend())
//        (*it++)->render();
//
//    Transform::gtrs = save;
//    OpenGL::API.glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

namespace {
    int fullRedraw = 0;
}

int Core::getRefreshRate() {
    return refreshrate;
}

#define uchar unsigned char
namespace {
    GLuint getFilledTexture(int w, int h, uchar r, uchar g, uchar b, uchar a) {
        int *arr = new int[w * h * 4];
        for(int i = 0; i < w * h; i = i + 4) {
            arr[i + 0] = r;
            arr[i + 1] = g;
            arr[i + 2] = b;
            arr[i + 3] = a;
        }
        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA,
                w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, arr);

        glBindTexture(GL_TEXTURE_2D, 0);
        return tex;
    }
}

GLuint Core::get_background() {
    if(background != -1) return background;

    return (background = LoadJPEG(plug->options["background"]->data.sval->c_str()));
}

//void Core::setBackground(const char *path) {
//    std::cout << "[DD] Background file: " << path << std::endl;
//
//    auto texture = GLXUtils::loadImage(const_cast<char*>(path));
//
//    if(texture == -1)
//        texture = getFilledTexture(width, height, 128, 128, 128, 255);
//
//    uint vao, vbo;
//    OpenGL::generateVAOVBO(0, height, width, -height, vao, vbo);
//
//    backgrounds.clear();
//    backgrounds.resize(vheight);
//    for(int i = 0; i < vheight; i++)
//        backgrounds[i].resize(vwidth);
//
//    for(int i = 0; i < vheight; i++){
//        for(int j = 0; j < vwidth; j++) {
//
//            backgrounds[i][j] = std::make_shared<FireWin>(0, false);
//
//            backgrounds[i][j]->vao = vao;
//            backgrounds[i][j]->vbo = vbo;
//            backgrounds[i][j]->norender = false;
//            backgrounds[i][j]->texture  = texture;
//
//            backgrounds[i][j]->attrib.x = (j - vx) * width;
//            backgrounds[i][j]->attrib.y = (i - vy) * height;
//            backgrounds[i][j]->attrib.width  = width;
//            backgrounds[i][j]->attrib.height = height;
//
//            backgrounds[i][j]->type = WindowTypeDesktop;
//            backgrounds[i][j]->updateVBO();
//            backgrounds[i][j]->transform.color = glm::vec4(1,1,1,1);
//            backgrounds[i][j]->updateRegion();
//            wins->addWindow(backgrounds[i][j]);
//        }
//    }
//}

namespace {
    template<class A, class B> B unionCast(A object) {
        union {
            A x;
            B y;
        } helper;
        helper.x = object;
        return helper.y;
    }
}

PluginPtr Core::loadPluginFromFile(std::string path, void **h) {
    void *handle = dlopen(path.c_str(), RTLD_NOW);
    if(handle == NULL){
        std::cout << "Error loading plugin " << path << std::endl;
        std::cout << dlerror() << std::endl;
        return nullptr;
    }

    auto initptr = dlsym(handle, "newInstance");
    if(initptr == NULL) {
        std::cout << "Failed to load newInstance from file " <<
            path << std::endl;
        std::cout << dlerror();
        return nullptr;
    }
    LoadFunction init = unionCast<void*, LoadFunction>(initptr);
    *h = handle;
    return std::shared_ptr<Plugin>(init());
}

void Core::loadDynamicPlugins() {
    std::stringstream stream(*plug->options["plugins"]->data.sval);
    auto path = *plug->options["pluginpath"]->data.sval + "/wayfire/";

    std::string plugin;
    while(stream >> plugin){
        if(plugin != "") {
            void *handle;
            auto ptr = loadPluginFromFile(path + "/lib" + plugin + ".so",
                    &handle);
            if(ptr) ptr->handle  = handle,
                    ptr->dynamic = true,
                    plugins.push_back(ptr);
        }
    }
}

template<class T>
PluginPtr Core::createPlugin() {
    return std::static_pointer_cast<Plugin>(std::make_shared<T>());
}

void Core::initDefaultPlugins() {
    plug = createPlugin<CorePlugin>();
    plugins.push_back(createPlugin<Focus>());
    plugins.push_back(createPlugin<Exit>());
    plugins.push_back(createPlugin<Close>());
    plugins.push_back(createPlugin<Refresh>());
}
