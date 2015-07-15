#include "../include/core.hpp"

/* specialized class for operations on window(for ex. Move and Resize) */
class Move : public Plugin {
    private:
        int sx, sy; // starting pointer x, y
        FireWindow win; // window we're operating on
    private:
        ButtonBinding press;
        ButtonBinding release;
        Hook hook;

        void Initiate(Context*);
        void Intermediate();
        void Terminate(Context*);

    public:
        void init();
        void initOwnership();
        void updateConfiguration();
};
