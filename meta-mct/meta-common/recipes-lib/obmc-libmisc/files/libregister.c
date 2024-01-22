#include "libmisc.h"
#include <errno.h>
#include <stdio.h>
#include <sys/mman.h>

#define DEVMEM      "/dev/mem"
#define MAP_SIZE    4096      // Page Size = 4KB

/* func: Read data from BMC chip register */
int read_register(uint32_t address, uint32_t *result)
{
    unsigned char* mapped_base;
    unsigned char* mapped_reg;
    off_t pa_offset;
    int fd, valid;

    fd = open(DEVMEM, O_RDONLY);

    if ( fd < 0 )
    {
        fprintf(stderr, "Could not open file /dev/mem: %s\n", strerror(errno));
        return fd;
    }

    valid = fd;

    pa_offset = ((off_t)address) & ~(MAP_SIZE - 1);

    mapped_base = mmap(NULL, MAP_SIZE, PROT_READ, MAP_PRIVATE, fd, pa_offset);
    close(fd);

    if ( mapped_base == MAP_FAILED)
    {
        fprintf(stderr, "Failed to map memory: %s\n", strerror(errno));
        munmap(mapped_base, MAP_SIZE);
        valid = -1;
        
        return valid;
    }

    mapped_reg = mapped_base + (address & (MAP_SIZE - 1));
    *result = *((volatile uint32_t*) mapped_reg);

    munmap(mapped_base, MAP_SIZE);

    return valid;
}

/* func: Write data to BMC chip register */
int write_register(uint32_t address, uint32_t value)
{
    unsigned char* mapped_base;
    unsigned char* mapped_reg;
    off_t pa_offset;
    int fd, valid;

    fd = open(DEVMEM, O_RDWR|O_SYNC);

    if ( fd < 0 )
    {
        fprintf(stderr, "Could not open file /dev/mem: %s\n", strerror(errno));
        return fd;
    }

    valid = fd;

    pa_offset = ((off_t)address) & ~(MAP_SIZE - 1);

    mapped_base = mmap(NULL, MAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, pa_offset);
    close(fd);

    if ( mapped_base == MAP_FAILED)
    {
        fprintf(stderr, "Failed to map memory: %s\n", strerror(errno));
        munmap(mapped_base, MAP_SIZE);
        valid = -1;
        
        return valid;
    }

    mapped_reg = mapped_base + (address & (MAP_SIZE - 1));
    *((volatile uint32_t*) mapped_reg) = value;

    munmap(mapped_base, MAP_SIZE);

    return valid;
}
