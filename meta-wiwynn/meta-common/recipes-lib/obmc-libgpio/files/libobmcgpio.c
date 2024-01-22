
#include "libobmcgpio.h"

#define GPIO_OK                      0
#define GPIO_OPEN_ERROR             -1
#define GPIO_READ_ERROR             -2
#define GPIO_WRITE_ERROR            -3
#define GPIO_ERROR                  -4
#define GPIO_OPEN_LINEHANDLE_ERROR  -5

/*
    @func: Get GPIO value func
    @parm1: gpio chip number
    @parm2: gpio number
    @parm3: gpio read value
    @return: success / error number
*/
int gpio_read_value(uint16_t chip_id, uint16_t gpio_num, uint8_t* value)
{
    char filename[255] = {'\0'};
    int gpio_fd = -1;
    struct gpiohandle_request req;
    struct gpiohandle_data data;
    int rc = -1;

    sprintf(filename, "/dev/gpiochip%d", chip_id);

    // 1. Open GPIO device
    gpio_fd = open(filename, O_RDONLY);
    if (gpio_fd < 0) 
    {
        return GPIO_OPEN_ERROR;
    }

	memset(&req, 0, sizeof(req));
	strncpy(req.consumer_label, "libobmc-gpio",  sizeof(req.consumer_label));
    req.flags = GPIOHANDLE_REQUEST_INPUT;
    req.lineoffsets[0] = gpio_num;
	req.lines = 1;
    req.fd = -1;

    rc = ioctl(gpio_fd, GPIO_GET_LINEHANDLE_IOCTL, &req);
    if(rc < 0)
    {
		close(gpio_fd);
		return GPIO_OPEN_LINEHANDLE_ERROR;
	}
    close(gpio_fd);

    // 2. Read GPIO value
    if(req.fd < 0)
    {
        return GPIO_ERROR;
    }

    memset(&data, 0, sizeof(data));
    rc = ioctl(req.fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data);
    if(rc < 0)
    {
		close(req.fd);
		return GPIO_READ_ERROR;
	}

    *value = data.values[0];
    close(req.fd);

    return GPIO_OK;
}
