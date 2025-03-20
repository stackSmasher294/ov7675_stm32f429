#ifndef __OV7675__
#define __OV7675__

#include <stdint.h>

typedef struct ov7675_s
{
    int crop_x;
    int crop_y;
    int crop_width;
    int crop_height;
    void (*write_i2c_reg)(int slave_addr, int address, uint8_t val);
}ov7675_cfg_t;

int ov7675_init();
#endif