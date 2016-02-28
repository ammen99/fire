#include "commonincludes.hpp"

#define SizeStates (WindowStateMaxH|WindowStateMaxV|WindowStateFullscreen)

class Transform {
    public: // applied to all windows
        static glm::mat4 grot;
        static glm::mat4 gscl;
        static glm::mat4 gtrs;

        static glm::mat4 ViewProj;
        static bool has_rotation;
    public:
        glm::mat4 rotation;
        glm::mat4 scalation;
        glm::mat4 translation;
        glm::mat4 translateToCenter;

        glm::vec4 color;
    public:
        Transform();
        glm::mat4 compose();
};


struct ViewData {
};

struct EffectHook;

bool point_inside(wlc_point point, wlc_geometry rect);
bool rect_inside(wlc_geometry screen, wlc_geometry win);

class FireView {
    wlc_handle view;

    public:

        FireView(wlc_handle);
        ~FireView();
        /* this can be used by plugins to store
         * specific for the plugin data */
        std::unordered_map<std::string, ViewData> data;
        std::unordered_map<uint, EffectHook*> effects;

        struct wlc_geometry attrib;

        bool norender = false;       // should we draw window?
        bool destroyed = false;      // is window destroyed?

        int keepCount = 0; // used to determine whether to destroy window
        Transform transform;

        bool is_visible();

        void move(int x, int y);
        void resize(int w, int h);

        void set_geometry(int x, int y, int w, int h);

        wlc_handle get_id() {return view;}
};

typedef std::shared_ptr<FireView> View;

class Core;

namespace WinUtil {
    bool        constrainNewWindowPosition(int &x, int &y);
}
