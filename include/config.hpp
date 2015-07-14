#include "./commonincludes.hpp"
#include "./plugin.hpp"

class Config {
    private:
        std::unordered_map<std::string,
            std::unordered_map<std::string, std::string>> tree;
        std::fstream stream;

        void readConfig();
        void setValue(Data *data, std::string val);

    public:
        Config(std::string path);
        void setOptionsForPlugin(PluginPtr plugin);
        void reset();
};
