#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "cpld-updater.hpp"

void usage ()
{
    std::cout << "Usage: cpld-updater [file name] : update cpld firmware\n";
}

int main(int argc, char **argv)
{

    if (argc != 2) {
        usage();
        return -1;
    }

    if (access(argv[1], F_OK) < 0)
    {
        std::cerr << "Missing CPLD FW file.\n";
        return -1;
    }

    struct stat buf;

    if (stat(argv[1], &buf) < 0)
    {
        std::cerr << "Failed to get CPLD FW file info.\n";
        return -1;
    }

    if (!S_ISREG(buf.st_mode))
    {
        std::cerr << "Invalid CPLD FW file type.\n";
        return -1;
    }

    int ret = -1;
    CpldUpdateManager cpld_updater;

    // Open jtag device
    ret = cpld_updater.cpldUpdateDevOpen();
    if (ret < 0)
    {
        std::cerr << "Failed to open jtag device.\n";
        cpld_updater.cpldUpdateCloseAll();
        return -1;
    }

    // Check CPLD ID
    ret = cpld_updater.cpldUpdateCheckId();
    if (ret < 0)
    {
        std::cerr << "Failed to check CPLD ID.\n";
        cpld_updater.cpldUpdateCloseAll();
        return -1;
    }

    // Open CPLD FW File
    std::string cpld_path = argv[1];

    ret = cpld_updater.cpldUpdateFileOpen(cpld_path);
    if (ret < 0)
    {
        std::cerr << "Failed to open CPLD FW file [" << cpld_path.c_str() <<"]\n";
        cpld_updater.cpldUpdateCloseAll();
        return -1;
    }

    // Get Update Data Size
    ret = cpld_updater.cpldUpdateGetUpdateDataSize();
    if (ret < 0)
    {
        std::cerr << "Failed to get update data size.\n";
        cpld_updater.cpldUpdateCloseAll();
        return -1;
    }

    // Get Update File Parsed
    ret = cpld_updater.cpldUpdateJedFileParser();
    if (ret < 0)
    {
        std::cerr << "Failed to parse CPLD JED File.\n";
        cpld_updater.cpldUpdateCloseAll();
        return -1;
    }

    // Enter transparent mode
    ret = cpld_updater.cpldUpdateCpldStart();
    if (ret < 0)
    {
        std::cerr << "Failed to enter Transparent mode.\n";
        cpld_updater.cpldUpdateCloseAll();
        return -1;
    }

    // Erase stage
    std::cout << "Erasing...\n";
    ret = cpld_updater.cpldUpdateCpldErase();
    if (ret < 0)
    {
        std::cerr << "Failed in the Erase stage.\n";
        cpld_updater.cpldUpdateCloseAll();
        return -1;
    }

    // Program stage
    std::cout << "Programming...\n";
    ret = cpld_updater.cpldUpdateCpldProgram();
    if (ret < 0)
    {
        std::cerr << "Failed in the Program stage.\n";
        cpld_updater.cpldUpdateCloseAll();
        return -1;
    }

    // Verify stage
    std::cout << "Verifying...\n";
    ret = cpld_updater.cpldUpdateCpldVerify();
    if (ret < 0)
    {
        std::cerr << "Failed in the Verify stage.\n";
        cpld_updater.cpldUpdateCloseAll();
        return -1;
    }

    // Exit transparent mode
    ret = cpld_updater.cpldUpdateCpldEnd();
    if (ret < 0)
    {
        std::cerr << "Failed to exit Transparent mode.\n";
        cpld_updater.cpldUpdateCloseAll();
        return -1;
    }

    cpld_updater.cpldUpdateCloseAll();

    std::cout << "=== CPLD FW update finished ===\n";

    return 0;
}
