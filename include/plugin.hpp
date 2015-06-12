#ifndef PLUGIN_H
#define PLUGIN_H
#include "commonincludes.hpp"

/*
 * Documentation for writing a plugin
 *
 * Plugins are just objects which are created during init of
 * Core and destroyed when core is destroyed
 *
 * Plugins aren't run at any time(i.e between redraws)
 * They must have at least the methon init()
 * which is the method when they register on the Core()
 * their bindings and hooks
 *
 * They should not atempt to access the global core variable
 * in their constructors since during that time that global
 * variable is still uninitialized
 *
 * A typical example of a plugin is when
 * in the init() function it registers a hook(disabled)
 * and then a key/button binding to activate it
 *
 * For example see viewexpo.cpp or movres.cpp
 */

class Core;

using Owner = std::string;
/* owners are used to acquire screen grab and to add bindings */
struct _Ownership {
    Owner name;
    /* list of plugins which we are compatible with */
    std::unordered_set<Owner> compat;
    bool active;
    // if we are compatible with all plugins
    bool compatAll = false;

    // call these functions to (un)grab keyboard and mouse
    void grab();
    void ungrab();
    bool grabbed;
};

using Ownership = std::shared_ptr<_Ownership>;


class Plugin {
    public:

        /* initOwnership() should set all values in own */
        virtual void initOwnership();
        virtual void init(Core*) = 0;
        Ownership owner;
};

using PluginPtr = std::shared_ptr<Plugin>;

#endif
