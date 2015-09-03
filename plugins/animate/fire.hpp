#ifndef FIRE_H_
#define FIRE_H_

#include "animate.hpp"
#include "particle.hpp"

class FireParticleSystem;

class Fire {
    FireParticleSystem *ps;
    FireWindow w;

    EffectHook hook;
    Hook transparency;
    SignalListener moveListener,
                   unmapListener;

    int progress = 0;

    public:
        Fire(FireWindow win);
        void step();
        void adjustAlpha();
        void handleWindowMoved(SignalListenerData d);
        void handleWindowUnmapped(SignalListenerData d);

        ~Fire();
};

#endif
