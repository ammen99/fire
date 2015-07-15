#ifndef CONFIG_H
#define CONFIG_H
#include "./commonincludes.hpp"
#include "./plugin.hpp"

class Config {
    private:
        std::unordered_map<std::string,
            std::unordered_map<std::string, std::string>> tree;
        std::fstream stream;
        std::string  path;
        bool blocked = false;

        void readConfig();
        void setValue(Data *data, std::string val);

    public:
        Config(std::string path);
        Config();
        void setOptionsForPlugin(PluginPtr plugin);
        void reset();
};

extern Config *config;
#endif
