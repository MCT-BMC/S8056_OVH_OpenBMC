#include "gpio-initial.hpp"

static constexpr bool DEBUG = false;

namespace GPIO{

void setInput(std::string lineName)
{
    int value;
    struct gpiod_line *line = nullptr;;

    line = gpiod_line_find(lineName.c_str());

    if(line == nullptr)
    {
        std::cerr << "Error find line\n";
        return;
    }

    if (gpiod_line_request_input(line,"set-inptut") < 0)
    {
        std::cerr << "Error request line\n";
    }
    gpiod_line_release(line);
}

void setOutput(std::string lineName,int setValue)
{
    int value;
    struct gpiod_line *line = nullptr;;

    line = gpiod_line_find(lineName.c_str());

    if(line == nullptr)
    {
        std::cerr << "Error find line\n";
        return;
    }

    int ret = gpiod_line_request_output(line,"set-output",setValue);
    if (ret < 0)
    {
        std::cerr << "Error request line\n";
    }
    gpiod_line_release(line);
}
}

int main(int argc, char *argv[])
{

    std::ifstream gpioInitialConfig(gpioInitialConfigPath);

    if(!gpioInitialConfig)
    {
        std::cerr << "GPIO initial config file not found. PATH:" << gpioInitialConfigPath << std::endl;
        return 0;
    }

    auto gpioConfig = nlohmann::json::parse(gpioInitialConfig, nullptr, false);

    if(gpioConfig.is_discarded())
    {
        std::cerr << "Syntax error in " << gpioInitialConfigPath << "\n";
        return 0;
    }

    for (auto& object : gpioConfig)
    {
        if constexpr (DEBUG){
            std::cerr << LINE_NAME << " : " << object[LINE_NAME] << "\n";
            std::cerr << DIRECTION << " : " << object[DIRECTION] << "\n";
            std::cerr << VALUE << " : " << object[VALUE] << "\n";
        }

        if(object[LINE_NAME].is_null() || object[DIRECTION].is_null() || object[VALUE].is_null())
        {
            continue;
        }
        if(object[DIRECTION]=="Input")
        {
            GPIO::setInput(object[LINE_NAME]);
        }
        if(object[DIRECTION]=="Output")
        {
            GPIO::setOutput(object[LINE_NAME],object[VALUE]);
        }
        // TODO: GPIOs should be used as interrupt feature
    }

    return 0;
}