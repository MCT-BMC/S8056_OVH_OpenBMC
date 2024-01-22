#include <cpld-update-utils.hpp>

namespace CpldUpdateUtil
{

void update_flash_progress(int pecent, const char *version_id, int progress)
{
    if(progress){
        FILE * fp;
        char buf[100];
        sprintf(buf, "flash-progress-update %s %d", version_id, pecent);
        if ((fp = popen(buf, "r")) == NULL){
            return ;
        }
        pclose(fp);
    }
    return;
}

void setUpdateProgress(cpld_t* cpld, char* input, uint8_t base, uint8_t divisor)
{
    std::string buffer(input);
    int start = buffer.find("(", 0);
    int end = buffer.find("%", 0);
    int progress = base + (atoi(buffer.substr(start+1,end-start-1).c_str())/ divisor);
    update_flash_progress(progress,cpld->version_id,cpld->progress);
}

void logSelEvent(std::string enrty, std::string path ,
                        uint8_t eventData0, uint8_t eventData1, uint8_t eventData2)
{
    auto bus = sdbusplus::bus::new_default_system();

    std::vector<uint8_t> eventData(3, 0xFF);
    eventData[0] = eventData0;
    eventData[1] = eventData1;
    eventData[2] = eventData2;
    uint16_t genid = 0x20;

    sdbusplus::message::message writeSEL = bus.new_method_call(
        ipmi::busName, ipmi::path,ipmi::interface, ipmi::request);
    writeSEL.append(enrty, path, eventData, true, (uint16_t)genid);
    try
    {
        bus.call(writeSEL);
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to log the button event:" << e.what() << "\n";
    }
}

int i2cLatticeCpldFwUpdate(cpld_t& cpld, std::string cpldFilePath)
{
    int ret;

    CpldUpdateManager cpldpdateManager(cpld.bus, cpld.address, cpldFilePath);

    ret = cpldpdateManager.jedFileParser();
    if (ret < 0)
    {
        std::cerr << "JED file parsing failed\n";
        return ret;
    }

    update_flash_progress(30,cpld.version_id,cpld.progress);

    ret = cpldpdateManager.fwUpdate();
    if (ret < 0)
    {
        std::cerr << "CPLD update failed\n";
        return ret;
    }

    update_flash_progress(80,cpld.version_id,cpld.progress);
    ret = cpldpdateManager.fwVerify();
    if (ret < 0)
    {
        std::cerr << "CPLD verify failed\n";
        return ret;
    }

    update_flash_progress(85,cpld.version_id,cpld.progress);
    ret = cpldpdateManager.fwDone();
    if (ret < 0)
    {
        std::cerr << "CPLD finish failed\n";
        return ret;
    }

    return 0;
}

int i2cLatticeCpldJedFileUserCode(std::string cpldFilePath, unsigned int& userCode)
{
    int ret;
    uint8_t bus = 0;
    uint8_t addr = 0;

    CpldUpdateManager cpldpdateManager(bus, addr, cpldFilePath);

    ret = cpldpdateManager.jedFileParser();
    if (ret < 0)
    {
        std::cerr << "JED file parsing failed\n";
        return ret;
    }

    ret = cpldpdateManager.getJedFileUserCode(userCode);
    if (ret < 0)
    {
        std::cerr << "Get JED file user code failed\n";
        return ret;
    }
    return 0;
}

} // namespace CpldUpdateUtil