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
    std::cout << "ro get shmid = " << shmid << std::endl;
    std::cout << "ro erno = " << errno << std::endl;
    auto dataid = shmat(shmid, 0, 0);
    std::cout << "ro got shmat = " << dataid;
    restart = (char*)dataid;



    *restart = 0;

    Transform::proj = Transform::view =
    Transform::grot = Transform::gscl =
    Transform::gtrs = glm::mat4();

    core = new Core();
    core->setBackground("/tarball/backgrounds/last.jpg");
    new Refresh();
    core->loop();
}

int main(int argc, const char **argv ) {

    std::cout << "Startup" << std::endl;
    signal(SIGINT, signalHandle);
    signal(SIGSEGV, signalHandle);
    signal(SIGFPE, signalHandle);
    signal(SIGILL, signalHandle);

    std::cout << "Registered Signals" << std::endl;

    shmid = shmget(shmkey, shmsize, 0666 | IPC_CREAT);

    std::cout << "Get shmid = " << shmid << std::endl;

    std::cout << "Errno = " << errno << std::endl;


    auto dataid = shmat(shmid, 0, 0);
    restart = (char*)dataid;

    std::cout << "Errno = " << errno << std::endl;
    std::cout << "Got ids" << std::endl;

    std::cout << "Memory is " << std::endl;
    std::cout << dataid << std::endl;
    if(dataid == 0 || restart == 0) {
        std::cout << "Got shared memory 0" << std::endl;
    }

    std::cout << "Memory is " << restart << std::endl;
    *restart = 1;
    std::cout << "Got everything" << std::endl;

    while(*restart) {
        auto pid = fork();
        if(pid == 0) {
            std::cout << "In fork" << std::endl;
            runOnce();
            std::exit(0);
        }
        int status = 0;
        waitpid(pid, &status, 0);
    }

    shmdt(dataid);
    shmctl(shmid, IPC_RMID, NULL);
}
