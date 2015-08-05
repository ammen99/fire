# Fire
# Introduction
Fire is a compositing window manager written from scratch. It uses X11 and OpenGL to render windows. It has a simple configuration system and extensible plugin interface. The master branch uses OpenGL4(tested on only NVIDIA binary driver + optimus), but there is also support for OpenGL3.3 in the fedora branch(for systems with opensource drivers).
Note: The Fire window manager should be stable, although I cannot guarantee for that(I can only say that I'm using it on my main system without problems).

# Dependencies
Fire depends on Xlib, OpenGL and on ILUT(DevIL-ILUT) for loading of background image.

# Configuration
Configuration is done through a *simple* configuration file. Its default location is ~/.config/firerc, but can be optionally set by the -c option.
It is recommended to edit the file using the fsm utility. Nevertheless, for people who want to edit the file by hand:
Each plugin has a section beginning like this:
    ['plugin name']

Then, options follow in the following way:
    type_name = value

where type is one of the following: pln(Plain, used for ints, floats, strings), key(keybinding), but(buttonbinding), clr(color)
See example configuration file config for more details

# Future plans
1. Improve configuration system(for ex. allow configurable KeyBindings)
2. Implement more ICCCM and EWMH functions
3. Port to wayland
