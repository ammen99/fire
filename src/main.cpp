
#include "../include/core.hpp"
#include "../include/opengl.hpp"

int main(int argc, const char **argv ) {
    google::InitGoogleLogging(argv[0]);

    Transform::proj = Transform::view =
    Transform::grot = Transform::gscl =
    Transform::gtrs = glm::mat4();

    core = new Core();
    WinUtil::init();

    GLXUtils::initGLX();
    OpenGLWorker::initOpenGL("/home/ilex/work/cwork/fire/shaders");

    core->setBackground("/tarball/backgrounds/last.jpg");

    core->loop();

    return 0;
}
