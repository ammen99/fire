#include "../include/core.hpp"
#include "../include/opengl.hpp"
#include <signal.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <execinfo.h>
#include <cxxabi.h>

/* shdata keeps all the shared memory between first process and fork()
 * shdata[0] is a flag whether to restart or no
 * shdata[1] and shdata[2] are initial viewport x and y */

char *shdata;
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
                shdata[0] = 1;
            };
            core->addKey(&ref, true);
        }
};
#define Crash 101
#define max_frames 100

void print_trace(int nSig) {
    std::cout << "stack trace:\n";

    //storage array for stack trace address data
    void* addrlist[max_frames+1];

    // retrieve current stack addresses
    int addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void*));

    if (addrlen == 0) {
        std::cout << "  <empty, possibly corrupt>\n";
        return;
    }

    //resolve addresses into strings containing "filename(function+address)",
    //this array must be free()-ed
    char** symbollist=backtrace_symbols(addrlist, addrlen);

    //allocate string which will be filled with
    //the demangled function name
    size_t funcnamesize = 256;
    char* funcname=(char*)malloc(funcnamesize);

    //iterate over the returned symbol lines.skip the first, it is the
    //address of this function.
    for(int i = 1;i < addrlen; i++) {
        char*begin_name=0,*begin_offset=0,*end_offset=0;

        //findparenthesesand+addressoffsetsurroundingthemangledname:
        //./module(function+0x15c)[0x8048a6d]
        for(char* p = symbollist[i]; *p; ++p) {
            if(*p == '(')
                begin_name = p;
            else if(*p == '+')
                begin_offset = p;
            else if(*p == ')' && begin_offset){
                end_offset = p;
                break;
            }
        }

        if(begin_name && begin_offset && end_offset
                &&begin_name < begin_offset)
        {
            *begin_name++ = '\0';
            *begin_offset++ = '\0';
            *end_offset = '\0';

            //manglednameisnowin[begin_name,begin_offset)andcaller
            //offsetin[begin_offset,end_offset).nowapply
            //__cxa_demangle():

            int status;
            char*ret=abi::__cxa_demangle(begin_name,
                    funcname,&funcnamesize,&status);
            if(status==0){
                funcname=ret;//usepossiblyrealloc()-edstring
                printf("%s:%s+%s\n",
                        symbollist[i],funcname,begin_offset);
            }
            else{
                //demanglingfailed.OutputfunctionnameasaCfunctionwith
                //noarguments.
                printf("%s:%s()+%s\n",
                        symbollist[i],begin_name,begin_offset);
            }
        }
        else
        {
            //couldn'tparsetheline?printthewholeline.
            printf("%s\n",symbollist[i]);
        }
    }

    free(funcname);
    free(symbollist);

    exit(-1);
}



void signalHandle(int sig) {
    switch(sig) {
        case SIGINT:                 // if interrupted, then
            std::cout << "EXITING BECAUSE OF SIGINT" << std::endl;
            shdata[0] = 0;         // make main loop exit
            core->terminate = true;  // and make core exit
            break;

        default: // program crashed, so restart core
            err << "Crash Detected!!!!!!" << std::endl;
            shdata[0] = 1;

            // try to fully recover, i.e do not lose windows

            GetTuple(vx, vy, core->getWorkspace());

            shdata[1] = int(vx);
            shdata[2] = int(vy);

            print_trace(SIGSEGV);
            break;
    }
}

void runOnce() { // simulates launching a new program

    std::cout << "runonce" << std::endl;
                 // get the shared memory from the main process
    shmid = shmget(shmkey, shmsize, 0666);
    auto dataid = shmat(shmid, 0, 0);
    shdata = (char*)dataid;
    shdata[0] = 0;

    signal(SIGINT, signalHandle);
    signal(SIGSEGV, signalHandle);
    signal(SIGFPE, signalHandle);
    signal(SIGILL, signalHandle);

    Transform::proj = Transform::view =
    Transform::grot = Transform::gscl =
    Transform::gtrs = glm::mat4();

    std::cout << "set up everything" << std::endl;
    core = new Core(shdata[1], shdata[2]);
    std::cout << "Did init of core" << std::endl;
    core->setBackground("/home/ilex/Desktop/maxresdefault.jpg");
    std::cout << "set background" << std::endl;
    new Refresh();

    std::cout << "hier" << std::endl;
    core->loop();
    std::cout << "after loop" << std::endl;

    if(core->mainrestart)
        shdata[0] = 1;

    GetTuple(vx, vy, core->getWorkspace());

    shdata[1] = int(vx);
    shdata[2] = int(vy);
}

int main(int argc, const char **argv ) {

    signal(SIGINT, signalHandle);
    signal(SIGSEGV, signalHandle);
    signal(SIGFPE, signalHandle);
    signal(SIGILL, signalHandle);

    /* get a bit of shared memory
     * it is used to check if fork() wants us to exit
     * or just to reload(in case of crash for ex.) */

    shmid = shmget(shmkey, shmsize, 0666 | IPC_CREAT);
    auto dataid = shmat(shmid, 0, 0);
    shdata = (char*)dataid;

    if(dataid == (void*)(-1)) {
        std::cout << "Failed to get shared memory, aborting..." << std::endl;
        std::exit(-1);
    }
    shdata[0] = 1;
    shdata[1] = shdata[2] = 0;

    int times = 0;
    while(shdata[0]) {
        std::cout << "In cycle" << std::endl;
        if(times++) // print a message if there has been crash
                    // but note that it could have just been a refresh
            std::cout << "Crash detected or just refresh" << std::endl;

        auto pid = fork();
        if(pid == 0) {
            runOnce();   // run Core()
            std::exit(0);// and exit
        }
        int status = 0;
        waitpid(pid, &status, 0); // wait for fork() to finish
        std::cout << "end cycle" << std::endl;
    }

    shmdt(dataid);
    shmctl(shmid, IPC_RMID, NULL);

    std::cout << "Gracefully exiting" << std::endl;
}
