#ifndef __OV7675__
#define __OV7675__

#include <stdint.h>

typedef struct ov7675_s
{
    int crop_x;
    int crop_y;
    int frame_width;
    int frame_height;
    void (*write_i2c_reg)(int slave_addr, int address, uint8_t val);
    uint8_t (*read_i2c_reg)(int slave_addr, int address);
} ov7675_cfg_t;

extern uint16_t* g_cam_fb;

int ov7675_init();
int ov7675_grab_frame(uint16_t* frame);
#endif