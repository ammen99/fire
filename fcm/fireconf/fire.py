import config
import gtk
import widgets

main_window = None
config_path = ""

# constant to scale gtk.gdk.Color.{red, green, blue} to the range 0-255
color_scaling_factor = 257

def get_rgb_from_string(value):
    list = value.split(" ")
    if len(list) != 3:
        print "Something is wrong with color parsing, returning black"
        return (0, 0, 0)

    return (int(list[0]) * color_scaling_factor, \
            int(list[1]) * color_scaling_factor, \
            int(list[2]) * color_scaling_factor)

class OptionBox(gtk.HBox):

    def __init__(self, name, value, type, plugin):
        super(OptionBox, self).__init__()
        global main_window

        self.label = gtk.Label(name)
        self.value = gtk.Entry()

        self.popup = None

        self.type = type
        self.m_name = name
        self.plugin = plugin

        if type == config.OptionTypePlain:
            self.value.set_text(value)
        else:
            if type == config.OptionTypeColor:
                (r, g, b) = get_rgb_from_string(value)
                self.value = gtk.ColorButton(gtk.gdk.Color(r, g, b))
                self.value.set_use_alpha(False)
                self.value.connect("color-set", self.on_new_color)

            else:
                self.value = gtk.Button(value)
                self.value.connect("clicked", self.on_clicked)

                if type == config.OptionTypeKey:
                    self.popup = widgets.KeyGrab(main_window)

                if type == config.OptionTypeButton:
                    self.popup = widgets.ButtonSelect(main_window)


        self.label.set_justify(gtk.JUSTIFY_FILL)
        self.add(self.label)
        self.add(self.value)
        self.set_homogeneous(True)

        self.show_all()


    def on_clicked(self, widget):
        global main_window
        self.popup.begin()
        text = ""

        if self.type == config.OptionTypeKey:
            if self.popup.mod and self.popup.key:
                text = self.popup.mod + self.popup.key
            self.popup = widgets.KeyGrab(main_window)

        if(self.type == config.OptionTypeButton):
            if self.popup.mod + self.popup.button:
                text = self.popup.mod + self.popup.button
            self.popup = widgets.ButtonSelect(main_window)

        if text:
            self.value.set_label(text)
            config.options[self.plugin][self.m_name] = \
                    config.Option(text, self.type)

    def on_new_color(self, widget):
        new_color = self.value.get_color()
        r = int(new_color.red   / color_scaling_factor)
        g = int(new_color.green / color_scaling_factor)
        b = int(new_color.blue  / color_scaling_factor)

        color_text = str(r) + " " + str(g) + " " + str(b)

        config.options[self.plugin][self.m_name] = \
                config.Option(color_text, self.type)


class Tab(gtk.VBox):

    def __init__(self, name):
        super(Tab, self).__init__()
        self.tab_name = name

        self.options = []

        for opt in config.options[name]:
            box = OptionBox(opt, config.options[name][opt].value,
                config.options[name][opt].type, self.tab_name)
            self.options.append(box)
            self.add(box)

        self.show_all()

    def add_option(self, option, value, type=config.OptionTypePlain):
        config.add_option(self.tab_name, option, config.Option(value, type))
        box = (OptionBox(option, value, type, plugin=self.tab_name))
        self.options.append(box)
        self.add(box)


class MainWindow(gtk.Window):
    note = gtk.Notebook()

    buttonBox = gtk.HButtonBox()
    buttons = [gtk.Button("New Plugin"),
            gtk.Button("New Option"),
            gtk.Button("Delete Plugin"),
            gtk.Button("Save To File"),
            gtk.Button("Exit")]

    vbox = gtk.VBox()

    def __init__(self):
        super(MainWindow, self).__init__()

    def init(self):
        for plugin in config.options:
            self.note.append_page(Tab(plugin), gtk.Label(plugin))

        self.vbox.add(self.note)

        self.buttons[0].connect("clicked", self.on_new_plugin)
        self.buttons[1].connect("clicked", self.on_add_option)
        self.buttons[2].connect("clicked", self.on_remove_plugin)
        self.buttons[3].connect("clicked", self.on_save)
        self.buttons[4].connect("clicked", self.on_button_exit)

        for button in self.buttons:
            self.buttonBox.add(button)

        self.vbox.add(self.buttonBox)

        self.add(self.vbox)
        self.connect("destroy", self.on_exit)

    def run(self):
        self.show_all()
        gtk.main()

    def on_new_plugin(self, widget):
        dialog = widgets.InputDialogOneField("Enter plugin name", self)
        response = dialog.get_input()

        if response == gtk.RESPONSE_OK:
            plugin = dialog.value
            config.add_plugin(plugin)

            self.note.append_page(Tab(plugin), gtk.Label(plugin))
            self.note.show_all()


    def on_remove_plugin(self, widget):
        tabno = self.note.get_current_page()
        tab   = self.note.get_nth_page(tabno)

        self.note.remove_page(tabno)
        config.remove_plugin(tab.tab_name)
        self.note.show_all()

    def on_add_option(self, widget):
        dialog = widgets.InputDialogOption("Enter option name:",
                "Enter option value", self)
        response = dialog.get_inputs()

        if response == gtk.RESPONSE_OK:
            name  = dialog.value1
            value = dialog.value2
            type  = dialog.value_type

            if name and value:
                tabno = self.note.get_current_page()
                tab   = self.note.get_nth_page(tabno)

                tab.add_option(name, value, type)
                self.note.show_all()

    def on_save(self, widget):
        config.write_config(config_path)
        pass

    def on_exit(self, unused):
        self.on_save(unused)
        gtk.main_quit(unused)

    def on_button_exit(self, widget):
        self.on_exit(0)

def run(path):
    global config_path
    global main_window
    config_path = path
    config.parse_file(path)

    main_window = MainWindow()
    main_window.init()
    main_window.run()
