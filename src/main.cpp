
#include "../include/core.hpp"
#include "../include/opengl.hpp"

int main(int argc, const char **argv ) {
    google::InitGoogleLogging(argv[0]);
    core = new Core();    
    WindowWorker::init();

    GLXWorker::createNewContext(core->overlay);
    GLXWorker::initGLX();

    OpenGLWorker::initOpenGL();
    //OpenGLWorker::prepare();

    core->loop();
   

   return 0;
}
