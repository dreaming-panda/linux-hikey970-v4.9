/*
 * Copyright (c) 2016-2017 Linaro Ltd.
 * Copyright (c) 2016-2017 HiSilicon Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include "ov5645.h"

#define OV5645_SYSTEM_CTRL0		0x3008
#define OV5645_SYSTEM_CTRL0_START	0x02
#define OV5645_SYSTEM_CTRL0_STOP	0x42
#define OV5645_CHIP_ID_HIGH_REG		0x300A
#define OV5645_CHIP_ID_HIGH		0x56
#define OV5645_CHIP_ID_LOW_REG		0x300B
#define OV5645_CHIP_ID_LOW		0x45
#define OV5645_AWB_MANUAL_CONTROL	0x3406
#define OV5645_AWB_MANUAL_ENABLE	(1 << 0)
#define OV5645_AEC_PK_MANUAL		0x3503
#define OV5645_AEC_MANUAL_ENABLE	(1 << 0)
#define OV5645_AGC_MANUAL_ENABLE	(1 << 1)
#define OV5645_TIMING_TC_REG20		0x3820
#define OV5645_SENSOR_VFLIP		(1 << 1)
#define OV5645_ISP_VFLIP		(1 << 2)
#define OV5645_TIMING_TC_REG21		0x3821
#define OV5645_SENSOR_MIRROR		(1 << 1)
#define OV5645_PRE_ISP_TEST_SETTING_1	0x503d

#define OV5645_SDE_SAT_U		0x5583
#define OV5645_SDE_SAT_V		0x5584

enum ov5645_mode {
	ov5645_mode_min = 0,
	ov5645_mode_sxga_1280_960 = 0,
	ov5645_mode_1080p_1920_1080 = 1,
	ov5645_mode_full_2592_1944 = 2,
	ov5645_mode_max = 2,
};

struct reg_value {
	u16 reg;
	u8 val;
};

struct ov5645_mode_info {
	enum ov5645_mode mode;
	u32 width;
	u32 height;
	struct reg_value *init_data_ptr;
	u32 init_data_size;
};

static struct reg_value ov5645_global_init_setting[] = {
	{0x3103, 0x11,},
	{0x3008, 0x82,},
	{0x3008, 0x42,},
	{0x3103, 0x03,},
	{0x3503, 0x07,},
	{0x3002, 0x1c,},
	{0x3006, 0xc3,},
	{0x300e, 0x45,},
	{0x3017, 0x00,},
	{0x3018, 0x00,},
	{0x302e, 0x0b,},
	{0x3037, 0x13,},
	{0x3108, 0x01,},
	{0x3611, 0x06,},
	{0x3500, 0x00,},
	{0x3501, 0x01,},
	{0x3502, 0x00,},
	{0x350a, 0x00,},
	{0x350b, 0x3f,},
	{0x3620, 0x33,},
	{0x3621, 0xe0,},
	{0x3622, 0x01,},
	{0x3630, 0x2e,},
	{0x3631, 0x00,},
	{0x3632, 0x32,},
	{0x3633, 0x52,},
	{0x3634, 0x70,},
	{0x3635, 0x13,},
	{0x3636, 0x03,},
	{0x3703, 0x5a,},
	{0x3704, 0xa0,},
	{0x3705, 0x1a,},
	{0x3709, 0x12,},
	{0x370b, 0x61,},
	{0x370f, 0x10,},
	{0x3715, 0x78,},
	{0x3717, 0x01,},
	{0x371b, 0x20,},
	{0x3731, 0x12,},
	{0x3901, 0x0a,},
	{0x3905, 0x02,},
	{0x3906, 0x10,},
	{0x3719, 0x86,},
	{0x3810, 0x00,},
	{0x3811, 0x10,},
	{0x3812, 0x00,},
	{0x3821, 0x01,},
	{0x3824, 0x01,},
	{0x3826, 0x03,},
	{0x3828, 0x08,},
	{0x3a19, 0xf8,},
	{0x3c01, 0x34,},
	{0x3c04, 0x28,},
	{0x3c05, 0x98,},
	{0x3c07, 0x07,},
	{0x3c09, 0xc2,},
	{0x3c0a, 0x9c,},
	{0x3c0b, 0x40,},
	{0x3c01, 0x34,},
	{0x4001, 0x02,},
	{0x4514, 0x00,},
	{0x4520, 0xb0,},
	{0x460b, 0x37,},
	{0x460c, 0x20,},
	{0x4818, 0x01,},
	{0x481d, 0xf0,},
	{0x481f, 0x50,},
	{0x4823, 0x70,},
	{0x4831, 0x14,},
	{0x5000, 0xa7,},
	{0x5001, 0x83,},
	{0x501d, 0x00,},
	{0x501f, 0x00,},
	{0x503d, 0x00,},
	{0x505c, 0x30,},
	{0x5181, 0x59,},
	{0x5183, 0x00,},
	{0x5191, 0xf0,},
	{0x5192, 0x03,},
	{0x5684, 0x10,},
	{0x5685, 0xa0,},
	{0x5686, 0x0c,},
	{0x5687, 0x78,},
	{0x5a00, 0x08,},
	{0x5a21, 0x00,},
	{0x5a24, 0x00,},
	{0x3008, 0x02,},
	{0x3503, 0x00,},
	{0x5180, 0xff,},
	{0x5181, 0xf2,},
	{0x5182, 0x00,},
	{0x5183, 0x14,},
	{0x5184, 0x25,},
	{0x5185, 0x24,},
	{0x5186, 0x09,},
	{0x5187, 0x09,},
	{0x5188, 0x0a,},
	{0x5189, 0x75,},
	{0x518a, 0x52,},
	{0x518b, 0xea,},
	{0x518c, 0xa8,},
	{0x518d, 0x42,},
	{0x518e, 0x38,},
	{0x518f, 0x56,},
	{0x5190, 0x42,},
	{0x5191, 0xf8,},
	{0x5192, 0x04,},
	{0x5193, 0x70,},
	{0x5194, 0xf0,},
	{0x5195, 0xf0,},
	{0x5196, 0x03,},
	{0x5197, 0x01,},
	{0x5198, 0x04,},
	{0x5199, 0x12,},
	{0x519a, 0x04,},
	{0x519b, 0x00,},
	{0x519c, 0x06,},
	{0x519d, 0x82,},
	{0x519e, 0x38,},
	{0x5381, 0x1e,},
	{0x5382, 0x5b,},
	{0x5383, 0x08,},
	{0x5384, 0x0a,},
	{0x5385, 0x7e,},
	{0x5386, 0x88,},
	{0x5387, 0x7c,},
	{0x5388, 0x6c,},
	{0x5389, 0x10,},
	{0x538a, 0x01,},
	{0x538b, 0x98,},
	{0x5300, 0x08,},
	{0x5301, 0x30,},
	{0x5302, 0x10,},
	{0x5303, 0x00,},
	{0x5304, 0x08,},
	{0x5305, 0x30,},
	{0x5306, 0x08,},
	{0x5307, 0x16,},
	{0x5309, 0x08,},
	{0x530a, 0x30,},
	{0x530b, 0x04,},
	{0x530c, 0x06,},
	{0x5480, 0x01,},
	{0x5481, 0x08,},
	{0x5482, 0x14,},
	{0x5483, 0x28,},
	{0x5484, 0x51,},
	{0x5485, 0x65,},
	{0x5486, 0x71,},
	{0x5487, 0x7d,},
	{0x5488, 0x87,},
	{0x5489, 0x91,},
	{0x548a, 0x9a,},
	{0x548b, 0xaa,},
	{0x548c, 0xb8,},
	{0x548d, 0xcd,},
	{0x548e, 0xdd,},
	{0x548f, 0xea,},
	{0x5490, 0x1d,},
	{0x5580, 0x02,},
	{0x5583, 0x40,},
	{0x5584, 0x10,},
	{0x5589, 0x10,},
	{0x558a, 0x00,},
	{0x558b, 0xf8,},
	{0x5800, 0x3f,},
	{0x5801, 0x16,},
	{0x5802, 0x0e,},
	{0x5803, 0x0d,},
	{0x5804, 0x17,},
	{0x5805, 0x3f,},
	{0x5806, 0x0b,},
	{0x5807, 0x06,},
	{0x5808, 0x04,},
	{0x5809, 0x04,},
	{0x580a, 0x06,},
	{0x580b, 0x0b,},
	{0x580c, 0x09,},
	{0x580d, 0x03,},
	{0x580e, 0x00,},
	{0x580f, 0x00,},
	{0x5810, 0x03,},
	{0x5811, 0x08,},
	{0x5812, 0x0a,},
	{0x5813, 0x03,},
	{0x5814, 0x00,},
	{0x5815, 0x00,},
	{0x5816, 0x04,},
	{0x5817, 0x09,},
	{0x5818, 0x0f,},
	{0x5819, 0x08,},
	{0x581a, 0x06,},
	{0x581b, 0x06,},
	{0x581c, 0x08,},
	{0x581d, 0x0c,},
	{0x581e, 0x3f,},
	{0x581f, 0x1e,},
	{0x5820, 0x12,},
	{0x5821, 0x13,},
	{0x5822, 0x21,},
	{0x5823, 0x3f,},
	{0x5824, 0x68,},
	{0x5825, 0x28,},
	{0x5826, 0x2c,},
	{0x5827, 0x28,},
	{0x5828, 0x08,},
	{0x5829, 0x48,},
	{0x582a, 0x64,},
	{0x582b, 0x62,},
	{0x582c, 0x64,},
	{0x582d, 0x28,},
	{0x582e, 0x46,},
	{0x582f, 0x62,},
	{0x5830, 0x60,},
	{0x5831, 0x62,},
	{0x5832, 0x26,},
	{0x5833, 0x48,},
	{0x5834, 0x66,},
	{0x5835, 0x44,},
	{0x5836, 0x64,},
	{0x5837, 0x28,},
	{0x5838, 0x66,},
	{0x5839, 0x48,},
	{0x583a, 0x2c,},
	{0x583b, 0x28,},
	{0x583c, 0x26,},
	{0x583d, 0xae,},
	{0x5025, 0x00,},
	{0x3a0f, 0x30,},
	{0x3a10, 0x28,},
	{0x3a1b, 0x30,},
	{0x3a1e, 0x26,},
	{0x3a11, 0x60,},
	{0x3a1f, 0x14,},
	{0x0601, 0x02,},
	{0x3008, 0x42,},
	{0x3008, 0x02},
};

static struct reg_value ov5645_setting_sxga[] = {
	{0x3612, 0xa9,},
	{0x3614, 0x50,},
	{0x3618, 0x00,},
	{0x3034, 0x18,},
	{0x3035, 0x21,},
	{0x3036, 0x70,},
	{0x3600, 0x09,},
	{0x3601, 0x43,},
	{0x3708, 0x66,},
	{0x370c, 0xc3,},
	{0x3800, 0x00,},
	{0x3801, 0x00,},
	{0x3802, 0x00,},
	{0x3803, 0x06,},
	{0x3804, 0x0a,},
	{0x3805, 0x3f,},
	{0x3806, 0x07,},
	{0x3807, 0x9d,},
	{0x3808, 0x05,},
	{0x3809, 0x00,},
	{0x380a, 0x03,},
	{0x380b, 0xc0,},
	{0x380c, 0x07,},
	{0x380d, 0x68,},
	{0x380e, 0x03,},
	{0x380f, 0xd8,},
	{0x3813, 0x06,},
	{0x3814, 0x31,},
	{0x3815, 0x31,},
	{0x3820, 0x47,},
	{0x3a02, 0x03,},
	{0x3a03, 0xd8,},
	{0x3a08, 0x01,},
	{0x3a09, 0xf8,},
	{0x3a0a, 0x01,},
	{0x3a0b, 0xa4,},
	{0x3a0e, 0x02,},
	{0x3a0d, 0x02,},
	{0x3a14, 0x03,},
	{0x3a15, 0xd8,},
	{0x3a18, 0x00,},
	{0x4004, 0x02,},
	{0x4005, 0x18,},
	{0x4300, 0x32,},
	{0x4202, 0x00,},
};

static struct reg_value ov5645_setting_1080P[] = {
	{0x3612, 0xab,},
	{0x3614, 0x50,},
	{0x3618, 0x04,},
	{0x3034, 0x18,},
	{0x3035, 0x11,},
	{0x3036, 0x54,},
	{0x3600, 0x08,},
	{0x3601, 0x33,},
	{0x3708, 0x63,},
	{0x370c, 0xc0,},
	{0x3800, 0x01,},
	{0x3801, 0x50,},
	{0x3802, 0x01,},
	{0x3803, 0xb2,},
	{0x3804, 0x08,},
	{0x3805, 0xef,},
	{0x3806, 0x05,},
	{0x3807, 0xf1,},
	{0x3808, 0x07,},
	{0x3809, 0x80,},
	{0x380a, 0x04,},
	{0x380b, 0x38,},
	{0x380c, 0x09,},
	{0x380d, 0xc4,},
	{0x380e, 0x04,},
	{0x380f, 0x60,},
	{0x3813, 0x04,},
	{0x3814, 0x11,},
	{0x3815, 0x11,},
	{0x3820, 0x47,},
	{0x4514, 0x88,},
	{0x3a02, 0x04,},
	{0x3a03, 0x60,},
	{0x3a08, 0x01,},
	{0x3a09, 0x50,},
	{0x3a0a, 0x01,},
	{0x3a0b, 0x18,},
	{0x3a0e, 0x03,},
	{0x3a0d, 0x04,},
	{0x3a14, 0x04,},
	{0x3a15, 0x60,},
	{0x3a18, 0x00,},
	{0x4004, 0x06,},
	{0x4005, 0x18,},
	{0x4300, 0x32,},
	{0x4202, 0x00,},
	{0x4837, 0x0b,},
};

static struct reg_value ov5645_setting_full[] = {
	{0x3612, 0xab,},
	{0x3614, 0x50,},
	{0x3618, 0x04,},
	{0x3034, 0x18,},
	{0x3035, 0x11,},
	{0x3036, 0x54,},
	{0x3600, 0x08,},
	{0x3601, 0x33,},
	{0x3708, 0x63,},
	{0x370c, 0xc0,},
	{0x3800, 0x00,},
	{0x3801, 0x00,},
	{0x3802, 0x00,},
	{0x3803, 0x00,},
	{0x3804, 0x0a,},
	{0x3805, 0x3f,},
	{0x3806, 0x07,},
	{0x3807, 0x9f,},
	{0x3808, 0x0a,},
	{0x3809, 0x20,},
	{0x380a, 0x07,},
	{0x380b, 0x98,},
	{0x380c, 0x0b,},
	{0x380d, 0x1c,},
	{0x380e, 0x07,},
	{0x380f, 0xb0,},
	{0x3813, 0x06,},
	{0x3814, 0x11,},
	{0x3815, 0x11,},
	{0x3820, 0x47,},
	{0x4514, 0x88,},
	{0x3a02, 0x07,},
	{0x3a03, 0xb0,},
	{0x3a08, 0x01,},
	{0x3a09, 0x27,},
	{0x3a0a, 0x00,},
	{0x3a0b, 0xf6,},
	{0x3a0e, 0x06,},
	{0x3a0d, 0x08,},
	{0x3a14, 0x07,},
	{0x3a15, 0xb0,},
	{0x3a18, 0x01,},
	{0x4004, 0x06,},
	{0x4005, 0x18,},
	{0x4300, 0x32,},
	{0x4837, 0x0b,},
	{0x4202, 0x00,},
};

static struct ov5645_mode_info ov5645_mode_info_data[ov5645_mode_max + 1] = {
	{ov5645_mode_sxga_1280_960, 1280, 960,
	 ov5645_setting_sxga,
	 ARRAY_SIZE(ov5645_setting_sxga)},
	{ov5645_mode_1080p_1920_1080, 1920, 1080,
	 ov5645_setting_1080P,
	 ARRAY_SIZE(ov5645_setting_1080P)},
	{ov5645_mode_full_2592_1944, 2592, 1944,
	 ov5645_setting_full,
	 ARRAY_SIZE(ov5645_setting_full)},
};

static int ov5645_regulators_enable(struct isp_device *dev)
{
	int ret = 0;

	if (dev->io_regulator) {
		ret = regulator_enable(dev->io_regulator);
		if (ret < 0) {
			ISP_ERR("set io voltage failed\n");
			return ret;
		}
	}

	return ret;
}

static void ov5645_regulators_disable(struct isp_device *dev)
{
	if (dev->io_regulator)
		regulator_disable(dev->io_regulator);
}

void __ov5645_set_power(struct isp_device *dev, bool on)
{
	if (on) {
		clk_prepare_enable(dev->xclk);
		ov5645_regulators_enable(dev);
		usleep_range(5000, 15000);
		if (dev->pwdn_gpio)
			gpio_set_value(dev->pwdn_gpio, 1);
		usleep_range(1000, 2000);
		if (dev->rst_gpio)
			gpio_set_value(dev->rst_gpio, 1);
		msleep(20);
		ISP_ERR("enter\n");
	} else {
		if (dev->rst_gpio)
			gpio_set_value(dev->rst_gpio, 0);
		if (dev->pwdn_gpio)
			gpio_set_value(dev->pwdn_gpio, 0);
		ov5645_regulators_disable(dev);
		clk_disable_unprepare(dev->xclk);
	}
}

static enum ov5645_mode ov5645_find_nearest_mode(int width, int height)
{
	int i;

	for (i = ov5645_mode_max; i >= 0; i--) {
		if (ov5645_mode_info_data[i].width <= width &&
		    ov5645_mode_info_data[i].height <= height)
			break;
	}

	if (i < 0)
		i = 0;

	return (enum ov5645_mode)i;
}

static int ov5645_set_register_array(struct isp_i2c_client_t *client,
				     struct reg_value *settings,
				     s32 num_settings)
{
	register u16 reg = 0;
	register u8 val = 0;
	int i, ret = 0;

	for (i = 0; i < num_settings; ++i, ++settings) {
		reg = settings->reg;
		val = settings->val;

		ret = client->ops->write(client, reg, val);
		if (ret < 0)
			goto err;
	}
err:
	return ret;
}

static int ov5645_init(struct isp_i2c_client_t *client)
{
	struct reg_value *settings = NULL;
	int num_settings = 0;
	int ret;

	settings = ov5645_global_init_setting;
	num_settings = ARRAY_SIZE(ov5645_global_init_setting);
	ret = ov5645_set_register_array(client, settings, num_settings);
	if (ret < 0)
		return ret;

	return 0;
}

static int
ov5645_change_mode(struct isp_i2c_client_t *client, enum ov5645_mode mode)
{
	struct reg_value *settings = NULL;
	s32 num_settings = 0;
	int ret = 0;

	settings = ov5645_mode_info_data[mode].init_data_ptr;
	num_settings = ov5645_mode_info_data[mode].init_data_size;
	ret = ov5645_set_register_array(client, settings, num_settings);

	return ret;
}

static int ov5645_set_aec_mode(struct isp_i2c_client_t *client, int auto_mode)
{
	u8 val;

	client->ops->read(client, OV5645_AEC_PK_MANUAL, &val);
	if (auto_mode) {
		val |= OV5645_AEC_MANUAL_ENABLE;
	} else { /* V4L2_EXPOSURE_MANUAL */
		val &= ~OV5645_AEC_MANUAL_ENABLE;
	}

	return client->ops->write(client, OV5645_AEC_PK_MANUAL, val);
}

static int ov5645_set_awb(struct isp_i2c_client_t *client, s32 enable_auto)
{
	u8 val;

	client->ops->read(client, OV5645_AWB_MANUAL_CONTROL, &val);
	if (enable_auto)
		val &= ~OV5645_AWB_MANUAL_ENABLE;
	else
		val |= OV5645_AWB_MANUAL_ENABLE;

	return client->ops->write(client, OV5645_AWB_MANUAL_CONTROL, val);
}

int ov5645_set_fmt(struct isp_i2c_client_t *client,
		unsigned int w, unsigned int h)
{
	enum ov5645_mode new_mode;

	new_mode = ov5645_find_nearest_mode(w, h);

	client->mode = new_mode;

	return 0;
}

int ov5645_get_chip_id(struct isp_i2c_client_t *client, unsigned int *chip_id)
{
	int ret;
	unsigned char chip_id_high, chip_id_low;

	ret = client->ops->read(client, OV5645_CHIP_ID_HIGH_REG, &chip_id_high);
	if (ret || chip_id_high != OV5645_CHIP_ID_HIGH) {
		ISP_ERR("get chip id high[%d] fail\n", chip_id_high);
		return -ENODEV;
	}

	ret = client->ops->read(client, OV5645_CHIP_ID_LOW_REG, &chip_id_low);
	if (ret || chip_id_low != OV5645_CHIP_ID_LOW) {
		ISP_ERR("get chip id low[%d] fail\n", chip_id_low);
		return -ENODEV;
	}

	*chip_id = (chip_id_high<<8) | chip_id_low;

	return 0;
}

int ov5645_stream_on_off(struct isp_i2c_client_t *client, int enable)
{
	int ret;

	ISP_ERR("enable = %d mode = %d\n", enable, client->mode);

	if (enable) {
		ret = ov5645_change_mode(client, client->mode);
		if (ret < 0) {
			ISP_ERR("could not set mode %d\n",
				client->mode);
			return ret;
		}

		ov5645_set_aec_mode(client, 0);
		ov5645_set_awb(client, 1);

		client->ops->write(client, OV5645_SYSTEM_CTRL0,
				 OV5645_SYSTEM_CTRL0_START);
	} else {
		client->ops->write(client, OV5645_SYSTEM_CTRL0,
				 OV5645_SYSTEM_CTRL0_STOP);
	}

	return 0;
}

int ov5645_power(struct isp_i2c_client_t *client, int on)
{
	int ret;

	__ov5645_set_power(client->isp_dev, !!on);

	if (on) {
		ret = ov5645_init(client);
		if (ret < 0) {
			ISP_ERR("could not set init registers\n");
			return -1;
		}

		client->ops->write(client, OV5645_SYSTEM_CTRL0,
				 OV5645_SYSTEM_CTRL0_STOP);
	}

	return 0;
}

static int ov5645_probe(struct platform_device *pdev)
{
	int ret;
	struct isp_device *idev = NULL;
	struct device *device = &pdev->dev;
	struct device_node *np = device->of_node;
	int index = 0;
	unsigned int chip_id = 0;
	struct isp_i2c_client_t *i2c_client = NULL;

	ISP_INFO("enter!\n");
	if (pdev->dev.parent)
		idev = dev_get_drvdata(pdev->dev.parent);

	if (!idev)
		return -ENXIO;

	ret = of_property_read_u32(np, "sensor_index", (u32 *)&index);
	if (ret < 0) {
		ISP_ERR("no sensor index available\n");
		return -ENXIO;
	}

	if (idev->sensor[index].probe == true)
		return 0;

	idev->sensor[index].rst_gpio = of_get_named_gpio(np, "reset-gpio", 0);
	if (!gpio_is_valid(idev->sensor[index].rst_gpio)) {
		ISP_ERR("no reset pin available\n");
		idev->sensor[index].rst_gpio = 0;
		return -ENXIO;
	}

	idev->sensor[index].pwdn_gpio = of_get_named_gpio(np, "pwdn-gpio", 0);
	if (!gpio_is_valid(idev->sensor[index].rst_gpio)) {
		ISP_ERR("no powerdown pin available\n");
		idev->sensor[index].rst_gpio = 0;
		return -ENXIO;
	}

	ret = devm_gpio_request_one(&pdev->dev,
			idev->sensor[index].rst_gpio,
			GPIOF_OUT_INIT_LOW, "sensor-reset");
	if (ret < 0)
		return ret;

	ret = devm_gpio_request_one(&pdev->dev,
			idev->sensor[index].pwdn_gpio,
			GPIOF_OUT_INIT_LOW, "sensor-pwdn");
	if (ret < 0)
		return ret;

	idev->sensor[index].addr = OV5645_SLAVE_ID;
	idev->sensor[index].flag = I2C_ADDR_7BIT;
	idev->sensor[index].speed_cfg = I2C_SPEED_STD;
	idev->sensor[index].csi_index = 0;
	idev->sensor[index].dt = YUV_DT_422_8BITS;
	idev->sensor[index].pf = DF_2PF8;
	idev->sensor[index].csi_lane = 2;

	idev->addr = idev->sensor[index].addr;
	idev->flag = idev->sensor[index].flag;
	idev->speed_cfg = idev->sensor[index].speed_cfg;
	idev->csi_index = idev->sensor[index].csi_index;
	idev->dt = idev->sensor[index].dt;
	idev->pf = idev->sensor[index].pf;
	idev->csi_lane = idev->sensor[index].csi_lane;
	idev->pwdn_gpio = idev->sensor[index].pwdn_gpio;
	idev->rst_gpio = idev->sensor[index].rst_gpio;

	isp_ispss_enable_clock(idev);

	mdelay(100);

	isp_ispss_reset_all(idev);

	mdelay(100);

	idev->client = create_isp_i2c_client(idev);
	if (!idev->client)
		return -ENODEV;

	i2c_client = idev->client;
	ret = i2c_client->ops->open(i2c_client);
	if (ret) {
		destroy_isp_i2c_client(i2c_client);
		return -EBUSY;
	}

	ov5645_power(i2c_client, 1);
	ov5645_get_chip_id(i2c_client, &chip_id);
	ov5645_power(i2c_client, 0);
	i2c_client->ops->close(i2c_client);
	destroy_isp_i2c_client(i2c_client);
	ISP_ERR("chip id is 0x%x\n", chip_id);
	if (chip_id != 0x5645) {
		ISP_ERR("chip id is 0x%x, probe failed\n", chip_id);
		return -1;
	}

	idev->sensor[index].probe = true;
	idev->sensor[index].camera_sensor_power = ov5645_power;
	idev->sensor[index].camera_sensor_set_fmt = ov5645_set_fmt;
	idev->sensor[index].camera_stream_on_off = ov5645_stream_on_off;

	ISP_INFO("exit!\n");

	return 0;
}

static int ov5645_remove(struct platform_device *pdev)
{
	ISP_INFO("enter!\n");


	ISP_INFO("exit!\n");
	return 0;
}

static const struct of_device_id ov5645_match_table[] = {
	{
		.compatible = "hisilicon,ov5645",
		.data = NULL,
	},
	{},
};
MODULE_DEVICE_TABLE(of, ov5645_match_table);

static struct platform_driver ov5645_driver = {
	.probe = ov5645_probe,
	.remove = ov5645_remove,
	.driver = {
		.name = "hisi-ov5645",
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(ov5645_match_table),
	},
};

static int __init ov5645_module_init(void)
{
	int ret = 0;

	ISP_INFO("enter!\n");

	ret = platform_driver_register(&ov5645_driver);
	if (ret)
		return ret;

	ISP_INFO("exit!\n");

	return ret;
}

static void __exit ov5645_module_exit(void)
{
	ISP_INFO("enter!\n");

	platform_driver_unregister(&ov5645_driver);

	ISP_INFO("exit!\n");
}

late_initcall(ov5645_module_init);
module_exit(ov5645_module_exit);

MODULE_DESCRIPTION("Hisilicon Isp Driver");
MODULE_LICENSE("GPL v2");

