#ifndef PLUGIN_H
#define PLUGIN_H

class Plugin {
    public:
        virtual void init(Core*) = 0;
        virtual void fini(Core*) = 0;
};



#endif
