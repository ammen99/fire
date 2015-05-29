#include "../include/core.hpp"
#include "../include/opengl.hpp"
#include <signal.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>


char *restart; // should we restart our app
constexpr int shmkey = 1010101234;
constexpr int shmsize = 8;
int shmid;

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
                *restart = 1;
            };
            core->addKey(&ref, true);
        }
};
#define Crash 101

void signalHandle(int sig) {
    switch(sig) {
        case SIGINT:                 // if interrupted, then
            *restart = 0;         // make main loop exit
            core->terminate = true;  // and make core exit
            break;

        default: // program crashed, so restart core
            err << "Crash Detected!!!!!!" << std::endl;
            *restart = 1;
            std::exit(0);
            break;
    }
}

void runOnce() { // simulates launching a new program

    shmid = shmget(shmkey, shmsize, 0666);
    auto dataid = shmat(shmid, 0, 0);
    restart = (char*)dataid;
    *restart = 0;

    signal(SIGINT, signalHandle);
    signal(SIGSEGV, signalHandle);
    signal(SIGFPE, signalHandle);
    signal(SIGILL, signalHandle);

    Transform::proj = Transform::view =
    Transform::grot = Transform::gscl =
    Transform::gtrs = glm::mat4();

    core = new Core();
    core->setBackground("/tarball/backgrounds/last.jpg");
    new Refresh();
    core->loop();
}

int main(int argc, const char **argv ) {

    signal(SIGINT, signalHandle);
    signal(SIGSEGV, signalHandle);
    signal(SIGFPE, signalHandle);
    signal(SIGILL, signalHandle);

    shmid = shmget(shmkey, shmsize, 0666 | IPC_CREAT);
    auto dataid = shmat(shmid, 0, 0);
    restart = (char*)dataid;

    if(dataid == (void*)(-1)) {
        std::cout << "Failed to get shared memory, aborting..." << std::endl;
        std::exit(-1);
    }
    *restart = 1;

    while(*restart) {
        auto pid = fork();
        if(pid == 0) {
            runOnce();
            std::exit(0);
        }
        int status = 0;
        waitpid(pid, &status, 0);
    }

    shmdt(dataid);
    shmctl(shmid, IPC_RMID, NULL);
}
