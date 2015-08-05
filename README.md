# Fire
# Introduction
Fire is a compositing window manager written from scratch. It uses X11 and OpenGL to render windows. It has a simple configuration system and extensible plugin interface. Right now I'm using Fire as my WM on my main system, but I cannot guarantee for 100% stability. 

# Dependencies
Fire depends on Xlib, OpenGL(at least 3.3, 4.0 needed for cube deformation) and on ILUT(DevIL-ILUT) for loading of background image.

# Configuration
Configuration is done through a *simple* configuration file. The default location is ~/.config/firerc, but can be optionally set by the -c option.
It is recommended to edit the file using the fsm utility. Nevertheless, for people who want to edit the file by hand:
Each plugin has a section beginning like this:

    ['plugin name']

Then, options follow in the following way:

    type_name = value

where type is one of the following: **pln**(short for plain, used for ints, floats, strings), **key**(keybinding), **but**(buttonbinding) or **clr**(color)
See example configuration file config for more details

# Future plans
1. Add more plugins
2. Implement more ICCCM and EWMH functions
3. Port to wayland
