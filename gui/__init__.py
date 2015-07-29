#! /usr/bin/env python2
import gui
import gtk

configFile = "/static/config"

win = gui.ConfigGUI()
win.create_gui_from_file(configFile)
gtk.main()
