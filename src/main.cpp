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

void signalHandle(int sig) {
    switch(sig) {
        case SIGINT:                 // if interrupted, then
            restart = false;         // make main loop exit
            core->terminate = true;  // and make core exit
            break;

        case SIGSEGV: // program crashed, so restart core
            err << "Crash Detected!!!!!!" << std::endl;
            core->terminate = true;
            break;
    }
}

void runOnce() { // simulates launching a new program
    Transform::proj = Transform::view =
    Transform::grot = Transform::gscl =
    Transform::gtrs = glm::mat4();

    core = new Core();
    core->setBackground("/tarball/backgrounds/last.jpg");
    Refresh *r = new Refresh();
    core->loop();
    delete r;
    delete core;
}

int main(int argc, const char **argv ) {
    //google::InitGoogleLogging(argv[0]);
    signal(SIGINT, signalHandle);
    signal(SIGSEGV, signalHandle);
    while(restart)
        runOnce();

    return 0;
}
