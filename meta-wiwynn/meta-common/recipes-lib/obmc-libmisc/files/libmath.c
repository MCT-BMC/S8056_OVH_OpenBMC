#include "libmisc.h"

uint8_t Get_Twos_Complement_Int_Data_Format(uint32_t raw_value, double *real_value) {
    int16_t value = raw_value & 0xFFFF;

    if (((value >> 15) & 0x01) == 0x01) {
        value = ((~raw_value & 0xFFFF) + 1) *(-1); //two's complement
    }

    *real_value = value;
    return 0;
}

uint8_t Get_Linear_Data_Format(double *real_value, uint32_t raw_value) {
    int16_t exp;
    int8_t base;

    exp = raw_value & 0x7FF;
    base = (raw_value >> 11) & 0x1F;

    if (base >= 0x10) {
        base -= 0x20;
    }

    if (((exp >> 10) & 0x01) == 0x01) {
        exp = (~exp) + 1;
        exp &= 0x7FF;
        *real_value = -exp * pow(2, base);
    } else {
        *real_value = exp * pow(2, base);
    }
    return 1;
}

/* Func: PMBus VID Format Conversion
*/
int Get_VID_Data_Format(double *real_value, uint32_t raw_value, uint8_t code_type) {
    
    float vid_start, vid_step;

    switch(code_type){
        case 1:
            vid_start = 0.25;
			vid_step = 0.005;
			break;

        case 2:
            vid_start = 0.5;
			vid_step = 0.01;
			break;

        default:
            return -1;
    }

    if(raw_value != 0){
        *real_value = ((raw_value - 1) * vid_step) + vid_start;
    }
    else{
        *real_value = 0;
    }

    return 0;
}

/* Func: Zero Checksum Calculation
*/
uint8_t zeroChecksum(void *data, int size)
{
    int i = 0;
    uint8_t checksum = 0;
    uint8_t *array = data;

    for(i = 0; i < size; i++)
    {
        checksum += array[i];
    }

    return (~checksum)+1;
}
