#include "animate.hpp"

extern int fadeDuration;
enum FadeMode { FadeIn = 1, FadeOut = -1 };
template<FadeMode mode> class Fade : public Animation {
    FireWindow win;
    int progress = 0;
    int maxstep = 0;
    int target = 0;
    bool run = true;
    bool restoretr = true;
    bool savetr;

    public:
    Fade(FireWindow w);
    bool Step();
    bool Run();
    ~Fade();
};


