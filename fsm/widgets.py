import time

import gtk
from gtk import gdk

import config

mod_table = {
        gtk.gdk.SHIFT_MASK:"<Shift>",
        gtk.gdk.CONTROL_MASK:"<Control>",
        gtk.gdk.MOD1_MASK:"<Alt>",
        gtk.gdk.MOD4_MASK:"<Win>"
}

def get_mods_keys_from_string(value):
    i = len(value) - 1
    while i >= 0 and value[i] != '>':
        i = i - 1

    return (value[:i], value[i+1:])

def get_mods_buts_from_string(value):
    return get_mods_keys_from_string(value)

class KeyGrab(gtk.MessageDialog):

    def __init__(self, win):
        super(KeyGrab, self).__init__(parent=win, type=gtk.MESSAGE_OTHER,
                buttons=gtk.BUTTONS_NONE);

        self.label1 = gtk.Label("Enter keybinding")
        self.key = ""
        self.mod = ""
        self.parent_win = win

        self.get_content_area().add(self.label1)
        self.set_uposition(500, 300)
        self.set_size_request(300, 100)
        self.handler = self.connect("key-press-event", self.on_key_press)

    def begin(self):
        while gtk.gdk.keyboard_grab(self.parent_win.window) != gtk.gdk.GRAB_SUCCESS:
                time.sleep(0.1)
        self.show_all()
        self.run()

    def end_grab(self):
        gtk.gdk.keyboard_ungrab(gtk.get_current_event_time())
        self.disconnect(self.handler)
        self.destroy()

    def on_key_press(self, widget, event):
        mods = event.state & gtk.accelerator_get_default_mod_mask()
        key = gtk.gdk.keyval_to_lower(event.keyval)

        if not mods and event.keyval == gtk.keysyms.Escape:
            self.mods = ""
            self.key = ""
            self.end_grab()
            return

        if not mods and event.keyval == gtk.keysyms.Return:
            self.end_grab()
            return

        if gtk.accelerator_valid(key, mods):
            self.update_label(key, mods)

    def update_label(self, key, mods):
        text = ""
        for mod in mod_table:
            if mods & mod:
                text = text + mod_table[mod]

        self.mod = text
        self.key = gtk.accelerator_name(key, 0)
        self.label1.set_text(self.mod + self.key)


class ButtonSelect(gtk.MessageDialog):

    def __init__(self, win):
        super(ButtonSelect, self).__init__(parent=win,
                type=gtk.MESSAGE_OTHER, buttons=gtk.BUTTONS_OK_CANCEL);

        self.label1 = gtk.Label("Choose ButtonBinding")
        self.button = ""
        self.mod = ""

        content = self.get_content_area()
        self.checkboxes = gtk.HButtonBox()

        self.mods = [gtk.CheckButton("Control"),
                     gtk.CheckButton("Shift"),
                     gtk.CheckButton("Alt"),
                     gtk.CheckButton("Win")]

        self.mods_toggled = {"<Control>":0, "<Shift>":0, "<Alt>":0, "<Win>":0}

        self.radios = gtk.VButtonBox()

        self.b1 = gtk.RadioButton(None   , "Left Button")
        self.b2 = gtk.RadioButton(self.b1, "Middle Button")
        self.b3 = gtk.RadioButton(self.b1, "Right Button")
        self.b4 = gtk.RadioButton(self.b1, "Scroll Up")
        self.b5 = gtk.RadioButton(self.b1, "Scroll Down")

        self.b1.set_active(True)

        content.add(self.label1)

        for w in self.mods:
            self.checkboxes.pack_end(w)
        content.add(self.checkboxes)

        self.radios.pack_end(self.b1)
        self.radios.pack_end(self.b3)
        self.radios.pack_end(self.b2)
        self.radios.pack_end(self.b4)
        self.radios.pack_end(self.b5)

        content.add(self.radios)

        self.set_uposition(450, 100)
        self.set_size_request(500, 300)

        self.mods[0].connect("toggled", self.on_mods_changed, "<Control>")
        self.mods[1].connect("toggled", self.on_mods_changed, "<Shift>")
        self.mods[2].connect("toggled", self.on_mods_changed, "<Alt>")
        self.mods[3].connect("toggled", self.on_mods_changed, "<Win>")

        self.b1.connect("toggled", self.on_toggle, "B1")
        self.b2.connect("toggled", self.on_toggle, "B2")
        self.b3.connect("toggled", self.on_toggle, "B3")
        self.b4.connect("toggled", self.on_toggle, "B4")
        self.b5.connect("toggled", self.on_toggle, "B5")

    def begin(self):
        self.show_all()
        resp = self.run()
        self.destroy()
        if resp != gtk.RESPONSE_OK:
            self.button = ""
            self.mod    = ""

    def on_toggle(self, widget, data=None):
        if data:
            self.button = data

    def on_mods_changed(self, widget, data=None):
        if data:
            self.mods_toggled[data] = 1 - self.mods_toggled[data]

        self.reconstruct_mod_string()

    def reconstruct_mod_string(self):
        self.mod = ""
        for mod in self.mods_toggled:
            if self.mods_toggled[mod] == 1:
                self.mod = self.mod + mod;

class InputDialogOneField(gtk.MessageDialog):

    def __init__(self, label, win=None):
        super(InputDialogOneField, self).__init__(parent=win,
                type=gtk.MESSAGE_OTHER, buttons=gtk.BUTTONS_OK_CANCEL)

        self.label1 = gtk.Label(label)
        self.entry1 = gtk.Entry()
        self.value = ""

        self.get_content_area().add(self.label1)
        self.get_content_area().add(self.entry1)

    def get_input(self):
        self.show_all()
        resp = self.run()
        self.value = self.entry1.get_text()
        self.destroy()
        return resp


class InputDialogOption(gtk.MessageDialog):
    def __init__(self, label1, label2, win):
        super(InputDialogOption, self).__init__(parent=win,
                type=gtk.MESSAGE_OTHER, buttons=gtk.BUTTONS_OK_CANCEL)

        self.parent_win = win
        self.label1 = gtk.Label(label1)
        self.label2 = gtk.Label(label1)

        self.entry1 = gtk.Entry()
        self.entry2 = gtk.Entry()

        self.value1 = ""
        self.value2 = ""

        self.b1 = gtk.RadioButton(None   , "Plain Text")
        self.b2 = gtk.RadioButton(self.b1, "Key Binding")
        self.b3 = gtk.RadioButton(self.b1, "Button Binding")
        self.b4 = gtk.RadioButton(self.b1, "Color")

        self.value_type = config.OptionTypePlain
        self.radio_box = gtk.VButtonBox()

        content = self.get_content_area()

        content.add(self.label1)
        content.add(self.entry1)
        content.add(self.label2)
        content.add(self.entry2)

        self.b1.set_active(True)
        self.b1.connect("toggled", self.on_select, config.OptionTypePlain)
        self.b2.connect("toggled", self.on_select, config.OptionTypeKey)
        self.b3.connect("toggled", self.on_select, config.OptionTypeButton)
        self.b4.connect("toggled", self.on_select, config.OptionTypeColor)

        self.radio_box.add(self.b1)
        self.radio_box.add(self.b2)
        self.radio_box.add(self.b3)
        self.radio_box.add(self.b4)

        content.add(self.radio_box)

    def get_inputs(self):
        self.show_all()
        resp = self.run()
        self.value1 = self.entry1.get_text()
        self.value2 = self.entry2.get_text()

        self.destroy()
        return resp

    def on_select(self, widget, data):
        self.value_type = data

        if data == config.OptionTypeKey:
            keygrab = KeyGrab(self.parent_win)
            keygrab.begin()
            if keygrab.mod and keygrab.key:
                self.value2 = keygrab.mod + keygrab.key
                self.entry2.set_text(self.value2)

        if data == config.OptionTypeButton:
            bb = ButtonSelect(self.parent_win)
            bb.begin()
            if bb.mod and bb.button:
                self.value2 = bb.mod + bb.button
                self.entry2.set_text(self.value2)

