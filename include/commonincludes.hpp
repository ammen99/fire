#ifndef COMMON_INCLUDES
#define COMMON_INCLUDES

#include <X11/Xlib.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/XShm.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xdamage.h>
#include <X11/keysym.h>
#include <X11/Xmu/Xmu.h>
#include <X11/Xmu/WinUtil.h>
#include <X11/Xregion.h>


#include <sys/time.h>
#include <poll.h>
#include <dlfcn.h>
#include <unistd.h>
#include <cstdlib>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <memory>
#include <mutex>
#include <cstring>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>


#include <GL/glew.h>
#include <GL/glx.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define ILUT_USE_OPENGL
#include <IL/ilut.h>

#include <fstream>

#ifdef YCM
#define private public
#endif

#define AllModifiers (ControlMask | ShiftMask | Mod1Mask \
        | Mod2Mask | Mod3Mask | Mod4Mask)

#endif

