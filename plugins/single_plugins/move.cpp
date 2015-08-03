#include <core.hpp>

class Move : public Plugin {
        int sx, sy; // starting pointer x, y
        FireWindow win; // window we're operating on
        ButtonBinding press;
        ButtonBinding release;
        Hook hook;

        SignalListener sigScl;

        int scX = 1, scY = 1;

        Button iniButton;

    public:
        void initOwnership() {
            owner->name = "move";
            owner->compatAll = true;
        }
        void updateConfiguration() {

            iniButton = *options["activate"]->data.but;
            if(iniButton.button == 0)
                return;

            hook.action = std::bind(std::mem_fn(&Move::Intermediate), this);
            core->addHook(&hook);

            using namespace std::placeholders;
            press.type   = BindingTypePress;
            press.mod    = iniButton.mod;
            press.button = iniButton.button;
            press.action = std::bind(std::mem_fn(&Move::Initiate), this, _1);
            core->addBut(&press, true);

            release.type   = BindingTypeRelease;
            release.mod    = AnyModifier;
            release.button = iniButton.button;
            release.action = std::bind(std::mem_fn(&Move::Terminate), this, _1);
            core->addBut(&release, false);
        }

        void init() {
            using namespace std::placeholders;
            options.insert(newButtonOption("activate", Button{0, 0}));
            sigScl.action = std::bind(std::mem_fn(&Move::onScaleChanged), this, _1);
            core->connectSignal("screen-scale-changed", &sigScl);
        }

        void Initiate(Context *ctx) {
            auto xev = ctx->xev.xbutton;
            auto w = core->getWindowAtPoint(xev.x_root, xev.y_root);

            if(!w)
                return;
            win = w;

            if(!core->activateOwner(owner)){
                return;
            }

            owner->grab();

            core->focusWindow(w);
            hook.enable();
            release.enable();

            this->sx = xev.x_root;
            this->sy = xev.y_root;
            core->setRedrawEverything(true);
        }

        void Terminate(Context *ctx) {
            hook.disable();
            release.disable();
            core->deactivateOwner(owner);

            auto xev = ctx->xev.xbutton;
            win->transform.translation = glm::mat4();

            int dx = (xev.x_root - sx) * scX;
            int dy = (xev.y_root - sy) * scY;

            int nx = win->attrib.x + dx;
            int ny = win->attrib.y + dy;

            win->move(nx, ny);
            core->setRedrawEverything(false);

            core->focusWindow(win);
            win->addDamage();
        }

        void Intermediate() {
            GetTuple(cmx, cmy, core->getMouseCoord());
            GetTuple(w, h, core->getScreenSize());

            win->transform.translation =
                glm::translate(glm::mat4(), glm::vec3(
                            float(cmx - sx) / float(w / 2.0),
                            float(sy - cmy) / float(h / 2.0),
                            0.f));
        }

        void onScaleChanged(SignalListenerData data) {
            scX = *(int*)data[0];
            scY = *(int*)data[1];
        }
};

extern "C" {
    Plugin *newInstance() {
        return new Move();
    }
}

