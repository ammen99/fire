
#include "../include/core.hpp"
#include "../include/opengl.hpp"

int main(int argc, const char **argv ) {
    google::InitGoogleLogging(argv[0]);

    core = new Core();    
    WinUtil::init();

    GLXUtils::createNewContext(core->overlay);
    GLXUtils::initGLX();
    OpenGLWorker::initOpenGL("/home/ilex/work/cwork/fire/shaders");

    core->setBackground("/tarball/backgrounds/last.jpg");

    core->loop();

    return 0;
}
