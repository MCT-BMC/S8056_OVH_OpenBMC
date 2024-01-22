#include <psu-update-utils.hpp>

namespace PsuUpdateUtil
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

std::vector<uint8_t> hexToNumByteArray(const std::string &srcStr) {
    std::vector<uint8_t> result;
    std::stringstream ss;
    size_t code = 0;

    if (srcStr.size() < 2) {
        return result;
    }

    for ( size_t i=1; i<srcStr.size(); i+=2 ) {
        ss << std::hex << srcStr.substr(i-1, 2);
        ss >> code;
        result.push_back(code);
        ss.clear();
    }

    return result;
}

std::string byteArrayToString(std::vector<uint8_t> byteArray) {
    std::string result;
    for(int i = 0; i < byteArray.size(); i++)
    {
        std::stringstream ss;
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byteArray[i]) << " ";
        result = result + "0x" + ss.str();
    }

    return result;
}

int8_t writeReadPmbusCmd(uint8_t bus, uint8_t addr,
                         std::vector<uint8_t> writeData, uint8_t readDateSize,
                         std::vector<uint8_t> *resData)
{
    int res = -1;
    std::vector<char> filename;
    filename.assign(20, 0);

    resData->clear();

    int fd = open_i2c_dev(bus, filename.data(), filename.size(), 0);
    if (fd < 0)
    {
        std::cerr << "Fail to open I2C device: " << bus << "\n";
        return -1;
    }

    std::vector<uint8_t> readData;
    readData.assign(readDateSize, 0x0);

    res = i2c_master_write_read(fd, addr, writeData.size(), writeData.data(),
                                readData.size(), readData.data());
    if (res < 0)
    {
        std::cerr << "Fail to r/w I2C device: " << +bus
                  << ", Addr: " << +addr << "\n";
        close_i2c_dev(fd);
        return -1;
    }
    *resData = readData;

/*

    printf("writePmbusCmd: ");
    for(int i = 0; i < writeData.size();i++)
    {
        printf("%.2X ", writeData[i]);
    }
    printf("\n");

    printf("ReadPmbusCmd: ");
    for(int i = 0; i < readData.size();i++)
    {
        printf("%.2X ", readData[i]);
    }
    printf("\n");
*/

    close_i2c_dev(fd);
    return 0;
}

int8_t writePmbusCmd(uint8_t bus, uint8_t addr, std::vector<uint8_t> writeData)
{
    std::vector<char> filename;
    filename.assign(20, 0);
    int fd = open_i2c_dev(bus, filename.data(), filename.size(), 0);
    if (fd < 0)
    {
        std::cerr << "Fail to open I2C device: " << bus << "\n";
        return -1;
    }
/*
    std::cout << "\n==write==\n";
    for (int i = 0; i < writeData.size(); i++)
    {
        std::cout << unsigned(writeData.at(i)) << " ";
    }
    std::cout << "\n==stop==\n";

    printf("writePmbusCmd: ");
    for(int i = 0; i < writeData.size();i++)
    {
        printf("%.2X ", writeData[i]);
    }
    printf("\n");
*/


    int ret = i2c_master_write(fd, addr, writeData.size(), writeData.data());
    if (ret < 0)
    {
        std::cerr << "Fail to write I2C device: " << bus
                  << ", Addr: " << addr << "\n";
        close_i2c_dev(fd);
        return -1;
    }

    close_i2c_dev(fd);
    return 0;
}

int8_t sendBlockWriteRead(uint8_t bus, uint8_t addr,
                          uint8_t cmd, std::vector<uint8_t> *rBuf)
{
    std::vector<uint8_t> cmdData;
    cmdData.assign(1, cmd);
    std::vector<uint8_t> mfrSize;

    // Get the data length
    int ret = writeReadPmbusCmd(bus, addr, cmdData, 1, &mfrSize);
    if (ret < 0)
    {
        std::cerr << __func__ << ": Get PSU byte value fail\n";
        return -1;
    }

    if (mfrSize.size() == 1)
    {
        if (mfrSize[0] == 0xFF)
        {
            std::cerr << __func__ << ": Unknown PSU upload mode\n";
            return -1;
        }
    }

    // Get the response of data
    ret = writeReadPmbusCmd(bus, addr, cmdData, mfrSize[0] + 1, rBuf);
    if (ret < 0)
    {
        std::cerr << __func__ << ": Get PSU byte value fail\n";
        return -1;
    }

    return 0;
}
int8_t getMfrId(uint8_t bus, uint8_t addr, std::string &mfrId)
{
    std::vector<uint8_t> rBuf;
    int ret = sendBlockWriteRead(bus, addr, CMD_MFR_ID, &rBuf);
    if (ret < 0)
    {
        std::cerr << __func__ << ": Get MFR ID fail\n";
        return -1;
    }

    if (rBuf.size() > 1)
    {
        mfrId.assign(rBuf.begin() + 1, rBuf.end());
        std::cout << "MFR ID = " << mfrId << "\n";
    }
    else
    {
        std::cerr << __func__ << ": Get MFR ID size fail\n";
        return -1;
    }

    return 0;
}

int8_t getMfrModel(uint8_t bus, uint8_t addr, std::string &mfrModel)
{
    std::vector<uint8_t> rBuf;
    int ret = sendBlockWriteRead(bus, addr, CMD_MFR_MODEL, &rBuf);
    if (ret < 0)
    {
        std::cerr << __func__ << ": Get MFR ID fail\n";
        return -1;
    }

    if (rBuf.size() > 1)
    {
        mfrModel.assign(rBuf.begin() + 1, rBuf.end());
        std::cout << "MFR model = " << mfrModel << "\n";
    }
    else
    {
        std::cerr << __func__ << ": Get MFR ID size fail\n";
        return -1;
    }

    return 0;
}

} // namespace PsuUpdateUtil