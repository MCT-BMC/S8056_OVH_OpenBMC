#ifndef _LIBOBMCGPIO_H
#define _LIBOBMCGPIO_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/gpio.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// gpio functions
int gpio_read_value(uint16_t chip_id, uint16_t gpio_num, uint8_t* value);

#ifdef  __cplusplus
}
#endif

#endif /* _LIBOBMCGPIO_H */
