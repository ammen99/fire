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
    public:
        Transform();
        glm::mat4 compose();
};

class Point {
    public:
        int x, y;
        Point();
        Point(int, int);
};
class Rect{
    public:
        int tlx, tly;
        int brx, bry;
    public:
        Rect();
        Rect(int, int, int, int);
        bool operator &(const Rect  &other) const;
        bool operator &(const Point &other) const;
};
std::ostream& operator<<(std::ostream& stream, const Rect& rect);

extern Rect output;

class __FireWindow {
    public:
        XVisualInfo *xvi;
        Pixmap pixmap;
        Window id;
        GLuint texture; // image on screen

        bool norender = false; // should we draw window?
        int mapTryNum = 5; // how many times have we tried to map this window?
        Transform transform;

        GLuint vbo = -1;
        GLuint vao = -1;

        Damage damage;

        std::shared_ptr<__FireWindow> transientFor; // transientFor
        std::shared_ptr<__FireWindow> leader;

        char *name;
        WindowType type;
        XWindowAttributes attrib;

        bool shouldBeDrawn();
        void recalcWorkspace();
        void regenVBOFromAttribs();
        Rect getRect();
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


namespace WinUtil {
    bool recurseIsAncestor(FireWindow parent, FireWindow win);
    void init();

    void renderWindow(FireWindow win);
    int setWindowTexture(FireWindow win);
    FireWindow createWindow(int x, int y, int w, int h);
    void initWindow(FireWindow win);
    void finishWindow(FireWindow win);

    void setInputFocusToWindow(Window win);

    void moveWindow(FireWindow win, int x, int y);
    void resizeWindow(FireWindow win, int w, int h);
    void syncWindowAttrib(FireWindow win);

    XVisualInfo *getVisualInfoForWindow(Window win);
    FireWindow getTransient(FireWindow win);
    FireWindow getClientLeader(FireWindow win);
    void getWindowName(FireWindow win, char *name);
    bool isTopLevelWindow(FireWindow win);

    WindowType getWindowType(FireWindow win);

    FireWindow getAncestor(FireWindow win);

    bool isAncestorTo(FireWindow parent, FireWindow win);
    StackType getStackType(FireWindow win1, FireWindow win2);
};

class Core;
class Run {
    public:
        Run(Core *core);
};

class Exit {
    public:
        Exit(Core *core);
};

class Close{
    public:
        Close(Core *core);
};
