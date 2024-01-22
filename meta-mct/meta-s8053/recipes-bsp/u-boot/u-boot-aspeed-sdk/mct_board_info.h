#ifndef __MCT_BOARD_INFO_H_INCLUDED
#define __MCT_BOARD_INFO_H_INCLUDED

void unlock_scu(void);
void enable_p2a_bridge(void);
void enable_button_passthrough(void);
void disable_hwstrap_button_passthrough(void);
void enable_Heartbeat_led_hw_mode(void);
void mct_print_board_info(void);

#endif
