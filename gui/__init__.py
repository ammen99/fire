#! /usr/bin/env python2
import gui
import gtk
import x

configFile = "/home/ilex/work/fire/gui/test_config"
win = x.EventCatcher(x.button_mask)
gtk.main()

win = gui.ConfigGUI()
win.create_gui_from_file(configFile)
gtk.main()
