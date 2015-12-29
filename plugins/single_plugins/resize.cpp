#include <wm.hpp>

class Resize : public Plugin {
    private:
        int sx, sy; // starting pointer x, y
        int cx, cy; // coordinates of the center of the window
        FireWindow win; // window we're operating on

        int scX = 1, scY = 1;
        SignalListener sigScl;
        Button iniButton;

        uint32_t edges;

    private:
        ButtonBinding press;
        ButtonBinding release;
        Hook hook;
    public:

    void initOwnership() {
        owner->name = "resize";
        owner->compatAll = true;
    }
    void updateConfiguration(){
        iniButton = *options["activate"]->data.but;
        if(iniButton.button == 0)
            return;

        using namespace std::placeholders;
        hook.action = std::bind(std::mem_fn(&Resize::Intermediate), this);
        core->add_hook(&hook);
        press.type   = BindingTypePress;
        press.mod    = iniButton.mod;
        press.button = iniButton.button;
        press.action = std::bind(std::mem_fn(&Resize::Initiate), this, _1);
        core->add_but(&press, true);


        release.type   = BindingTypeRelease;
        release.mod    = AnyModifier;
        release.button = iniButton.button;
        release.action = std::bind(std::mem_fn(&Resize::Terminate), this,_1);
        core->add_but(&release, false);
    }

    void init() {
        options.insert(newButtonOption("activate", Button{0, 0}));
        using namespace std::placeholders;
        sigScl.action = std::bind(std::mem_fn(&Resize::onScaleChanged), this, _1);
        core->connectSignal("screen-scale-changed", &sigScl);
    }

    void Initiate(Context ctx) {
        auto xev = ctx.xev.xbutton;
        auto w = core->getWindowAtPoint(xev.x_root,xev.y_root);

        if(!w)
            return;

        if(!core->activateOwner(owner)) {
            return;
        }

        owner->grab();

        core->focus_window(w);

        win = w;
        hook.enable();
        release.enable();

        sx = xev.x_root;
        sy = xev.y_root;

        const int32_t halfw = win->attrib.origin.x + win->attrib.size.w / 2;
        const int32_t halfh = win->attrib.origin.y + win->attrib.size.h / 2;

        edges = (sx < halfw ? WLC_RESIZE_EDGE_LEFT : (sx >= halfw ? WLC_RESIZE_EDGE_RIGHT : 0)) |
            (sy < halfh ? WLC_RESIZE_EDGE_TOP : (sy >= halfh ? WLC_RESIZE_EDGE_BOTTOM : 0));

        if(!edges) Terminate(ctx);
    }

    void Terminate(Context ctx) {
        hook.disable();
        release.disable();
        core->deactivateOwner(owner);
    }

    void Intermediate() {

        GetTuple(cmx, cmy, core->getMouseCoord());

        const int32_t dx = cmx - sx;
        const int32_t dy = cmy - sy;

        if (edges) {
            const struct wlc_size min = { 10, 10 };

            struct wlc_geometry n = win->attrib;

            if (edges & WLC_RESIZE_EDGE_LEFT) n.size.w -= dx, n.origin.x += dx;
            else if (edges & WLC_RESIZE_EDGE_RIGHT) n.size.w += dx;

            if (edges & WLC_RESIZE_EDGE_TOP) n.size.h -= dy, n.origin.y += dy;
            else if (edges & WLC_RESIZE_EDGE_BOTTOM) n.size.h += dy;

            if (n.size.w < min.w) {
                n.size.w = min.w;
                n.origin.x = win->attrib.origin.x;
            }

            if (n.size.h < min.h) {
                n.origin.y = win->attrib.origin.y;
                n.size.h = min.h;
            }

            win->moveResize(n.origin.x, n.origin.y, n.size.w, n.size.h);

        }
    }

    void onScaleChanged(SignalListenerData data) {
        scX = *(int*)data[0];
        scY = *(int*)data[1];
    }
};

extern "C" {
    Plugin *newInstance() {
        return new Resize();
    }
}
