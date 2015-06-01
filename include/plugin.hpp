#ifndef PLUGIN_H
#define PLUGIN_H

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
class Plugin {
    public:
        virtual void init(Core*) = 0;
};

#endif
