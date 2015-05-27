#include "../include/core.hpp"
#include "../include/opengl.hpp"
#include <signal.h>

class Refresh { // keybinding to restart window manager
    KeyBinding ref;
    public:
        Refresh() {
            ref.key = XKeysymToKeycode(core->d, XK_r);
            ref.type = BindingTypePress;
            ref.active = true;
            ref.mod = ControlMask | Mod1Mask;
            ref.action = [] (Context *ctx) {
                core->terminate = true;
            };
            core->addKey(&ref, true);
        }
};


bool restart = true; // should we restart our app

void signalHandle(int sig) { // if interrupted, then
    restart = false;         // make main loop exit
    core->terminate = true;  // and make core exit
}

void runOnce() { // simulates launching a new program
    err << "running once" << std::endl;
    Transform::proj = Transform::view =
    Transform::grot = Transform::gscl =
    Transform::gtrs = glm::mat4();

    err << "Init core" << std::endl;
    core = new Core();
    err << "Core inited" << std::endl;
    core->setBackground("/tarball/backgrounds/last.jpg");
    err << "Backgroudn set" << std::endl;
    Refresh *r = new Refresh();
    err << "Refreshcom enabled" << std::endl;
    core->loop();
    err << "End of loop" << std::endl;
    delete r;
    delete core;
}

int main(int argc, const char **argv ) {
    //google::InitGoogleLogging(argv[0]);
    err << "Reg singal" << std::endl;
    signal(SIGINT, signalHandle);
    err << "here" << std::endl;
    while(restart)
        runOnce();

    return 0;
}
