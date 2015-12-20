# Fire
# Introduction
Fire is a compositing window manager written from scratch. It uses X11 and OpenGL to render windows. It has a simple configuration system and extensible plugin interface. Right now I'm using Fire as my WM on my main system, but I cannot guarantee for 100% stability. 

# Dependencies
Fire depends on Xlib, OpenGL(at least 3.3, 4.0 needed for cube deformation) and on ILUT(DevIL-ILUT) for loading of background image.

# Installation
You need the following libraries(for Fedora):

    sudo dnf install libX11-devel libXext-devel DevIL-devel libGLEW libXdamage-devel libXfixes-devel libXcomposite-devel libGL-devel libXmu-devel glm-devel dmenu

Then just clone the repo and run install.sh:

    git clone https://github.com/ammen99/fire
    cd fire
    ./install.sh


# Configuration
Configuration is done through a *simple* configuration file. The default location is ~/.config/firerc, but can be optionally set by the -c option.
It is recommended to edit the file using the fcm utility. Nevertheless, for people who want to edit the file by hand:
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
