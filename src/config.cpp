#include "../include/config.hpp"

#define copyInto(x,y) std::memcpy(&(y), &x, sizeof((y)))

namespace {
    /* removes whitespace from beginning and from end */
    std::string trim(std::string line) {
        int i = 0, j = line.length() - 1;
        for(; line[i] == ' ' && i < line.length(); i++);
        for(; line[j] == ' ' && j > i; j--);
        if(i >= j)
            return "";

        return line.substr(i, j - i + 1);
    }

    /* self-explanatory */
    void setDefaultOptions(PluginPtr p) {
        for(auto o : p->options)
            copyInto(o.second->def, o.second->data);
    }

    /* used for parsing of different values */
    std::stringstream theMachine;

    template<class T> T readValue(std::string val) {
        theMachine.str("");
        theMachine.clear();
        theMachine.str("val");

        T tmp;
        theMachine >> tmp;
        return tmp;
    }

}

Config::Config(std::string path) {
    stream.open(path, std::ios::in | std::ios::out);
    readConfig();
}

void Config::readConfig() {
    std::string line;
    std::string currentPluginName = "";
    std::string option, value;

    while(std::getline(stream, line)) {
        line = trim(line);
        if(!line.length())
            continue;

        if(line[0] == '[') {
            currentPluginName = line.substr(1, line.length() - 2);
            continue;
        }

        auto pos = line.find("=");
        if(pos == std::string::npos) {
            std::cout << "Warning - Garbage in config" << std::endl;
            continue;
        }

        option = trim(line.substr(0, pos));
        value  = trim(line.substr(pos + 1, line.length() - pos));
        tree[currentPluginName][option] = value;
    }
}

void Config::reset() {
    stream.close();
    tree.clear();
}

void Config::setOptionsForPlugin(PluginPtr p) {
    auto name = p->owner->name;
    auto it = tree.find(name);
    if(it == tree.end()) {
        setDefaultOptions(p);
        return;
    }

    for(auto option : p->options) {
        auto it = tree[name].find(option.first);
        if(it == tree[name].end()) {
            p->options[option.first]->data = p->options[option.first]->def;
        }
    }
}

