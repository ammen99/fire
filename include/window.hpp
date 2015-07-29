#include "commonincludes.hpp"

enum WindowType {
    WindowTypeNormal,
    WindowTypeWidget,
    WindowTypeModal,
    WindowTypeDock,
    WindowTypeDesktop,
    WindowTypeUnknown
};

enum WindowState {
    WindowStateBase         = 0,
    WindowStateSticky       = 1,
    WindowStateHidden       = 2,
    WindowStateSkipTaskbar  = 4,
    WindowStateFullscreen   = 8,
    WindowStateMaxH         = 16,
    WindowStateMaxV         = 32,
    WindowStateAbove        = 64,
    WindowStateBelow        = 128,
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

struct WindowData {
};

#define GetData(type, win, name) ((type*)(win->data[(name)]))
#define ExistsData(win, name) ((win)->data.find((name)) != (win)->data.end())
#define AllocData(type, win, name) (win)->data[(name)] = new type()

class FireWin {
    public:

        FireWin(Window id, bool init = true);
        ~FireWin();
        /* this can be used by plugins to store
         * specific for the plugin data */
        std::unordered_map<std::string, WindowData*> data;

        static bool allDamaged;
        Window id;
        GLuint texture; // image on screen

        bool norender = false;       // should we draw window?
        bool destroyed = false;      // is window destroyed?
        bool transparent = false;    // is the window transparent?
        bool damaged = true;         // refresh window contents?
        bool visible = true;         // is window visible on screen?
        bool initialMapping = false; // is window already mapped?

        int keepCount = 0; // used to determine whether to destroy window
        Transform transform;

        bool disableVBOChange = false;
        GLuint vbo = -1;
        GLuint vao = -1;

        Damage damagehnd;

        std::shared_ptr<FireWin> transientFor; // transientFor
        std::shared_ptr<FireWin> leader;
        Layer layer;

        char *name;
        WindowType type;
        uint state = WindowStateBase;
        XWindowAttributes attrib;
        Region region = nullptr;

        Pixmap pixmap = 0;
        SharedImage shared;


        bool isVisible();
        bool shouldBeDrawn();
        void updateVBO();
        void updateRegion();
        void updateState();

        void addDamage();

        void move(int x, int y, bool configure = true);
        void resize(int w, int h, bool configure = true);
        void syncAttrib();
        void getInputFocus();

        void render();
        int  setTexture();
        void init();
        void fini();
};

typedef std::shared_ptr<FireWin> FireWindow;

extern Atom winTypeAtom, winTypeDesktopAtom, winTypeDockAtom,
       winTypeToolbarAtom, winTypeMenuAtom, winTypeUtilAtom,
       winTypeSplashAtom, winTypeDialogAtom, winTypeNormalAtom,
       winTypeDropdownMenuAtom, winTypePopupMenuAtom, winTypeDndAtom,
       winTypeTooltipAtom, winTypeNotificationAtom, winTypeComboAtom;


extern Atom winStateAtom, winStateModalAtom, winStateStickyAtom,
       winStateMaximizedVertAtom, winStateMaximizedHorzAtom,
       winStateShadedAtom, winStateSkipTaskbarAtom,
       winStateSkipPagerAtom, winStateHiddenAtom,
       winStateFullscreenAtom, winStateAboveAtom,
       winStateBelowAtom, winStateDemandsAttentionAtom,
       winStateDisplayModalAtom;


extern Atom activeWinAtom;
extern Atom winActiveAtom;
extern Atom wmTakeFocusAtom;
extern Atom wmProtocolsAtom;
extern Atom wmClientLeaderAtom;
extern Atom wmNameAtom;
extern Atom winOpacityAtom;
extern Atom clientListAtom;

class Core;

namespace WinUtil {
    void init();
    FireWindow getTransient(Window win);
    void       getWindowName(Window win, char *name);
    FireWindow getClientLeader(Window win);

    int         readProp(Window win, Atom prop, int def);
    WindowType  getWindowType (Window win);
    uint        getWindowState(Window win);
    bool        constrainNewWindowPosition(int &x, int &y);
};
