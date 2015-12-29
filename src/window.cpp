#include <core.hpp>
#include <opengl.hpp>


/* misc definitions */

glm::mat4 Transform::grot;
glm::mat4 Transform::gscl;
glm::mat4 Transform::gtrs;

Transform::Transform() {
    this->translation = glm::mat4();
    this->rotation = glm::mat4();
    this->scalation = glm::mat4();
}

glm::mat4 Transform::compose() {
    return (gtrs*translation*viewport_translation)*(grot*rotation)*(gscl*scalation);
}

bool FireWin::allDamaged = false;


FireWin::FireWin(wlc_handle _view) {
    view = _view;

    auto geom = wlc_view_get_geometry(view);

    std::memcpy(&attrib, geom, sizeof(attrib));
}

FireWin::~FireWin() {

    for(auto d : data)
        delete d.second;
    data.clear();
}

#define Mod(x,m) (((x)%(m)+(m))%(m))



bool FireWin::isVisible() {
    if(destroyed && !keepCount)
        return false;

    if(!visible && !allDamaged)
        return false;

    if(norender)
        return false;

    return true;
}

bool FireWin::shouldBeDrawn() {
    if(!isVisible())
        return false;

    return true;
}

void FireWin::move(int x, int y) {
    attrib.origin.x = x;
    attrib.origin.y = y;

    wlc_view_set_geometry(view, 0, &attrib);
}

void FireWin::resize(int w, int h) {
    attrib.size.w = w;
    attrib.size.h = h;

    wlc_view_set_geometry(view, 0, &attrib);
}

void FireWin::moveResize(int x, int y, int w, int h) {

    attrib.origin.x = x;
    attrib.origin.y = y;
    attrib.size.w = w;
    attrib.size.h = h;

    wlc_view_set_geometry(view, 0, &attrib);
}


namespace WinUtil {

    /* ensure that new window is on current viewport */
    bool constrainNewWindowPosition(int &x, int &y) {
        GetTuple(sw, sh, core->getScreenSize());
        auto nx = (x % sw + sw) % sw;
        auto ny = (y % sh + sh) % sh;

        if(nx != x || ny != y){
            x = nx, y = ny;
            return true;
        }
        return false;
    }

}
