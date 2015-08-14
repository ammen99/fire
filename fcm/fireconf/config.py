#! /bin/env python2

# File which parses config

OptionTypePlain  = 1
OptionTypeColor  = 2
OptionTypeKey    = 3
OptionTypeButton = 4

options = {}

class Option:
    type = OptionTypePlain
    value = ""
    def __init__(self, value, type):
        self.type = type
        self.value = value


def add_plugin(name):
    options[name] = {}

def remove_plugin(name):
    options.pop(name)

def add_option(pluginName, optionName, value):
    if pluginName not in options:
        add_plugin(pluginName)

    options[pluginName][optionName] = value

def remove_option(pluginName, optionName):
    if pluginName in options:
        options[pluginName].pop(optionName)

def get_typecode_from_string(string):
    if string == "pln":
        return OptionTypePlain
    if string == "key":
        return OptionTypeKey
    if string == "but":
        return OptionTypeButton
    if string == "clr":
        return OptionTypeColor

    return OptionTypePlain

def get_type_name_from_typename(string):
    i = 0
    while i < len(string) and string[i] != '_':
        i = i + 1
    type = string[:i]
    name = string[i+1:]
    return (type, name)

def parse_file(file):
    handle = open(file, "r")
    plugin = "Unknown"

    line = handle.readline().strip()

    while line != "":
        lst = line.split(" ", 2)

        if not lst:
            pass

        elif lst[0].startswith("["):        # a section for plugin
            plugin = lst[0].strip("[] \t\n")
            add_plugin(plugin)

        elif len(lst) < 3 or lst[1] != '=': # garbage
            pass

        elif plugin != "Unknown":           # option for plugin
            (typename, name) = get_type_name_from_typename(lst[0])
            type = get_typecode_from_string(typename)
            value = lst[2][:-1]

            add_option(plugin, name, Option(value, type))

        line = handle.readline()

    handle.close()

def get_typename_from_type(type):
    if type == OptionTypePlain:
        return "pln"
    if type == OptionTypeKey:
        return "key"
    if type == OptionTypeButton:
        return "but"
    if type == OptionTypeColor:
        return "clr"

    return "pln"

def write_config(file):
    handle = open(file, "w")
    handle.truncate()

    for x in sorted(options):
        handle.write("[" + x + "]\n")
        for y in sorted(options[x]):
            typename = get_typename_from_type(options[x][y].type)
            handle.write(typename + "_" + y + " = "
                    + options[x][y].value + "\n")

        handle.write("\n")

    handle.close()


def print_config():
    for x in sorted(options):
        print x
        for y in sorted(options[x]):
            print(y + " = " + options[x][y].value +
                    " of type " + str(options[x][y].type))

