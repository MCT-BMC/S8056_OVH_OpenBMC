#pragma once

#include <stdint.h>

int open_jtag_dev();
int jtag_interface_end_tap_state(int fd, uint8_t reset,
                                 uint8_t endstate, uint8_t tck);
int jtag_interface_set_freq(int fd, unsigned int freq);
int jtag_interface_get_freq(int fd, unsigned int* freq);
int jtag_interface_xfer(int fd, uint8_t type, uint8_t direction,
                        uint8_t endstate, uint32_t length, uint32_t *buf);
int jtag_interface_sir_xfer(int fd, unsigned char endir,
                            unsigned int len, unsigned int tdi);
int jtag_interface_tdo_xfer(int fd, unsigned char endir,
                            unsigned int len, unsigned int *buf);
int jtag_interface_tdi_xfer(int fd, unsigned char endir,
                            unsigned int len, unsigned int *buf);
int jtag_interface_bitbang(int fd, uint8_t tms,
                           uint8_t tdi, uint8_t *tdo);
void close_jtag_dev(int fd);
