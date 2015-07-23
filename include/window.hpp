#include "commonincludes.hpp"

enum WindowType {
    WindowTypeNormal,
    WindowTypeWidget,
    WindowTypeModal,
    WindowTypeDock,
    WindowTypeDesktop,
    WindowTypeOther
};

enum WindowState {
    WindowStateSticky,
    WindowStateNormal,
    WindowStateSkipTaskbar,
    WindowStateFullscreen
};

class Transform {
    public: // applied to all windows
        static glm::mat4 proj;
        static glm::mat4 view;
        static glm::mat4 grot;
        static glm::mat4 gscl;
        static glm::mat4 gtrs;
    public:
        float stackID; // used to move the window forward if front
        glm::mat4 rotation;
        glm::mat4 scalation;
        glm::mat4 translation;
        glm::vec4 color;
    public:
        Transform();
        glm::mat4 compose();
};

extern Region output;
Region copyRegion(Region r);

struct SharedImage {
    XShmSegmentInfo shminfo;
    XImage *image;
    bool init = true;
    bool existing = false;
};

enum Layer {LayerAbove = 0, LayerNormal = 1, LayerBelow = 2};

class __FireWindow {
    public:

        static bool allDamaged;
        Window id;
        GLuint texture; // image on screen

        bool norender = false; // should we draw window?
        bool destroyed = false; // is window dead?
        bool transparent = false; // is the window transparent
        bool damaged = true;

        int mapTryNum = 5; // how many times have we tried to map this window?
        int keepCount = 0; // used to determine whether to destroy window
        Transform transform;

        bool disableVBOChange = false;
        GLuint vbo = -1;
        GLuint vao = -1;

        Damage damage;

        std::shared_ptr<__FireWindow> transientFor; // transientFor
        std::shared_ptr<__FireWindow> leader;
        Layer layer;

        char *name;
        WindowType type;
        XWindowAttributes attrib;
        Region region = nullptr;

        Pixmap pixmap = 0;
        SharedImage shared;


        bool shouldBeDrawn();
        void updateVBO();
        void updateRegion();
};

typedef std::shared_ptr<__FireWindow> FireWindow;

extern Atom winTypeAtom, winTypeDesktopAtom, winTypeDockAtom,
       winTypeToolbarAtom, winTypeMenuAtom, winTypeUtilAtom,
       winTypeSplashAtom, winTypeDialogAtom, winTypeNormalAtom,
       winTypeDropdownMenuAtom, winTypePopupMenuAtom, winTypeDndAtom,
       winTypeTooltipAtom, winTypeNotificationAtom, winTypeComboAtom;

extern Atom activeWinAtom;
extern Atom wmTakeFocusAtom;
extern Atom wmProtocolsAtom;
extern Atom wmClientLeaderAtom;
extern Atom wmNameAtom;
extern Atom winOpacityAtom;

class Core;

namespace WinUtil {
    void init();

    void renderWindow(FireWindow win);
    int setWindowTexture(FireWindow win);
    void initWindow(FireWindow win);
    void finishWindow(FireWindow win);

    void setInputFocusToWindow(Window win);

    void moveWindow(FireWindow win, int x, int y, bool configure = true);
    void resizeWindow(FireWindow win, int w, int h, bool configure = true);
    void syncWindowAttrib(FireWindow win);

    XVisualInfo *getVisualInfoForWindow(Window win);

    FireWindow getTransient(FireWindow win);
    FireWindow getClientLeader(FireWindow win);

    void getWindowName(FireWindow win, char *name);
    WindowType getWindowType(FireWindow win);
    int readProp(Window win, Atom prop, int def);
};
