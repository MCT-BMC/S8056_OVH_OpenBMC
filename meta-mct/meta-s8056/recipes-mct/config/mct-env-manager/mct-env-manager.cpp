#include "mct-env-manager.hpp"

static constexpr bool DEBUG = false;

int main(int argc, char *argv[])
{

    std::ifstream envConfigfile(envConfigPath);

    if(!envConfigfile)
    {
        std::cerr << "Env config file not found. PATH:" << envConfigPath << std::endl;
        return 0;
    }

    auto envConfig = nlohmann::json::parse(envConfigfile, nullptr, false);

    if(envConfig.is_discarded())
    {
        std::cerr << "Syntax error in " << envConfigPath << std::endl;
        return 0;
    }

    for (auto& object : envConfig)
    {
        if constexpr (DEBUG)
        {
            std::cerr << KEY << " : " << object[KEY] << std::endl;
            std::cerr << VALUE << " : " << object[VALUE] << std::endl;
        }

        if(object[KEY].is_null() ||
           object[VALUE].is_null() ||
           object[VALUE].get<std::string>() == "NA")
        {
            continue;
        }

        std::string cmd = "/sbin/fw_setenv " + object[KEY].get<std::string>() + " " + object[VALUE].get<std::string>();

        FILE* fp = popen(cmd.c_str(), "r");
        if (!fp)
        {
            std::cerr << "Syntax error in export command" << std::endl;
            return -1;
        }
        pclose(fp);

        std::cerr << object[KEY]  << " = " << object[VALUE] << std::endl;
    }

    return 0;
}