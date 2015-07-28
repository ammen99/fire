#! /usr/bin/env python2

import config
import pygtk
import gtk
import sys


class Option:
    def __init__(self, name, value):
        self.box = gtk.HBox()

        self.label = gtk.Label(name)
        self.label.set_justify(gtk.JUSTIFY_LEFT)
        self.label.set_alignment(0, 0.5)

        self.entry = gtk.Entry()
        self.entry.set_text(value)

        self.box.pack_start(self.label)
        self.box.pack_start(self.entry)
        self.box.set_homogeneous(1)
        self.box.set_spacing(50)
        self.box.show_all()

class Tab:
    def __init__(self, widget, text):
        self.widget = widget
        self.widget.set_homogeneous(1)
        self.label = gtk.Label()
        self.label.set_text(text)
        self.label.set_justify(gtk.JUSTIFY_LEFT)
        self.text = text
        self.opts = []

class ConfigGUI:

    def create_gui_from_file(self, srcfile):
        config.parse_file(srcfile)
        self.path = srcfile

        self.window = gtk.Window()
        self.vbox = gtk.VBox()
        self.window.add(self.vbox)

        self.bbox = gtk.HBox() # used for create buttons
        self.bbox.set_homogeneous(True)
        self.bbox.set_spacing(30)
        self.vbox.pack_end(self.bbox)

        self.notebook = gtk.Notebook()
        self.vbox.pack_end(self.notebook)

        self.newButton    = gtk.Button("New Plugin")
        self.newButton.connect("clicked", self.on_new_plugin)
        self.newOptButton = gtk.Button("New Option")
        self.newOptButton.connect("clicked", self.on_add_option)
        self.deleteButton = gtk.Button("Delete Plugin")
        self.deleteButton.connect("clicked", self.on_remove_plugin)
        self.saveButton   = gtk.Button("Save to file")
        self.saveButton.connect("clicked", self.on_save)
        self.exitButton   = gtk.Button("Exit")
        self.exitButton.connect("clicked", self.on_button_exit)

        self.bbox.pack_start(self.newButton)
        self.bbox.pack_start(self.newOptButton)
        self.bbox.pack_start(self.deleteButton)
        self.bbox.pack_start(self.saveButton)
        self.bbox.pack_start(self.exitButton)

        self.window.connect("delete-event", self.on_exit)
        self.window.connect("destroy", self.on_exit)

        self.window.set_size_request(1000, 400)
        self.window.show_all()
        self.init_tabs()
        gtk.main()

    def init_tabs(self):
        self.tabs = {}
        for plugin in config.options:
            self.tabs[plugin] = Tab(gtk.VBox(), plugin)
            self.notebook.append_page(self.tabs[plugin].widget,
                    self.tabs[plugin].label)

            for option in config.options[plugin]:
                opt = Option(option,
                    config.options[plugin][option])
                self.tabs[plugin].opts.append(opt)
                self.tabs[plugin].widget.pack_start(opt.box)

        self.notebook.show_all()

    def on_save(self, widget):
        for tab in self.tabs:
            plugin = self.tabs[tab].text
            for opt in self.tabs[tab].opts:
                name = opt.label.get_text()
                value = opt.entry.get_text()
                config.options[plugin][name] = value

        config.write_config(self.path)

    def on_exit(self, unused, unused2):
        self.on_save(unused)
        sys.exit(0)

    def on_button_exit(self, widget):
        self.on_exit(0, 0)

    def on_new_plugin(self, widget):
        message = gtk.MessageDialog(parent=self.window,
                type=gtk.MESSAGE_OTHER, buttons=gtk.BUTTONS_OK_CANCEL)

        message.set_modal(True)

        label = gtk.Label("enter plugin name")
        content = message.get_content_area()
        content.pack_start(label)

        entry = gtk.Entry()
        content.pack_start(entry)
        content.show_all()

        response = message.run()

        if response == gtk.RESPONSE_OK:
            plugin = entry.get_text()
            self.tabs[plugin] = Tab(gtk.VBox(), plugin)
            self.notebook.append_page(self.tabs[plugin].widget,
                    self.tabs[plugin].label)
            self.notebook.show_all()

            config.add_plugin(plugin)

        message.destroy()

    def on_remove_plugin(self, widget):
        tabno = self.notebook.get_current_page()

        plugin = self.notebook.get_tab_label_text(
                 self.notebook.get_nth_page(tabno))

        self.notebook.remove_page(tabno)
        config.remove_plugin(plugin)
        self.notebook.show_all()

    def on_add_option(self, widget):
        message = gtk.MessageDialog(parent=self.window,
                type=gtk.MESSAGE_OTHER, buttons=gtk.BUTTONS_OK_CANCEL)

        message.set_modal(True)

        olabel = gtk.Label("enter option name")
        content = message.get_content_area()
        content.pack_start(olabel)

        oentry = gtk.Entry()
        content.pack_start(oentry)

        vlabel = gtk.Label("enter option value")
        content.pack_start(vlabel)

        ventry = gtk.Entry()
        content.pack_start(ventry)
        content.show_all()

        response = message.run()

        if response == gtk.RESPONSE_OK:
            name = oentry.get_text()
            value = ventry.get_text()

            if name and value:
                tabno = self.notebook.get_current_page()

                plugin = self.notebook.get_tab_label_text(
                    self.notebook.get_nth_page(tabno))

                box = Option(name, value)
                self.tabs[plugin].opts.append(box)
                self.tabs[plugin].widget.pack_start(box.box)
                self.tabs[plugin].widget.show_all()

                config.add_option(plugin, name, value)
                self.notebook.show_all()

        message.destroy()

