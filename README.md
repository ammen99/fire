# Fire
# Introduction
Fire is a compositing window manager written from scratch. It uses X11 and OpenGL to render windows. It has a simple configuration system and extensible plugin interface. The master branch uses OpenGL4(tested on only NVIDIA binary driver + optimus), but there is also support for OpenGL3.3 in the fedora branch(for systems with opensource drivers).
Note: The Fire window manager should be stable, although I cannot guarantee for that(I can only say that I'm using it on my main system without problems).

# Configuration
Currently configurable options are few. For example see config file.

# Future plans
1. Improve configuration system(for ex. allow configurable KeyBindings)
2. Implement more ICCCM and EWMH functions
3. Port to wayland
