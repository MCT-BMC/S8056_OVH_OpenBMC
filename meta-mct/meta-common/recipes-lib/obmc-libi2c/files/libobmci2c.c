
#include "libobmci2c.h"

static bool DEBUG = false;

#define POLY    (0x1070U << 3)
static uint8_t crc8(uint16_t data)
{
	int i;

	for (i = 0; i < 8; i++) {
		if (data & 0x8000)
			data = data ^ POLY;
		data = data << 1;
	}
	return (uint8_t)(data >> 8);
}

/* Incremental CRC8 over count bytes in the array pointed to by p */
uint8_t i2c_smbus_pec(uint8_t crc, uint8_t *p, size_t count)
{
	int i;

	for (i = 0; i < count; i++)
		crc = crc8((crc ^ p[i]) << 8);
	return crc;
}

int open_i2c_dev(int i2cbus, char *filename, size_t size, int quiet)
{
    int file;

    snprintf(filename, size, "/dev/i2c/%d", i2cbus);
    filename[size - 1] = '\0';
    file = open(filename, O_RDWR);

    if (file < 0 && (errno == ENOENT || errno == ENOTDIR)) {
        sprintf(filename, "/dev/i2c-%d", i2cbus);
        file = open(filename, O_RDWR);
    }

    if (file < 0 && !quiet) {
        if (errno == ENOENT) {
            fprintf(stderr, "Error: Could not open file "
                    "`/dev/i2c-%d' or `/dev/i2c/%d': %s\n",
                    i2cbus, i2cbus, strerror(ENOENT));
        } else {
            fprintf(stderr, "Error: Could not open file "
                    "`%s': %s\n", filename, strerror(errno));
            if (errno == EACCES)
                fprintf(stderr, "Run as root?\n");
        }
    }

    return file;
}

int open_i2c_slave_dev(int i2cbus, int bmc_addr)
{
    int file;
    char slaveDevPath[MAX_FILE_PATH_SIZE] = {0};

    // 7-bit BMC slave address
    snprintf(slaveDevPath, MAX_FILE_PATH_SIZE,
             "/sys/bus/i2c/devices/i2c-%d/%d-%04x/slave-mqueue",
             i2cbus, i2cbus, bmc_addr);

    file = open(slaveDevPath, O_RDWR | O_NONBLOCK | O_CLOEXEC);
    if (file < 0)
    {
        if (DEBUG)
        {
            fprintf(stderr, "Unable to open i2c slave-mqueue [%s]\n", slaveDevPath);
        }
    }

    return file;
}

int set_slave_addr(int file, int address, int force)
{
    /* With force, let the user read from/write to the registers
       even when a driver is also running */
    if (ioctl(file, force ? I2C_SLAVE_FORCE : I2C_SLAVE, address) < 0) {
        fprintf(stderr,
                "Error: Could not set address to 0x%02x: %s\n",
                address, strerror(errno));
        return -errno;
    }
    return 0;
}

void close_i2c_dev(int file)
{
    close(file);
}

int i2c_master_write_read(int file, uint8_t slave_addr,
                          uint8_t tx_cnt, uint8_t* tx_buf,
                          uint8_t rx_cnt, uint8_t* rx_buf)
{
    struct i2c_rdwr_ioctl_data iomsg;
    struct i2c_msg i2cmsg[2];
    int ret;

    // 7-bit slave address
    i2cmsg[0].addr = slave_addr & 0xFF;
    i2cmsg[0].flags = 0;
    i2cmsg[0].len = tx_cnt;
    i2cmsg[0].buf = (char *)tx_buf;

    i2cmsg[1].addr = slave_addr & 0xFF;
    i2cmsg[1].flags = I2C_M_RD;
    i2cmsg[1].len = rx_cnt;
    i2cmsg[1].buf = (char *)rx_buf;

    iomsg.msgs = i2cmsg;
    iomsg.nmsgs = 2;

    ret = ioctl(file, I2C_RDWR, &iomsg);

    return ret;
}

int i2c_master_write(int file, uint8_t slave_addr,
                     uint8_t tx_cnt, uint8_t* tx_buf)
{
    struct i2c_rdwr_ioctl_data iomsg;
    struct i2c_msg i2cmsg;
    int ret;

    // 7-bit slave address
    i2cmsg.addr = slave_addr & 0xFF;
    i2cmsg.flags = 0;
    i2cmsg.len = tx_cnt;
    i2cmsg.buf = (char *)tx_buf;

    iomsg.msgs = &i2cmsg;
    iomsg.nmsgs = 1;

    ret = ioctl(file, I2C_RDWR, &iomsg);

    return ret;
}

int i2c_slave_read(int file, uint8_t* rx_buf)
{
    uint8_t tmp_buf[MAX_SLAVE_REV_SIZE] = {0};
    int readLen = -1;

    readLen = read(file, tmp_buf, MAX_SLAVE_REV_SIZE);

    if (readLen > 0)
    {
        memcpy(rx_buf, tmp_buf, readLen);
    }

    return readLen;
}

int i2c_slave_clear_buffer(int file)
{
    uint8_t tmp_buf[CLR_SLAVE_BUF_SIZE] = {0};
    int ret;

    tmp_buf[0] = IOCTL_CLR_SLAVE_BUF;
    ret = write(file, tmp_buf, CLR_SLAVE_BUF_SIZE);

    return ret;
}

/**
 *  @brief Function of getting EEPROM data on i2c bus
 *
 *  @param i2cbus: The i2c bus number of EEPROM
 *  @param i2caddr: The i2c address (7-bit) of EEPROM
 *  @param offset: Offset to target location
 *  @param len: The data length retrieved from target location
 *  @param buffer: The data buffer of returned data
 *
 *  @return Status of fetching EEPROM data.
 *      0: Success
 *      others: Fail
 **/
uint8_t i2cEEPROMGet(const char* i2cbus, const char* i2caddr, uint32_t offset,
                     uint8_t len, uint8_t* buffer)
{
    uint8_t status = SUCCESS;
    char eeprom_path[64] = {0};
    FILE *eeprom_fp;
    uint8_t* ptr = NULL;
    uint8_t result, unit = 1;

    ptr = (uint8_t*)malloc(sizeof(uint8_t)*len);
    if (NULL == ptr)
    {
        fprintf(stderr, "Failed to allocate memory.\n");
        return FAILURE;
    }

    sprintf(eeprom_path, "/sys/bus/i2c/devices/%s-00%s/eeprom", i2cbus, i2caddr);

    // Open EEPROM bin file
    eeprom_fp = fopen(eeprom_path, "rb");
    if (NULL == eeprom_fp)
    {
        fprintf(stderr, "eeprom_fp: Unable to open the file %s\n", eeprom_path);

        free(ptr);

        return FAILURE;
    }

    fseek(eeprom_fp, offset, SEEK_SET);

    result = fread(ptr, unit, len, eeprom_fp);
    if (result != (len * unit))
    {
        fprintf(stderr, "Failed to read EEPROM.\n");

        fclose(eeprom_fp);

        free(ptr);

        return FAILURE;
    }

    memcpy(buffer, ptr, len);

    fclose(eeprom_fp);

    free(ptr);

    return status;
}

uint8_t i2cEEPROMSet(const char* i2cbus, const char* i2caddr, uint32_t offset,
                     uint8_t len, uint8_t* buffer)
{
    uint8_t status = SUCCESS;

    char eeprom_path[64] = {0};
    FILE *eeprom_fp;
    uint8_t* ptr = NULL;
    uint8_t result, unit = 1;

    ptr = (uint8_t*)malloc(sizeof(uint8_t)*len);
    if (NULL == ptr)
    {
        fprintf(stderr, "Failed to allocate memory.\n");
        return FAILURE;
    }

    memcpy(ptr, buffer, len);

    sprintf(eeprom_path, "/sys/bus/i2c/devices/%s-00%s/eeprom", i2cbus, i2caddr);

    // Open EEPROM bin file
    eeprom_fp = fopen(eeprom_path, "wb");
    if (NULL == eeprom_fp)
    {
        fprintf(stderr, "eeprom_fp: Unable to open the file %s\n", eeprom_path);

        free(ptr);

        return FAILURE;
    }

    fseek(eeprom_fp, offset, SEEK_SET);

    result = fwrite(ptr, unit, len, eeprom_fp);
    if (result != (len * unit))
    {
        fprintf(stderr, "Failed to write EEPROM.\n");

        fclose(eeprom_fp);

        free(ptr);

        return FAILURE;
    }

    fclose(eeprom_fp);

    free(ptr);

    return status;
}
