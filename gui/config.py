#! /bin/sh

# File which parses config


options = {}

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

def parse_file(file):
    handle = open(file, "r")
    plugin = "Unknown"

    line = handle.readline().strip()

    while line != "":
        lst = line.split(" ", 2)

        if not lst:
            pass

        elif lst[0].startswith("["):    # a section for plugin
            plugin = lst[0].strip("[] \t\n")
            add_plugin(plugin)

        elif len(lst) < 3 or lst[1] != '=': # garbage
            pass

        elif plugin != "Unknown": # option for plugin, ignore dummy ones
            options[plugin][lst[0]] = lst[2][:-1]

        line = handle.readline()

    handle.close()

def write_config(file):
    handle = open(file, "w")
    handle.truncate()

    for x in sorted(options):
        handle.write("[" + x + "]\n")
        for y in sorted(options[x]):
            handle.write(y + " = " + options[x][y] + "\n")

        handle.write("\n")

    handle.close()


def print_config():
    for x in sorted(options):
        for y in sorted(options[x]):
            print options[x][y]

