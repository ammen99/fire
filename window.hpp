#include "commonincludes.hpp"


enum WindowType {
    WindowTypeNormal,
    WindowTypeWidget,
    WindowTypeModal,
    WindowTypeDock,
    WindowTypeDesktop,
    WindowTypeOther
};

enum StackType{
    StackTypeSibling    = 1,
    StackTypeAncestor   = 2,
    StackTypeChild      = 4,
    StackTypeNoStacking = 8
};

class Transform {
    public:
        float stackID; // used to move the window forward if front
        glm::mat4 rotation;
        glm::mat4 scalation;
        glm::mat4 translation;
    public:
        Transform();
        glm::mat4 compose();
};

class __FireWindow {
    public:
        XVisualInfo *xvi;
        Pixmap pixmap;
        Window id;
        GLuint texture; // image on screen

        bool norender = false; // should we draw window?
        bool paintTransformed;
        Transform transform;

        GLuint vbo = -1;
        GLuint vao = -1;

        Damage damage;

        std::shared_ptr<__FireWindow> transientFor; // transientFor
        std::shared_ptr<__FireWindow> leader;

        char *name;
        WindowType type;
        XWindowAttributes attrib;
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


class WindowWorker {
    private:
        static bool recurseIsAncestor(FireWindow parent, FireWindow win);
    public:
        static void init();

        static void renderWindow(FireWindow win);
        static int setWindowTexture(FireWindow win);
        static FireWindow createWindow(int x, int y, int w, int h);
        static void initWindow(FireWindow win);
        static void finishWindow(FireWindow win);

        static void setInputFocusToWindow(Window win);

        static void moveWindow(FireWindow win, int x, int y);
        static void resizeWindow(FireWindow win, int w, int h);
        static void syncWindowAttrib(FireWindow win);

        static XVisualInfo *getVisualInfoForWindow(Window win);
        static FireWindow getTransient(FireWindow win);
        static FireWindow getClientLeader(FireWindow win);
        static void getWindowName(FireWindow win, char *name);
        static bool isTopLevelWindow(FireWindow win);
        static FireWindow getAncestor(FireWindow win);
        static WindowType getWindowType(FireWindow win);

        static bool isAncestorTo(FireWindow parent, FireWindow win);
        static StackType getStackType(FireWindow win1, FireWindow win2);
};
