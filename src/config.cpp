#include <config.hpp>
#include <core.hpp>

#define copyInto(x,y) std::memcpy(&(y), &(x), sizeof((y)))

#ifdef log
#undef log
#endif

#define log std::cout<<"[CC] "

namespace {
    /* removes whitespace from beginning and from end */
    std::string trim(std::string line) {
        int i = 0, j = line.length() - 1;
        for(; line[i] == ' ' && i < line.length(); i++);
        for(; line[j] == ' ' && j > i; j--);
        if(i > j)
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
        theMachine.str(val);

        T tmp;
        theMachine >> tmp;
        return tmp;
    }

    uint getModFromStr(std::string value) {
        if(value == "Control")
            return ControlMask;
        if(value == "Alt")
            return Mod1Mask;
        if(value == "Win")
            return Mod4Mask;
        if(value == "Shift")
            return ShiftMask;

        log << "modval not recognized" << std::endl;
        return 0;
    }

    uint getModsFromString(std::string value) {
        std::string cmod;
        uint mods = 0;
        for(auto c : value) {
            if(c == '<')
                cmod = "";
            else if(c == '>')
                mods |= getModFromStr(cmod);
            else
                cmod += c;
        }
        return mods;
    }

    template<> Color  readValue<Color>(std::string value) {
        Color c;
        std::stringstream str;
        str << value;
        str >> c.r >> c.g >> c.b;
        /* values are saved as ints, not floats */
        c.r /= 255.f, c.g /= 255.f, c.b /= 255.f;
        return c;
    }
    template<>Key readValue<Key>(std::string value) {
        Key key;
        key.mod = getModsFromString(value);
        int i = 0;
        for(i = value.length() - 1; i >= 0; i--) {
            if(value[i] == '>') break;
        }
        ++i;
        std::string keystr = value.substr(i, value.size() - i);
        KeySym ks = XStringToKeysym(keystr.c_str());
        key.key = XKeysymToKeycode(core->d, ks);
        return key;
    }

    int Buttons [] = {0, Button1, Button2, Button3, Button4, Button5};

    template<>Button readValue<Button>(std::string value) {
        Button but;
        but.mod = getModsFromString(value);
        int i = 0;
        for(i = value.length() - 1; i >= 0; i--) {
            if(value[i] == '>') break;
        }
        i++;
        std::string tmp = value.substr(i, value.size() - i);
        tmp = trim(tmp); tmp = tmp.substr(1, tmp.size() - 1);
        if(!std::isdigit(tmp[0])){
            std::cout << "Error by reading button binding!" << std::endl;
            but.button = 0;
            return but;
        }
        but.button = Buttons[tmp[0] - '0'];
        return but;
    }

    /* get internal type from string code */
    InternalOptionType getIOTFromString(std::string str) {
        log << "getIOTFromString " << str << std::endl;

        if(str == "pln")
            return IOTPlain;
        if(str == "key")
            return IOTKey;
        if(str == "but")
            return IOTButton;
        if(str == "clr")
            return IOTColor;

        return IOTPlain;
    }

    /* return internal type and set name to the name
     * str should be trimmed! */
    InternalOptionType getIOTNameFromString(std::string str,
            std::string &name) {
        int i = 0;
        for(; i < str.length() && str[i] != '_'; i++);

        if(i == str.length()){
            log << "Warning! Option without a type!\n";
            name = str;
            return IOTPlain;
        }

        name = str.substr(i + 1, str.size() - i - 1);
        return getIOTFromString(str.substr(0, i));
    }

    /* checks if we can read the given option
     * in the requested format */
    bool isValidToRead(InternalOptionType itype, DataType type) {
        if(type == DataTypeKey && itype != IOTKey)
            return false;

        if(type == DataTypeButton && itype != IOTButton)
            return false;

        if(type == DataTypeColor && itype != IOTColor)
            return false;

        if(itype != IOTPlain)
            std::cout << "[WW] Type mismatch in config file.\n";

        /* other cases are too much to be checked */
        // TODO: implement some more invalid cases

        return true;
    }
}

Config::Config(std::string path) {
    this->path = path;
    stream.open(path, std::ios::in | std::ios::out);
    if(!stream.is_open())
        blocked = true;
    else
        readConfig();
}

Config::Config() : Config("~/.config/firerc") { }

void Config::readConfig() {
    if(blocked)
        return;

    std::string line;
    std::string currentPluginName = "";
    std::string option, value;

    while(std::getline(stream, line)) {
        line = trim(line);
        if(!line.length())
            continue;

        if(line[0] == '[') {
            currentPluginName =
                line.substr(1, line.length() - 2);
            continue;
        }

        auto pos = line.find("=");
        if(pos == std::string::npos) {
            std::cout << "Warning - Garbage in config" << std::endl;
            continue;
        }

        option = trim(line.substr(0, pos));
        std::string realOption;
        auto type = getIOTNameFromString(option, realOption);
        value  = trim(line.substr(pos + 1, line.length() - pos));

        log << "Got type " << type << " for option " << realOption << std::endl;

        tree[currentPluginName][realOption] = Option{type, value};
    }
}

void Config::reset() {
    stream.close();
    tree.clear();
    stream.open(path, std::ios::in | std::ios::out);
}

void Config::setOptionsForPlugin(PluginPtr p) {
    auto name = p->owner->name;
    auto it = tree.find(name);
    if(it == tree.end()) {
        setDefaultOptions(p);
        return;
    }

    for(auto option : p->options) {
        auto oname = option.first;
        auto it = tree[name].find(oname);

        if(it == tree[name].end()) {
            copyInto(p->options[oname]->def,
                    p->options[oname]->data);
            continue;
        }

        auto opt = it->second;
        auto reqType = p->options[oname]->type;

        if(!isValidToRead(opt.type, reqType)) {
            std::cout << "[EE] Type mismatch: \n";
            std::cout << "\t Plugin name:" << name
                << " Option name: " << oname << std::endl;

            copyInto(p->options[oname]->def,
                    p->options[oname]->data);
            continue;
        }

        auto data = opt.value;

        switch(reqType) {
            case DataTypeInt:
                p->options[oname]->data.ival =
                    readValue<int>(data);
                break;
            case DataTypeFloat:
                p->options[oname]->data.fval =
                    readValue<float>(data);
                break;
            case DataTypeBool:
                p->options[oname]->data.bval =
                    readValue<bool>(data);
                break;
            case DataTypeString:
                p->options[oname]->data.sval =
                    new std::string(data);
                break;
            case DataTypeColor:
                p->options[oname]->data.color =
                    new Color(readValue<Color>(data));
                break;
            case DataTypeKey:
                p->options[oname]->data.key =
                    new Key(readValue<Key>(data));
                break;
            case DataTypeButton:
                p->options[oname]->data.but =
                    new Button(readValue<Button>(data));
                break;
        }
    }
}
