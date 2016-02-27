#include <core.hpp>

class Move : public Plugin {

        int sx, sy; // starting pointer x, y

        FireWindow win; // window we're operating on

        ButtonBinding press;
        ButtonBinding release;
        //Hook hook;

        SignalListener sigScl, move_request;

        int scX = 1, scY = 1;

        Hook hook;

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
            core->add_hook(&hook);

            using namespace std::placeholders;
            press.type   = BindingTypePress;
            press.mod    = iniButton.mod;
            press.button = iniButton.button;
            press.action = std::bind(std::mem_fn(&Move::Initiate), this, _1, nullptr);
            core->add_but(&press, true);

            release.type   = BindingTypeRelease;
            release.mod    = 0;
            release.button = iniButton.button;
            release.action = std::bind(std::mem_fn(&Move::Terminate), this, _1);
            core->add_but(&release, false);
        }

        void init() {
            printf("move init\n");

            using namespace std::placeholders;
            options.insert(newButtonOption("activate", Button{0, 0}));
            sigScl.action = std::bind(std::mem_fn(&Move::onScaleChanged), this, _1);
            core->connectSignal("screen-scale-changed", &sigScl);

            move_request.action = std::bind(std::mem_fn(&Move::on_move_request), this, _1);
            core->connectSignal("move-request", &move_request);
        }

        void Initiate(Context ctx, FireWindow pwin) {

            auto xev = ctx.xev.xbutton;
            if(!pwin) {
                auto w = core->getWindowAtPoint(xev.x_root, xev.y_root);

                if(!w) return;
                else win = w;
            }

            else win = pwin;

            if(!core->activateOwner(owner)){
                return;
            }

            owner->grab();

            core->focus_window(win);

            hook.enable();
            release.enable();

            this->sx = xev.x_root;
            this->sy = xev.y_root;
        }

        void Terminate(Context ctx) {
            hook.disable();
            release.disable();
            core->deactivateOwner(owner);

            auto xev = ctx.xev.xbutton;
            win->transform.translation = glm::mat4();

            int dx = (xev.x_root - sx) * scX;
            int dy = (xev.y_root - sy) * scY;

            int nx = win->attrib.origin.x + dx;
            int ny = win->attrib.origin.y + dy;

            win->move(nx, ny);
        }

        void Intermediate() {
            GetTuple(cmx, cmy, core->getMouseCoord());
            GetTuple(w, h, core->getScreenSize());

            printf("%d %d\n", cmx - sx, sy - cmy);

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

        void on_move_request(SignalListenerData data) {
            FireWindow w = *(FireWindow*)data[0];
            if(!w) return;

            wlc_point origin = *(wlc_point*)data[1];

            Initiate(Context(origin.x, origin.y, 0, 0), w);
        }
};

extern "C" {
    Plugin *newInstance() {
        return new Move();
    }
}

