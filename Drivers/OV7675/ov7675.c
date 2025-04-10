#include "ov7675.h"
#include "main.h"
#include "stm32f4xx_hal_i2c.h"
#include "clock.h"

#define OV7675_I2C_ADDR 0x21

/* Registers */
#define REG_GAIN 0x00     /* Gain lower 8 bits (rest in vref) */
#define REG_BLUE 0x01     /* blue gain */
#define REG_RED 0x02      /* red gain */
#define REG_VREF 0x03     /* Pieces of GAIN, VSTART, VSTOP */
#define REG_COM1 0x04     /* Control 1 */
#define COM1_CCIR656 0x40 /* CCIR656 enable */
#define REG_BAVE 0x05     /* U/B Average level */
#define REG_GbAVE 0x06    /* Y/Gb Average level */
#define REG_AECHH 0x07    /* AEC MS 5 bits */
#define REG_RAVE 0x08     /* V/R Average level */
#define REG_COM2 0x09     /* Control 2 */
#define COM2_SSLEEP 0x10  /* Soft sleep mode */
#define REG_PID 0x0a      /* Product ID MSB */
#define REG_VER 0x0b      /* Product ID LSB */
#define REG_COM3 0x0c     /* Control 3 */
#define COM3_SWAP 0x40    /* Byte swap */
#define COM3_SCALEEN 0x08 /* Enable scaling */
#define COM3_DCWEN 0x04   /* Enable downsamp/crop/window */
#define REG_COM4 0x0d     /* Control 4 */
#define REG_COM5 0x0e     /* All "reserved" */
#define REG_COM6 0x0f     /* Control 6 */
#define REG_AECH 0x10     /* More bits of AEC value */
#define REG_CLKRC 0x11    /* Clocl control */
#define CLK_EXT 0x40      /* Use external clock directly */
#define CLK_SCALE 0x3f    /* Mask for internal clock scale */
#define REG_COM7 0x12     /* Control 7 */
#define COM7_RESET 0x80   /* Register reset */
#define COM7_FMT_MASK 0x38
#define COM7_FMT_VGA 0x00
#define COM7_FMT_CIF 0x20   /* CIF format */
#define COM7_FMT_QVGA 0x10  /* QVGA format */
#define COM7_FMT_QCIF 0x08  /* QCIF format */
#define COM7_RGB 0x04       /* bits 0 and 2 - RGB format */
#define COM7_YUV 0x00       /* YUV */
#define COM7_BAYER 0x01     /* Bayer format */
#define COM7_PBAYER 0x05    /* "Processed bayer" */
#define REG_COM8 0x13       /* Control 8 */
#define COM8_FASTAEC 0x80   /* Enable fast AGC/AEC */
#define COM8_AECSTEP 0x40   /* Unlimited AEC step size */
#define COM8_BFILT 0x20     /* Band filter enable */
#define COM8_AGC 0x04       /* Auto gain enable */
#define COM8_AWB 0x02       /* White balance enable */
#define COM8_AEC 0x01       /* Auto exposure enable */
#define REG_COM9 0x14       /* Control 9  - gain ceiling */
#define REG_COM10 0x15      /* Control 10 */
#define COM10_HSYNC 0x40    /* HSYNC instead of HREF */
#define COM10_PCLK_HB 0x20  /* Suppress PCLK on horiz blank */
#define COM10_HREF_REV 0x08 /* Reverse HREF */
#define COM10_VS_LEAD 0x04  /* VSYNC on clock leading edge */
#define COM10_VS_NEG 0x02   /* VSYNC negative */
#define COM10_HS_NEG 0x01   /* HSYNC negative */
#define REG_HSTART 0x17     /* Horiz start high bits */
#define REG_HSTOP 0x18      /* Horiz stop high bits */
#define REG_VSTART 0x19     /* Vert start high bits */
#define REG_VSTOP 0x1a      /* Vert stop high bits */
#define REG_PSHFT 0x1b      /* Pixel delay after HREF */
#define REG_MIDH 0x1c       /* Manuf. ID high */
#define REG_MIDL 0x1d       /* Manuf. ID low */
#define REG_MVFP 0x1e       /* Mirror / vflip */
#define MVFP_MIRROR 0x20    /* Mirror image */
#define MVFP_FLIP 0x10      /* Vertical flip */

#define REG_AEW 0x24      /* AGC upper limit */
#define REG_AEB 0x25      /* AGC lower limit */
#define REG_VPT 0x26      /* AGC/AEC fast mode op region */
#define REG_HSYST 0x30    /* HSYNC rising edge delay */
#define REG_HSYEN 0x31    /* HSYNC falling edge delay */
#define REG_HREF 0x32     /* HREF pieces */
#define REG_TSLB 0x3a     /* lots of stuff */
#define TSLB_YLAST 0x04   /* UYVY or VYUY - see com13 */
#define REG_COM11 0x3b    /* Control 11 */
#define COM11_NIGHT 0x80  /* NIght mode enable */
#define COM11_NMFR 0x60   /* Two bit NM frame rate */
#define COM11_HZAUTO 0x10 /* Auto detect 50/60 Hz */
#define COM11_50HZ 0x08   /* Manual 50Hz select */
#define COM11_EXP 0x02
#define REG_COM12 0x3c     /* Control 12 */
#define COM12_HREF 0x80    /* HREF always */
#define REG_COM13 0x3d     /* Control 13 */
#define COM13_GAMMA 0x80   /* Gamma enable */
#define COM13_UVSAT 0x40   /* UV saturation auto adjustment */
#define COM13_UVSWAP 0x01  /* V before U - w/TSLB */
#define REG_COM14 0x3e     /* Control 14 */
#define COM14_DCWEN 0x10   /* DCW/PCLK-scale enable */
#define REG_EDGE 0x3f      /* Edge enhancement factor */
#define REG_COM15 0x40     /* Control 15 */
#define COM15_R10F0 0x00   /* Data range 10 to F0 */
#define COM15_R01FE 0x80   /*            01 to FE */
#define COM15_R00FF 0xc0   /*            00 to FF */
#define COM15_RGB565 0x10  /* RGB565 output */
#define COM15_RGB555 0x30  /* RGB555 output */
#define REG_COM16 0x41     /* Control 16 */
#define COM16_AWBGAIN 0x08 /* AWB gain enable */
#define REG_COM17 0x42     /* Control 17 */
#define COM17_AECWIN 0xc0  /* AEC window - must match COM4 */
#define COM17_CBAR 0x08    /* DSP Color bar */
/*
 * This matrix defines how the colors are generated, must be
 * tweaked to adjust hue and saturation.
 *
 * Order: v-red, v-green, v-blue, u-red, u-green, u-blue
 *
 * They are nine-bit signed quantities, with the sign bit
 * stored in 0x58.  Sign for v-red is bit 0, and up from there.
 */
#define REG_CMATRIX_BASE 0x4f
#define CMATRIX_LEN 6
#define REG_CMATRIX_SIGN 0x58

#define REG_BRIGHT 0x55  /* Brightness */
#define REG_CONTRAS 0x56 /* Contrast control */

#define REG_GFIX 0x69 /* Fix gain control */

#define REG_DBLV 0x6b    /* PLL control an debugging */
#define DBLV_BYPASS 0x0a /* Bypass PLL */
#define DBLV_X4 0x4a     /* clock x4 */
#define DBLV_X6 0x8a     /* clock x6 */
#define DBLV_X8 0xca     /* clock x8 */

#define REG_SCALING_XSC 0x70 /* Test pattern and horizontal scale factor */
#define TEST_PATTTERN_0 0x80
#define REG_SCALING_YSC 0x71 /* Test pattern and vertical scale factor */
#define TEST_PATTTERN_1 0x80

#define REG_REG76 0x76   /* OV's name */
#define R76_BLKPCOR 0x80 /* Black pixel correction enable */
#define R76_WHTPCOR 0x40 /* White pixel correction enable */

#define REG_RGB444 0x8c  /* RGB 444 control */
#define R444_ENABLE 0x02 /* Turn on RGB444, overrides 5x5 */
#define R444_RGBX 0x01   /* Empty nibble at end */

#define REG_HAECC1 0x9f /* Hist AEC/AGC control 1 */
#define REG_HAECC2 0xa0 /* Hist AEC/AGC control 2 */

#define REG_BD50MAX 0xa5 /* 50hz banding step limit */
#define REG_HAECC3 0xa6  /* Hist AEC/AGC control 3 */
#define REG_HAECC4 0xa7  /* Hist AEC/AGC control 4 */
#define REG_HAECC5 0xa8  /* Hist AEC/AGC control 5 */
#define REG_HAECC6 0xa9  /* Hist AEC/AGC control 6 */
#define REG_HAECC7 0xaa  /* Hist AEC/AGC control 7 */
#define REG_BD60MAX 0xab /* 60hz banding step limit */


struct regval_list {
	uint8_t reg_num;
	uint8_t value;
};

static struct regval_list ov7670_default_regs[] = {
	{ REG_COM7, COM7_RESET },
/*
 * Clock scale: 3 = 15fps
 *              2 = 20fps
 *              1 = 30fps
 */
	{ REG_CLKRC, 0x1 },	/* OV: clock scale */
	{ REG_TSLB,  0x0D },	/* OV */
	{ REG_COM7, COM7_FMT_QVGA },	/* VGA */
	/*
	 * Set the hardware window.  These values from OV don't entirely
	 * make sense - hstop is less than hstart.  But they work...
	 */
	{ REG_HSTART, 0x13 },	{ REG_HSTOP, 0x01 },
	{ REG_HREF, 0xb6 },	{ REG_VSTART, 0x21 },
	{ REG_VSTOP, 0x5d },	{ REG_VREF, 0x00 },

	{ REG_COM3, 0 },	
    {REG_COM2, 0x3}, // 4x drive
    { REG_COM14, 0x20 }, // pclk divided by 16
	/* Mystery scaling numbers */
	{ REG_SCALING_XSC, 0x3a },
	{ REG_SCALING_YSC, 0x35 },
	{ 0x72, 0x11 },		{ 0x73, 0xf0 },
	{ 0xa2, 0x02 },		{ REG_COM10,  COM10_PCLK_HB },

	/* Gamma curve values */
	{ 0x7a, 0x20 },		{ 0x7b, 0x10 },
	{ 0x7c, 0x1e },		{ 0x7d, 0x35 },
	{ 0x7e, 0x5a },		{ 0x7f, 0x69 },
	{ 0x80, 0x76 },		{ 0x81, 0x80 },
	{ 0x82, 0x88 },		{ 0x83, 0x8f },
	{ 0x84, 0x96 },		{ 0x85, 0xa3 },
	{ 0x86, 0xaf },		{ 0x87, 0xc4 },
	{ 0x88, 0xd7 },		{ 0x89, 0xe8 },

	/* AGC and AEC parameters.  Note we start by disabling those features,
	   then turn them only after tweaking the values. */
	{ REG_COM8, COM8_FASTAEC | COM8_AECSTEP | COM8_BFILT },
	{ REG_GAIN, 0 },	{ REG_AECH, 0 },
	{ REG_COM4, 0x40 }, /* magic reserved bit */
	{ REG_COM9, 0x18 }, /* 4x gain + magic rsvd bit */
	{ REG_BD50MAX, 0x05 },	{ REG_BD60MAX, 0x07 },
	{ REG_AEW, 0x95 },	{ REG_AEB, 0x33 },
	{ REG_VPT, 0xe3 },	{ REG_HAECC1, 0x78 },
	{ REG_HAECC2, 0x68 },	{ 0xa1, 0x03 }, /* magic */
	{ REG_HAECC3, 0xd8 },	{ REG_HAECC4, 0xd8 },
	{ REG_HAECC5, 0xf0 },	{ REG_HAECC6, 0x90 },
	{ REG_HAECC7, 0x94 },
	{ REG_COM8, COM8_FASTAEC|COM8_AECSTEP|COM8_BFILT|COM8_AGC|COM8_AEC },

	/* Almost all of these are magic "reserved" values.  */
	{ REG_COM5, 0x61 },	{ REG_COM6, 0x4b },
	{ 0x16, 0x02 },		{ REG_MVFP, 0x07 },
	{ 0x21, 0x02 },		{ 0x22, 0x91 },
	{ 0x29, 0x07 },		{ 0x33, 0x0b },
	{ 0x35, 0x0b },		{ 0x37, 0x1d },
	{ 0x38, 0x71 },		{ 0x39, 0x2a },
	{ REG_COM12, 0x78 },	{ 0x4d, 0x40 },
	{ 0x4e, 0x20 },		{ REG_GFIX, 0 },
	{ 0x6b, 0x4a },		{ 0x74, 0x10 },
	{ 0x8d, 0x4f },		{ 0x8e, 0 },
	{ 0x8f, 0 },		{ 0x90, 0 },
	{ 0x91, 0 },		{ 0x96, 0 },
	{ 0x9a, 0 },		{ 0xb0, 0x84 },
	{ 0xb1, 0x0c },		{ 0xb2, 0x0e },
	{ 0xb3, 0x82 },		{ 0xb8, 0x0a },

	/* More reserved magic, some of which tweaks white balance */
	{ 0x43, 0x0a },		{ 0x44, 0xf0 },
	{ 0x45, 0x34 },		{ 0x46, 0x58 },
	{ 0x47, 0x28 },		{ 0x48, 0x3a },
	{ 0x59, 0x88 },		{ 0x5a, 0x88 },
	{ 0x5b, 0x44 },		{ 0x5c, 0x67 },
	{ 0x5d, 0x49 },		{ 0x5e, 0x0e },
	{ 0x6c, 0x0a },		{ 0x6d, 0x55 },
	{ 0x6e, 0x11 },		{ 0x6f, 0x9f }, /* "9e for advance AWB" */
	{ 0x6a, 0x40 },		{ REG_BLUE, 0x40 },
	{ REG_RED, 0x60 },
	{ REG_COM8, COM8_FASTAEC|COM8_AECSTEP|COM8_BFILT|COM8_AGC|COM8_AEC|COM8_AWB },

	/* Matrix coefficients */
	{ 0x4f, 0x80 },		{ 0x50, 0x80 },
	{ 0x51, 0 },		{ 0x52, 0x22 },
	{ 0x53, 0x5e },		{ 0x54, 0x80 },
	{ 0x58, 0x9e },

	{ REG_COM16, COM16_AWBGAIN },	{ REG_EDGE, 0 },
	{ 0x75, 0x05 },		{ 0x76, 0xe1 },
	{ 0x4c, 0 },		{ 0x77, 0x01 },
	{ REG_COM13, 0xc3 },	{ 0x4b, 0x09 },
	{ 0xc9, 0x60 },		{ REG_COM16, 0x38 },
	{ 0x56, 0x40 },

	{ 0x34, 0x11 },		{ REG_COM11, COM11_EXP|COM11_HZAUTO },
	{ 0xa4, 0x88 },		{ 0x96, 0 },
	{ 0x97, 0x30 },		{ 0x98, 0x20 },
	{ 0x99, 0x30 },		{ 0x9a, 0x84 },
	{ 0x9b, 0x29 },		{ 0x9c, 0x03 },
	{ 0x9d, 0x4c },		{ 0x9e, 0x3f },
	{ 0x78, 0x04 },

	/* Extra-weird stuff.  Some sort of multiplexor register */
	{ 0x79, 0x01 },		{ 0xc8, 0xf0 },
	{ 0x79, 0x0f },		{ 0xc8, 0x00 },
	{ 0x79, 0x10 },		{ 0xc8, 0x7e },
	{ 0x79, 0x0a },		{ 0xc8, 0x80 },
	{ 0x79, 0x0b },		{ 0xc8, 0x01 },
	{ 0x79, 0x0c },		{ 0xc8, 0x0f },
	{ 0x79, 0x0d },		{ 0xc8, 0x20 },
	{ 0x79, 0x09 },		{ 0xc8, 0x80 },
	{ 0x79, 0x02 },		{ 0xc8, 0xc0 },
	{ 0x79, 0x03 },		{ 0xc8, 0x40 },
	{ 0x79, 0x05 },		{ 0xc8, 0x30 },
	{ 0x79, 0x26 },

	{ 0xff, 0xff },	/* END MARKER */
};

static struct regval_list ov7670_fmt_yuv422[] = {
	{ REG_COM7, 0x0 },  /* Selects YUV mode */
	{ REG_RGB444, 0 },	/* No RGB444 please */
    {REG_TSLB, 0x01},
	{ REG_COM1, 0 },	/* CCIR601 */
	{ REG_COM15, COM15_R00FF },
	{ REG_COM9, 0x48 }, /* 32x gain ceiling; 0x8 is reserved bit */
	{ 0x4f, 0x80 },		/* "matrix coefficient 1" */
	{ 0x50, 0x80 },		/* "matrix coefficient 2" */
	{ 0x51, 0    },		/* vb */
	{ 0x52, 0x22 },		/* "matrix coefficient 4" */
	{ 0x53, 0x5e },		/* "matrix coefficient 5" */
	{ 0x54, 0x80 },		/* "matrix coefficient 6" */
	{ REG_COM13, COM13_GAMMA|COM13_UVSAT },
	{ 0xff, 0xff },
};

static struct regval_list ov7670_fmt_rgb565[] = {
	{ REG_COM7, COM7_RGB | COM7_FMT_QVGA },	/* Selects RGB mode */
	{ REG_RGB444, 0 },	/* No RGB444 please */
	{ REG_COM1, 0x0 },	/* CCIR601 */
	{ REG_COM15, COM15_RGB565 },
	{ REG_COM9, 0x38 },	/* 16x gain ceiling; 0x8 is reserved bit */
	{ 0x4f, 0xb3 },		/* "matrix coefficient 1" */
	{ 0x50, 0xb3 },		/* "matrix coefficient 2" */
	{ 0x51, 0    },		/* vb */
	{ 0x52, 0x3d },		/* "matrix coefficient 4" */
	{ 0x53, 0xa7 },		/* "matrix coefficient 5" */
	{ 0x54, 0xe4 },		/* "matrix coefficient 6" */
	{ REG_COM13, COM13_GAMMA|COM13_UVSAT },
	{ 0xff, 0xff },
};

__attribute__((section(".ccmram"))) uint16_t scratch_buffer[640];
// uint16_t scratch_buffer[640];


static uint8_t ov7675_read_reg(uint8_t addr)
{
    HAL_StatusTypeDef status = HAL_OK;
    uint8_t value = 0;
    const uint8_t slave_addr = (OV7675_I2C_ADDR << 1);

    status = HAL_I2C_Master_Transmit(&hi2c3, slave_addr, &addr, 1, HAL_MAX_DELAY);
    if (status != HAL_OK)
    {
        printf("HAL_I2C_Master_Transmit failed for 0x%02x [ret: %d]\n", addr, status);
        printf("HAL I2C Error code: 0x%08x\n", hi2c3.ErrorCode);
    }

    // Receive the data from the register
    status = HAL_I2C_Master_Receive(&hi2c3, slave_addr, &value, 1, HAL_MAX_DELAY);
    if (status != HAL_OK)
    {
        printf("HAL_I2C_Master_Receive failed [ret: %d]\n", status);
        printf("HAL I2C Error code: 0x%08x\n", hi2c3.ErrorCode);
    }

    return value;
}

static int ov7675_write_reg(uint8_t addr, uint8_t value)
{
    const uint8_t slave_addr = (OV7675_I2C_ADDR << 1);
    HAL_StatusTypeDef status = HAL_OK;
    uint8_t tx_buf[2];
    tx_buf[0] = addr;
    tx_buf[1] = value;
    status = HAL_I2C_Master_Transmit(&hi2c3, slave_addr, tx_buf, 2, 500);
    if (status != HAL_OK)
    {
        printf("HAL_I2C_Master_Transmit failed for 0x%02x [ret: %d]\n", addr, status);
        printf("HAL I2C Error code: 0x%08x\n", hi2c3.ErrorCode);
    }
    return status;
}

/*
 * Write a list of register settings; ff/ff stops the process.
 */
static int ov7675_write_array(struct regval_list *vals)
{
    uint8_t reg_val;
	while (vals->reg_num != 0xff || vals->value != 0xff) {
		int ret = ov7675_write_reg(vals->reg_num, vals->value);
        if ( (vals->reg_num == REG_COM7) && (vals->value & COM7_RESET) )
        {
            HAL_Delay(10);
        } 
        // reg_val = ov7675_read_reg(vals->reg_num);
        // printf("0x%02x = 0x%02x", vals->reg_num, reg_val);
        // if (reg_val != vals->value)
        // {
        //     printf(", expected: 0x%02x\n", vals->value);
        // }
        // else
        // {
        //     printf("\n");
        // }

		if (ret != 0)
			return ret;
		vals++;
	}
	return 0;
}

int ov7675_set_window(uint16_t hstart, uint16_t hstop, uint16_t vstart, uint16_t vstop)
{
    printf("requested settings: hstart: %d, hstop: %d, vstart: %d, vstop: %d\n", hstart, hstop, vstart, vstop);
    // Horizontal frame start {HSTART[7:0], HREF[2:0]}
    // Horizontal frame end {HSTOP[7:0], HREF[5:3]}
    // Vertical frame start {VSTART[7:0], VREF[1:0]}
    // Vertical frame end REG_VSTOP[7:0], VREF[3:2]}
    uint8_t reg_hstart, reg_hstop, reg_href;
    uint8_t reg_vstart, reg_vstop, reg_vref;
    uint8_t reg_tmp;
    int ret = 0;

    reg_hstart = reg_hstop = reg_href = 0;
    reg_vstart = reg_vstop = reg_vref = 0;

    // Horizontal bounds
    reg_hstart = (hstart >> 2) & 0xff;
    reg_hstop = (hstop >> 2) & 0xff;
    reg_href = ((hstop & 0x3) << 3) | (hstart & 0x3);
    
    reg_tmp = ov7675_read_reg(REG_HREF);
    printf("writing HSTART: 0x%02x\n", reg_hstart);
    ov7675_write_reg(REG_HSTART, reg_hstart);
    printf("writing HSTOP: 0x%02x\n", reg_hstop);
    ov7675_write_reg(REG_HSTOP, reg_hstop);
    printf("writing HREF: 0x%02x\n", reg_tmp | reg_href);
    ov7675_write_reg(REG_HREF, reg_tmp | reg_href);

    // Vertical bounds
    reg_vstart = (vstart >> 2) & 0xff;
    reg_vstop = (vstop >> 2) & 0xff;
    reg_vref = ((vstop & 0x3) << 2) | (vstart & 0x3);
    
    reg_tmp = ov7675_read_reg(REG_VREF);
    printf("writing VSTART: 0x%02x\n", reg_vstart);
    ov7675_write_reg(REG_VSTART, reg_vstart);
    printf("writing VSTOP: 0x%02x\n", reg_vstop);
    ov7675_write_reg(REG_VSTOP, reg_vstop);
    printf("writing VREF: 0x%02x\n", reg_tmp | reg_vref);
    ov7675_write_reg(REG_VREF, reg_tmp | reg_vref);
    printf("===========================\n");

    return ret;
}

int ov7675_set_test_pattern(int onoff)
{
    int ret = 0;
    uint8_t regval;

    regval = ov7675_read_reg(REG_COM17);
    if (onoff)
    {
        regval |= COM17_CBAR;
    }
    else
    {
        regval &= ~COM17_CBAR;
    }
    ret = ov7675_write_reg(REG_COM17, regval);
    return ret;
}



int ov7675_init()
{
    // SH1106_init();
    int ret = 0;
    uint8_t val;
    uint8_t reg_hstart, reg_hstop, reg_href;
    uint8_t reg_vstart, reg_vstop, reg_vref; 
    uint16_t hstart, hstop, vstart, vstop;  
    printf("=========================\n");

    // PEN = 1, PDN = `0
    HAL_GPIO_WritePin(PEN_PDN_GPIO_Port, PEN_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(PEN_PDN_GPIO_Port, PDN_Pin, GPIO_PIN_SET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(PEN_PDN_GPIO_Port, PDN_Pin, GPIO_PIN_RESET);
    HAL_Delay(20);


    // Check if powerup reads are working
    val = ov7675_read_reg(REG_MIDH); // 0x7f
    printf("MIDH: 0x%02x\r\n", val); 

    val = ov7675_read_reg(REG_MIDL); // 0xa2
    printf("MIDL: 0x%02x\r\n", val); 

    val = ov7675_read_reg(REG_PID); // 0x76
    printf("PID: 0x%02x\r\n", val);

    val = ov7675_read_reg(REG_VER); // 0x73
    printf("VER: 0x%02x\r\n", val);

    printf("Writing default configurations\n");
    ov7675_write_array(ov7670_default_regs);

    printf("Setting RGB565 format\n");
    ov7675_write_array(ov7670_fmt_rgb565);
    printf("Done\n");


    // ov7675_set_window(7, 647, 14, 494); // 640 x 480
    // ov7675_set_window(68, 388, 132, 372); // 320 x 240
    ov7675_set_window(68, 388, 14, 494); // 
    HAL_Delay(10);

    ov7675_write_reg(REG_DBLV, DBLV_X6);
    ov7675_write_reg(REG_CLKRC, 0x03); // internal clock = xclk / (CLKRC[5:0] + 1)
    ov7675_write_reg(REG_COM14, 0x11); // plclk / COM14[2:0]
    // ov7675_write_reg(REG_CLKRC, 0x00); // internal clock = xclk / (CLKRC[5:0] + 1)
    // ov7675_write_reg(REG_COM14, 0x00); // plclk / COM14[2:0]

    val = ov7675_read_reg(REG_CLKRC); 
    printf("CLKRC: 0x%02x\r\n", val);

    val = ov7675_read_reg(REG_COM14); 
    printf("COM14: 0x%02x\r\n", val);

    val = ov7675_read_reg(REG_DBLV); 
    printf("DBLV: 0x%02x\r\n", val);

    val = ov7675_read_reg(REG_TSLB); 
    printf("TSLB: 0x%02x\r\n", val);


    // Set the format
    // val = ov7675_read_reg(REG_COM7);
    // printf("COM7: 0x%02x\n", val);
    // val = val | (1 << 4); // COM7[4] = 1 for QVGA output
    // ov7675_write_reg(REG_COM7, val);
    
    ov7675_set_test_pattern(0);

    ov7675_write_reg(REG_PSHFT, 0);

    


    // Horizontal frame start {HSTART[7:0], HREF[2:0]}
    // Horizontal frame end {HSTOP[7:0], HREF[5:3]}
    reg_hstart = ov7675_read_reg(REG_HSTART); 
    printf("HSTART: 0x%02x\r\n", reg_hstart);
    reg_href  = ov7675_read_reg(REG_HREF); 
    printf("HREF: 0x%02x\r\n", reg_href);
    reg_hstop = ov7675_read_reg(REG_HSTOP); 
    printf("HSTOP: 0x%02x\r\n", reg_hstop);

    hstart = 0; hstop = 0;
    hstart = (reg_hstart << 2) | (reg_href & 0x3);
    hstop = (reg_hstop << 2) | ( (reg_href >> 3) & 0x3 );
    printf("hstart: %d pix, hstop: %d pix \n", hstart, hstop);
 
 
    // Vertical frame start {VSTART[7:0], VREF[1:0]}
    // Vertical frame end {REG_VSTOP[7:0], VREF[3:2]}
    reg_vstart = ov7675_read_reg(REG_VSTART); 
    printf("VSTART: 0x%02x\r\n", reg_vstart);
    reg_vref = ov7675_read_reg(REG_VREF); 
    printf("VREF: 0x%02x\r\n", reg_vref);
    reg_vstop = ov7675_read_reg(REG_VSTOP); 
    printf("VSTOP: 0x%02x\r\n", reg_vstop);

    vstart = 0; vstop = 0;
    vstart = (reg_vstart << 2) | (reg_vref & 0x3);
    vstop = (reg_vstop << 2) | ((reg_vref >> 2) & 0x03);
    printf("vstart: %d pix, vstop: %d pix \n", vstart, vstop);

    val = ov7675_read_reg(REG_COM7);
    printf("COM7: 0x%02x\n", val);

    



    return ret;
}



int ov7675_grab_frame(uint16_t* frame)
{
    uint32_t vsync, hsync;
    uint32_t idr;
    int done = 0;
    int irow, icol;

    uint16_t pixel;
    uint8_t *bytes = (uint8_t*) &pixel;

    uint32_t tick_start, tick_stop;
    uint32_t write_ptr = 0;


    // 1. wait for vsync to go high
    while(!(GPIOD->IDR & CAM_VSYNC_Pin));

    // 2. wait for vsync to go low
    while((GPIOD->IDR & CAM_VSYNC_Pin));
    for (irow = 0; irow < 480; irow++)
    {
        // 3. wait for hsync to go high
        while(!(GPIOD->IDR & CAM_HSYNC_Pin));
        GPIOG->ODR |= GPIO_PIN_13;

        // 4. read data pins on positive edge of pclk 2 * width times.
        write_ptr = 0;
        for (icol = 0; icol < 320; icol++)
        {
            
            // wait for pclk to be high
            while(!(PCLK_GPIO_Port->IDR & PCLK_Pin));
            // sample the parallel data

            /**
             * DATA[4:0] = GPIOE->IDR[6:2]
             * DATA[6:5] = GPIOB->IDR[4:3]
             * DATA[7] = GPIOB->IDR[7] 
             */
            bytes[0] = (GPIOE->IDR >> 2) & 0x1f;
            bytes[0] |= (GPIOB->IDR << 2) & 0x60;
            bytes[0] |= (GPIOB->IDR & 0x80); 

            // wait for pclk to be low
            while((PCLK_GPIO_Port->IDR & PCLK_Pin));

            // Sample the 2nd byte

            // wait for pclk to be high
            while(!(PCLK_GPIO_Port->IDR & PCLK_Pin));
            bytes[1] = (GPIOE->IDR >> 2) & 0x1f;
            bytes[1] |= (GPIOB->IDR << 2) & 0x60;
            bytes[1] |= (GPIOB->IDR & 0x80);
            while((PCLK_GPIO_Port->IDR & PCLK_Pin));
            
            scratch_buffer[write_ptr++] = pixel;
            
        }
        
        // 5. wait for hsync to go low
        while((GPIOD->IDR & CAM_HSYNC_Pin));

        GPIOG->ODR &= ~GPIO_PIN_13;
        for (int i = 0; i < 320; i++)
        {
            frame[(irow >> 1) + (240 * i)] = scratch_buffer[i];
        }


    }

}