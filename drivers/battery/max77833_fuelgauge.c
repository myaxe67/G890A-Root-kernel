/*
 *  max77833_fuelgauge.c
 *  Samsung MAX77833 Fuel Gauge Driver
 *
 *  Copyright (C) 2012 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define DEBUG
/* #define BATTERY_LOG_MESSAGE */

#include <linux/mfd/max77833-private.h>
#include <linux/of_gpio.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>

static enum power_supply_property max77833_fuelgauge_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_AVG,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CURRENT_AVG,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_ENERGY_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_TEMP_AMBIENT,
#if defined(CONFIG_EN_OOPS)
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
#endif
	POWER_SUPPLY_PROP_ENERGY_FULL,
	POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN,
	POWER_SUPPLY_PROP_TIME_TO_FULL_NOW,
	POWER_SUPPLY_PROP_POWER_NOW,
	POWER_SUPPLY_PROP_POWER_AVG,
#if defined(CONFIG_BATTERY_AGE_FORECAST)
	POWER_SUPPLY_PROP_CAPACITY_LEVEL,
#endif
};

bool max77833_fg_fuelalert_init(struct max77833_fuelgauge_data *fuelgauge, int soc, int vol);

#if !defined(CONFIG_SEC_FACTORY)
static void max77833_fg_read_time(struct max77833_fuelgauge_data *fuelgauge)
{
	u16 data;
	u8 test_data[2];
	int lsb, msb, time;

	if (max77833_read_fg(fuelgauge->i2c,
			     TIME_TO_FULL_REG, &data) < 0) {
		pr_err("%s: Failed to read TTF\n", __func__);
		return;
	}

	test_data[0] = data & 0xFF;
	test_data[1] = (data >> 8);
	lsb = test_data[0] & 0x1f;
	msb = ((test_data[1] << 3) + ((test_data[0] & 0xe0) >> 5));
	time = msb * 180 + (lsb * 5625) / 1000;

	pr_info("[Time-to-FULL] %d(secs), %d(mins)\n", time, time / 60);

	if (max77833_read_fg(fuelgauge->i2c,
			     TIME_TO_EMPTY_REG, &data) < 0) {
		pr_err("%s: Failed to read TTE\n", __func__);
		return;
	}

	test_data[0] = data & 0xFF;
	test_data[1] = (data >> 8);

	lsb = test_data[0] & 0x1f;
	msb = ((test_data[1] << 3) + ((test_data[0] & 0xe0) >> 5));
	time = msb * 180 + (lsb * 5625) / 1000;

	pr_info("[Time-to-EMPTY] %d(secs), %d(mins)\n", time, time / 60);
}

static void max77833_fg_test_print(struct max77833_fuelgauge_data *fuelgauge)
{
#ifdef BATTERY_LOG_MESSAGE
	u16 reg_data;

	max77833_read_fg(fuelgauge->i2c, FULLCAP_REG, &reg_data);
	pr_info("%s: FULLCAP(%d), data(0x%04x)\n", __func__,
			reg_data/2, reg_data);

	max77833_read_fg(fuelgauge->i2c, REMCAP_REP_REG, &reg_data);
	pr_info("%s: REMCAP_REP(%d), data(0x%04x)\n", __func__,
			reg_data/2, reg_data);

	max77833_read_fg(fuelgauge->i2c, REMCAP_MIX_REG, &reg_data);
	pr_info("%s: REMCAP_MIX(%d), data(0x%04x)\n", __func__,
			reg_data/2, reg_data);

	max77833_read_fg(fuelgauge->i2c, REMCAP_AV_REG, &reg_data);
	pr_info("%s: REMCAP_AV(%d), data(0x%04x)\n", __func__,
			reg_data/2, reg_data);

	max77833_read_fg(fuelgauge->i2c, CONFIG_REG, &reg_data);
	pr_info("%s: CONFIG_REG(0x%02x), data(0x%04x)\n", __func__,
			CONFIG_REG, reg_data);
#endif

	max77833_fg_read_time(fuelgauge);
}

static void max77833_fg_periodic_read(struct max77833_fuelgauge_data *fuelgauge)
{
	u16 reg_data;
	u16 reg_addr;
	char *str = NULL;

	str = kzalloc(sizeof(char)*1024, GFP_KERNEL);
	if (!str)
		return;

	/* 0x0000~0x009E : 2 step */
	reg_addr = 0x0000;
	while (reg_addr <= 0x009E)
	{
		max77833_read_fg(fuelgauge->i2c, reg_addr, &reg_data);
		sprintf(str+strlen(str), "%04xh,", reg_data);
		reg_addr += 0x02;
	}
	/* 0x0160~0x017E : 2 step */
	reg_addr = 0x0160;
	while (reg_addr <= 0x017E)
	{
		max77833_read_fg(fuelgauge->i2c, reg_addr, &reg_data);
		sprintf(str+strlen(str), "%04xh,", reg_data);
		reg_addr += 0x02;
	}
	/* 0x01A0~0x01BE : 2 step */
	reg_addr = 0x01A0;
	while (reg_addr <= 0x01BE)
	{
		max77833_read_fg(fuelgauge->i2c, reg_addr, &reg_data);
		sprintf(str+strlen(str), "%04xh,", reg_data);
		reg_addr += 0x02;
	}
	/* 0x07C0~0x07EE : 2 step */
	reg_addr = 0x07C0;
	while (reg_addr <= 0x07EE)
	{
		max77833_read_fg(fuelgauge->i2c, reg_addr, &reg_data);
		sprintf(str+strlen(str), "%04xh,", reg_data);
		reg_addr += 0x02;
	}
	/* 0x01D0~0x01D4 : 2 step */
	reg_addr = 0x01D0;
	while (reg_addr <= 0x01D4)
	{
		max77833_read_fg(fuelgauge->i2c, reg_addr, &reg_data);
		sprintf(str+strlen(str), "%04xh,", reg_data);
		reg_addr += 0x02;
	}
	/* 0x07F6~0x07FE : 2 step */
	reg_addr = 0x07F6;
	while (reg_addr <= 0x07FE)
	{
		max77833_read_fg(fuelgauge->i2c, reg_addr, &reg_data);
		sprintf(str+strlen(str), "%04xh,", reg_data);
		reg_addr += 0x02;
	}

	pr_info("[FG] %s\n", str);

	kfree(str);
}
#endif

static int max77833_fg_read_vcell(struct max77833_fuelgauge_data *fuelgauge)
{
	u16 data;
	u32 vcell;
	u32 temp;
	u32 temp2;

	if (max77833_read_fg(fuelgauge->i2c, VCELL_REG, &data) < 0) {
		pr_err("%s: Failed to read VCELL\n", __func__);
		return -1;
	}

	temp = (data & 0xFFF) * 78125;
	vcell = temp / 1000000;

	temp = ((data & 0xF000) >> 4) * 78125;
	temp2 = temp / 1000000;
	vcell += (temp2 << 4);

	if (!(fuelgauge->info.pr_cnt++ % PRINT_COUNT)) {
		fuelgauge->info.pr_cnt = 1;
		pr_info("%s: VCELL(%d), data(0x%04x)\n",
			__func__, vcell, data);
	}

	if ((fuelgauge->vempty_mode == VEMPTY_MODE_SW_VALERT) && 
		(vcell >= fuelgauge->battery_data->sw_v_empty_recover_vol)) {
		fuelgauge->vempty_mode = VEMPTY_MODE_SW_RECOVERY;
		max77833_fg_fuelalert_init(fuelgauge,
					   fuelgauge->pdata->fuel_alert_soc,
					   fuelgauge->battery_data->sw_v_empty_vol);
		pr_info("%s : Recoverd from SW V EMPTY Activation\n", __func__);
	}

	return vcell;
}

static int max77833_fg_read_vfocv(struct max77833_fuelgauge_data *fuelgauge)
{
	u16 data;
	u32 vfocv = 0;
	u32 temp;
	u32 temp2;

	if (max77833_read_fg(fuelgauge->i2c, VFOCV_REG, &data) < 0) {
		pr_err("%s: Failed to read VFOCV\n", __func__);
		return -1;
	}

	temp = (data & 0xFFF) * 78125;
	vfocv = temp / 1000000;

	temp = ((data & 0xF000) >> 4) * 78125;
	temp2 = temp / 1000000;
	vfocv += (temp2 << 4);

#if !defined(CONFIG_SEC_FACTORY)
	max77833_fg_test_print(fuelgauge);
	max77833_fg_periodic_read(fuelgauge);
#endif

	return vfocv;
}

static int max77833_fg_read_avg_vcell(struct max77833_fuelgauge_data *fuelgauge)
{
	u32 avg_vcell = 0;
	u16 data;
	u32 temp;
	u32 temp2;

	if (max77833_read_fg(fuelgauge->i2c, AVR_VCELL_REG, &data) < 0) {
		pr_err("%s: Failed to read AVG_VCELL\n", __func__);
		return -1;
	}

	temp = (data & 0xFFF) * 78125;
	avg_vcell = temp / 1000000;

	temp = ((data & 0xF000) >> 4) * 78125;
	temp2 = temp / 1000000;
	avg_vcell += (temp2 << 4);

	return avg_vcell;
}

static int max77833_fg_check_battery_present(struct max77833_fuelgauge_data *fuelgauge)
{
	u16 status_data;
	int ret = 1;

	/* 1. Check Bst bit */
	if (max77833_read_fg(fuelgauge->i2c, STATUS_REG, &status_data) < 0) {
		pr_err("%s: Failed to read STATUS_REG\n", __func__);
		return 0;
	}

	if (status_data & (0x1 << 3)) {
		pr_info("%s: addr(0x01), data(0x%04x)\n", __func__,
			status_data);
		pr_info("%s: battery is absent!!\n", __func__);
		ret = 0;
	}

	return ret;
}

static void max77833_fg_low_temp_compensation(struct max77833_fuelgauge_data *fuelgauge, bool en)
{
	u16 data;
	u16 reg_data;

	if (!fuelgauge->using_temp_compensation)
		return;

	if (en) {
		/* Reset VALRT Threshold setting (disable) */
		reg_data = 0xFFA2;
		if (max77833_write_fg(fuelgauge->i2c, VALRT_THRESHOLD_REG,
					reg_data) < 0) {
			pr_info("%s: Failed to write VALRT_THRESHOLD_REG\n", __func__);
			return;
		}

		max77833_read_fg(fuelgauge->i2c, VALRT_THRESHOLD_REG, &data);
		pr_info("%s: VALRT_THRESHOLD_REG is (0x%x)\n",
			__func__, data);
	} else {
		/* Reset VALRT Threshold setting (disable) */
		reg_data = 0xFFA5;
		if (max77833_write_fg(fuelgauge->i2c, VALRT_THRESHOLD_REG,
					reg_data) < 0) {
			pr_info("%s: Failed to write VALRT_THRESHOLD_REG\n", __func__);
			return;
		}

		max77833_read_fg(fuelgauge->i2c, VALRT_THRESHOLD_REG, &data);
		pr_info("%s: VALRT_THRESHOLD_REG is (0x%x)\n",
			__func__, data);
	}
}

static int max77833_fg_write_temp(struct max77833_fuelgauge_data *fuelgauge,
			 int temperature)
{
	u8 data[2];
	u16 reg_data;

	data[0] = (temperature%10) * 1000 / 39;
	data[1] = temperature / 10;

	reg_data = (data[1] << 8) | data[0];
	max77833_write_fg(fuelgauge->i2c, TEMPERATURE_REG, reg_data);

	pr_debug("%s: temperature to (%d, 0x%04x)\n",
		__func__, temperature, reg_data);

	fuelgauge->temperature = temperature;
	return temperature;
}

static int max77833_fg_read_temp(struct max77833_fuelgauge_data *fuelgauge)
{
	u8 data[2] = {0, 0};
	u16 reg_data;
	int temper = 0;

	if (max77833_fg_check_battery_present(fuelgauge)) {
		if (max77833_read_fg(fuelgauge->i2c, TEMPERATURE_REG, &reg_data) < 0) {
			pr_err("%s: Failed to read TEMPERATURE_REG\n",
				__func__);
			return -1;
		}

		data[1] = (reg_data >> 8);
		data[0] = (reg_data & 0xFF);

		if (data[1]&(0x1 << 7)) {
			temper = ((~(data[1]))&0xFF)+1;
			temper *= (-1000);
			temper -= ((~((int)data[0]))+1) * 39 / 10;
		} else {
			temper = data[1] & 0x7f;
			temper *= 1000;
			temper += data[0] * 39 / 10;
		}
	} else
		temper = 20000;

	if (!(fuelgauge->info.pr_cnt % PRINT_COUNT))
		pr_info("%s: TEMPERATURE(%d), data(0x%04x)\n",
			__func__, temper, (data[1]<<8) | data[0]);

	return temper/100;
}

/* soc should be 0.1% unit */
static int max77833_fg_read_vfsoc(struct max77833_fuelgauge_data *fuelgauge)
{
	u16 reg_data;
	u8 data[2];
	int soc;

	if (max77833_read_fg(fuelgauge->i2c, VFSOC_REG, &reg_data) < 0) {
		pr_err("%s: Failed to read VFSOC\n", __func__);
		return -1;
	}

	data[1] = (reg_data >> 8);
	data[0] = (reg_data & 0xFF);
	soc = ((data[1] * 100) + (data[0] * 100 / 256)) / 10;

	return min(soc, 1000);
}

/* soc should be 0.1% unit */
static int max77833_fg_read_avsoc(struct max77833_fuelgauge_data *fuelgauge)
{
	u16 reg_data;
	u8 data[2];
	int soc;

	if (max77833_read_fg(fuelgauge->i2c, SOCAV_REG, &reg_data) < 0) {
		pr_err("%s: Failed to read AVSOC\n", __func__);
		return -1;
	}

	data[1] = (reg_data >> 8);
	data[0] = (reg_data & 0xFF);
	soc = ((data[1] * 100) + (data[0] * 100 / 256)) / 10;

	return min(soc, 1000);
}

/* soc should be 0.1% unit */
static int max77833_fg_read_soc(struct max77833_fuelgauge_data *fuelgauge)
{
	u16 reg_data;
	u8 data[2];
	int soc, vf_soc;

	if (max77833_read_fg(fuelgauge->i2c, SOCREP_REG, &reg_data) < 0) {
		pr_err("%s: Failed to read SOCREP\n", __func__);
		return -1;
	}

	data[1] = (reg_data >> 8);
	data[0] = (reg_data & 0xFF);
	soc = ((data[1] * 100) + (data[0] * 100 / 256)) / 10;
	vf_soc = max77833_fg_read_vfsoc(fuelgauge);

#ifdef BATTERY_LOG_MESSAGE
	pr_debug("%s: raw capacity (%d)\n", __func__, soc);

	if (!(fuelgauge->info.pr_cnt % PRINT_COUNT)) {
		pr_debug("%s: raw capacity (%d), data(0x%04x)\n",
			 __func__, soc, (data[1]<<8) | data[0]);
		pr_debug("%s: REPSOC (%d), VFSOC (%d), data(0x%04x)\n",
				__func__, soc/10, vf_soc/10, (data[1]<<8) | data[0]);
	}
#endif

	return min(soc, 1000);
}

/* soc should be 0.01% unit */
static int max77833_fg_read_rawsoc(struct max77833_fuelgauge_data *fuelgauge)
{
	u16 reg_data;
	u8 data[2];
	int soc;

	if (max77833_read_fg(fuelgauge->i2c, SOCREP_REG, &reg_data) < 0) {
		pr_err("%s: Failed to read SOCREP\n", __func__);
		return -1;
	}

	data[1] = (reg_data >> 8);
	data[0] = (reg_data & 0xFF);
	soc = (data[1] * 100) + (data[0] * 100 / 256);

	pr_debug("%s: raw capacity (0.01%%) (%d)\n",
		 __func__, soc);

	if (!(fuelgauge->info.pr_cnt % PRINT_COUNT))
		pr_debug("%s: raw capacity (%d), data(0x%04x)\n",
			 __func__, soc, (data[1]<<8) | data[0]);

	return min(soc, 10000);
}

static int max77833_fg_read_fullcap(struct max77833_fuelgauge_data *fuelgauge)
{
	u16 data;

	if (max77833_read_fg(fuelgauge->i2c, FULLCAP_REG, &data) < 0) {
		pr_err("%s: Failed to read FULLCAP\n", __func__);
		return -1;
	}

	return (int)data;
}

static int max77833_fg_read_fullcaprep(struct max77833_fuelgauge_data *fuelgauge)
{
	u16 data;

	if (max77833_read_fg(fuelgauge->i2c, FULLCAPREP_REG, &data) < 0) {
		pr_err("%s: Failed to read FULLCAP\n", __func__);
		return -1;
	}

	return (int)data;
}


static int max77833_fg_read_fullcapnom(struct max77833_fuelgauge_data *fuelgauge)
{
	u16 data;

	if (max77833_read_fg(fuelgauge->i2c, FULLCAP_NOM_REG, &data) < 0) {
		pr_err("%s: Failed to read FULLCAP\n", __func__);
		return -1;
	}

	return (int)data;
}

static int max77833_fg_read_mixcap(struct max77833_fuelgauge_data *fuelgauge)
{
	u16 data;

	if (max77833_read_fg(fuelgauge->i2c, REMCAP_MIX_REG, &data) <  0) {
		pr_err("%s: Failed to read REMCAP_MIX_REG\n",
		       __func__);
		return -1;
	}

	return (int)data;
}

static int max77833_fg_read_avcap(struct max77833_fuelgauge_data *fuelgauge)
{
	u16 data;

	if (max77833_read_fg(fuelgauge->i2c, REMCAP_AV_REG, &data) < 0) {
		pr_err("%s: Failed to read REMCAP_AV_REG\n",
		       __func__);
		return -1;
	}

	return (int)data;
}

static int max77833_fg_read_repcap(struct max77833_fuelgauge_data *fuelgauge)
{
	u16 data;

	if (max77833_read_fg(fuelgauge->i2c, REMCAP_REP_REG, &data) < 0) {
		pr_err("%s: Failed to read REMCAP_REP_REG\n",
		       __func__);
		return -1;
	}

	return (int)data;
}

static int max77833_fg_read_current(struct max77833_fuelgauge_data *fuelgauge, int unit)
{
	u16 data1;
	u32 temp, sign;
	s32 i_current;

	if (max77833_read_fg(fuelgauge->i2c, CURRENT_REG, &data1) <0) {
		pr_err("%s: Failed to read CURRENT\n", __func__);
		return -1;
	}

	temp = data1 & 0xFFFF;
	/* Debug log for abnormal current case */
	if (temp & (0x1 << 15)) {
		sign = MAX77833_NEGATIVE;
		temp = (~temp & 0xFFFF) + 1;
	} else
		sign = MAX77833_POSITIVE;

	/* 1.5625uV/0.0O5hm(Rsense) = 312.5uA */
	switch (unit) {
	case SEC_BATTERY_CURRENT_UA:
		i_current = temp * 15625 / 50;
		break;
	case SEC_BATTERY_CURRENT_MA:
	default:
		i_current = temp * 15625 / 50000;
	}

	if (sign)
		i_current *= -1;

	pr_debug("%s: current=%d\n", __func__, i_current);

	return i_current;
}

static int max77833_fg_read_avg_current(struct max77833_fuelgauge_data *fuelgauge, int unit)
{
	u16  data2;
	u32 temp, sign;
	s32 avg_current;
	int vcell;
	static int cnt;

	if (max77833_read_fg(fuelgauge->i2c, AVG_CURRENT_REG, &data2) < 0) {
		pr_err("%s: Failed to read AVERAGE CURRENT\n",
		       __func__);
		return -1;
	}

	temp = data2 & 0xFFFF;
	if (temp & (0x1 << 15)) {
		sign = MAX77833_NEGATIVE;
		temp = (~temp & 0xFFFF) + 1;
	} else
		sign = MAX77833_POSITIVE;

	/* 1.5625uV/0.005hm(Rsense) = 312.5uA */
	switch (unit) {
	case SEC_BATTERY_CURRENT_UA:
		avg_current = temp * 15625 / 50;
		break;
	case SEC_BATTERY_CURRENT_MA:
	default:
		avg_current = temp * 15625 / 50000;
	}

	if (sign)
		avg_current *= -1;

	vcell = max77833_fg_read_vcell(fuelgauge);
	if ((vcell > 3000) && (vcell < 3500) && (cnt < 10) && (avg_current < 0) &&
	    fuelgauge->is_charging) {
		avg_current = 1;
		cnt++;
	}

	pr_debug("%s: avg_current=%d\n", __func__, avg_current);

	return avg_current;
}

static int max77833_fg_read_cycle(struct max77833_fuelgauge_data *fuelgauge)
{
	u16 data;

	if (max77833_read_fg(fuelgauge->i2c, CYCLES_REG, &data) < 0) {
		pr_err("%s: Failed to read FULLCAPCYCLE\n", __func__);
		return -1;
	}

	return (int)data;
}

static int max77833_fg_read_isys_current(struct max77833_fuelgauge_data *fuelgauge, int unit)
{
	u16 data1;
	u32 temp, sign;
	s32 i_current;

	if (max77833_read_fg(fuelgauge->i2c, ISYS_REG, &data1) <0) {
		pr_err("%s: Failed to read ISYS CURRENT\n", __func__);
		return -1;
	}

	temp = data1 & 0xFFFF;
	/* Debug log for abnormal current case */
	if (temp & (0x1 << 15)) {
		sign = MAX77833_NEGATIVE;
		temp = (~temp & 0xFFFF) + 1;
	} else
		sign = MAX77833_POSITIVE;

	/* 1.5625uV/0.0O5hm(Rsense) = 312.5uA */
	switch (unit) {
	case SEC_BATTERY_CURRENT_UA:
		i_current = temp * 15625 / 50;
		break;
	case SEC_BATTERY_CURRENT_MA:
	default:
		i_current = temp * 15625 / 50000;
	}

	if (sign)
		i_current *= -1;

	return i_current;
}

static int max77833_fg_read_avg_isys_current(struct max77833_fuelgauge_data *fuelgauge, int unit)
{
	u16  data2;
	u32 temp, sign;
	s32 avg_current;

	if (max77833_read_fg(fuelgauge->i2c, AVGISYS_REG, &data2) < 0) {
		pr_err("%s: Failed to read ISYS AVERAGE CURRENT\n",
		       __func__);
		return -1;
	}

	temp = data2 & 0xFFFF;
	if (temp & (0x1 << 15)) {
		sign = MAX77833_NEGATIVE;
		temp = (~temp & 0xFFFF) + 1;
	} else
		sign = MAX77833_POSITIVE;

	/* 1.5625uV/0.005hm(Rsense) = 312.5uA */
	switch (unit) {
	case SEC_BATTERY_CURRENT_UA:
		avg_current = temp * 15625 / 50;
		break;
	case SEC_BATTERY_CURRENT_MA:
	default:
		avg_current = temp * 15625 / 50000;
	}

	if (sign)
		avg_current *= -1;

	return avg_current;
}

int max77833_fg_reset_soc(struct max77833_fuelgauge_data *fuelgauge)
{
	u16 reg_data, fullcap;
	int vfocv;

	/* delay for current stablization */
	msleep(500);

	pr_info("%s: Before quick-start - VCELL(%d), VFOCV(%d), VfSOC(%d), RepSOC(%d)\n",
		__func__, max77833_fg_read_vcell(fuelgauge), max77833_fg_read_vfocv(fuelgauge),
		max77833_fg_read_vfsoc(fuelgauge), max77833_fg_read_soc(fuelgauge));
	pr_info("%s: Before quick-start - current(%d), avg current(%d)\n",
		__func__, max77833_fg_read_current(fuelgauge, SEC_BATTERY_CURRENT_MA),
		max77833_fg_read_avg_current(fuelgauge, SEC_BATTERY_CURRENT_MA));

	if (fuelgauge->pdata->check_jig_status &&
	    !fuelgauge->pdata->check_jig_status()) {
		pr_info("%s : Return by No JIG_ON signal\n", __func__);
		return 0;
	}

	max77833_write_fg(fuelgauge->i2c, CYCLES_REG, 0);

	if (max77833_read_fg(fuelgauge->i2c, MISCCFG_REG, &reg_data) < 0) {
		pr_err("%s: Failed to read MiscCFG\n", __func__);
		return -1;
	}

	reg_data |= (0x1 << 10);
	if (max77833_write_fg(fuelgauge->i2c, MISCCFG_REG, reg_data) < 0) {
		pr_err("%s: Failed to write MiscCFG\n", __func__);
		return -1;
	}

	msleep(250);
	max77833_write_fg(fuelgauge->i2c, FULLCAP_REG,
			  fuelgauge->battery_data->Capacity);
	msleep(500);

	pr_info("%s: After quick-start - VCELL(%d), VFOCV(%d), VfSOC(%d), RepSOC(%d)\n",
		__func__, max77833_fg_read_vcell(fuelgauge), max77833_fg_read_vfocv(fuelgauge),
		max77833_fg_read_vfsoc(fuelgauge), max77833_fg_read_soc(fuelgauge));
	pr_info("%s: After quick-start - current(%d), avg current(%d)\n",
		__func__, max77833_fg_read_current(fuelgauge, SEC_BATTERY_CURRENT_MA),
		max77833_fg_read_avg_current(fuelgauge, SEC_BATTERY_CURRENT_MA));

	max77833_write_fg(fuelgauge->i2c, CYCLES_REG, 0x00A0);

/* P8 is not turned off by Quickstart @3.4V
 * (It's not a problem, depend on mode data)
 * Power off for factory test(File system, etc..) */
	vfocv = max77833_fg_read_vfocv(fuelgauge);
	if (vfocv < POWER_OFF_VOLTAGE_LOW_MARGIN) {
		pr_info("%s: Power off condition(%d)\n", __func__, vfocv);

		max77833_read_fg(fuelgauge->i2c, FULLCAP_REG, &fullcap);

		/* FullCAP * 0.009 */
		max77833_write_fg(fuelgauge->i2c, REMCAP_REP_REG, (fullcap * 9 / 1000));
		msleep(200);
		pr_info("%s: new soc=%d, vfocv=%d\n", __func__,
			max77833_fg_read_soc(fuelgauge), vfocv);
	}

	pr_info("%s: Additional step - VfOCV(%d), VfSOC(%d), RepSOC(%d)\n",
		__func__, max77833_fg_read_vfocv(fuelgauge),
		max77833_fg_read_vfsoc(fuelgauge), max77833_fg_read_soc(fuelgauge));

	return 0;
}

int max77833_fg_reset_capacity_by_jig_connection(struct max77833_fuelgauge_data *fuelgauge)
{

	pr_info("%s: DesignCap = Capacity - 1 (Jig Connection)\n", __func__);

	return max77833_write_fg(fuelgauge->i2c, DESIGNCAP_REG, 
				 fuelgauge->battery_data->Capacity-1);
}

void max77833_fg_low_batt_compensation(struct max77833_fuelgauge_data *fuelgauge,
				       u32 level)
{
	u16 read_val;
	u32 temp;

	pr_info("%s: Adjust SOCrep to %d!!\n", __func__, level);

	max77833_read_fg(fuelgauge->i2c, FULLCAP_REG, &read_val);
	/* RemCapREP (05h) = FullCap(10h) x 0.0090 */
	temp = read_val * (level*90) / 10000;
	max77833_write_fg(fuelgauge->i2c, REMCAP_REP_REG,
			  (u16)temp);
}

static int max77833_fg_check_status_reg(struct max77833_fuelgauge_data *fuelgauge)
{
	u16 status_data;
	int ret = 0;

	/* 1. Check Smn was generatedread */
	if (max77833_read_fg(fuelgauge->i2c, STATUS_REG, &status_data) < 0) {
		pr_err("%s: Failed to read STATUS_REG\n", __func__);
		return -1;
	}

#ifdef BATTERY_LOG_MESSAGE
	pr_info("%s: addr(0x00), data(0x%04x)\n", __func__,
		status_data);
#endif

	if (status_data & (0x1 << 10))
		ret = 1;

	/* 2. clear Status reg */
	status_data |= 0x0F;
	if (max77833_write_fg(fuelgauge->i2c, STATUS_REG, status_data) < 0) {
		pr_info("%s: Failed to write STATUS_REG\n", __func__);
		return -1;
	}

	return ret;
}

int max77833_get_fuelgauge_value(struct max77833_fuelgauge_data *fuelgauge, int data)
{
	int ret;

	switch (data) {
	case MAX77833_FG_LEVEL:
		ret = max77833_fg_read_soc(fuelgauge);
		break;

	case MAX77833_FG_TEMPERATURE:
		ret = max77833_fg_read_temp(fuelgauge);
		break;

	case MAX77833_FG_VOLTAGE:
		ret = max77833_fg_read_vcell(fuelgauge);
		break;

	case MAX77833_FG_CURRENT:
		ret = max77833_fg_read_current(fuelgauge, SEC_BATTERY_CURRENT_MA);
		break;

	case MAX77833_FG_CURRENT_AVG:
		ret = max77833_fg_read_avg_current(fuelgauge, SEC_BATTERY_CURRENT_MA);
		break;

	case MAX77833_FG_CHECK_STATUS:
		ret = max77833_fg_check_status_reg(fuelgauge);
		break;

	case MAX77833_FG_RAW_SOC:
		ret = max77833_fg_read_rawsoc(fuelgauge);
		break;

	case MAX77833_FG_VF_SOC:
		ret = max77833_fg_read_vfsoc(fuelgauge);
		break;

	case MAX77833_FG_AV_SOC:
		ret = max77833_fg_read_avsoc(fuelgauge);
		break;

	case MAX77833_FG_FULLCAP:
		ret = max77833_fg_read_fullcap(fuelgauge);
		break;

	case MAX77833_FG_FULLCAPNOM:
		ret = max77833_fg_read_fullcapnom(fuelgauge);
		break;

	case MAX77833_FG_FULLCAPREP:
		ret = max77833_fg_read_fullcaprep(fuelgauge);
		break;

	case MAX77833_FG_MIXCAP:
		ret = max77833_fg_read_mixcap(fuelgauge);
		break;

	case MAX77833_FG_AVCAP:
		ret = max77833_fg_read_avcap(fuelgauge);
		break;

	case MAX77833_FG_REPCAP:
		ret = max77833_fg_read_repcap(fuelgauge);
		break;

	case MAX77833_FG_CYCLE:
		ret = max77833_fg_read_cycle(fuelgauge);
		break;

	case MAX77833_FG_ISYS:
		ret = max77833_fg_read_isys_current(fuelgauge, SEC_BATTERY_CURRENT_MA);
		break;

	case MAX77833_FG_AVGISYS:
		ret = max77833_fg_read_avg_isys_current(fuelgauge, SEC_BATTERY_CURRENT_MA);
		break;

	default:
		ret = -1;
		break;
	}

	return ret;
}

int max77833_fg_alert_init(struct max77833_fuelgauge_data *fuelgauge, int soc, int vol)
{
	u16 misccfg_data;
	u16 salrt_data;
	u16 valrt_data;

	fuelgauge->is_fuel_alerted = false;

	/* Using RepSOC */
	if (max77833_read_fg(fuelgauge->i2c, MISCCFG_REG, &misccfg_data) < 0) {
		pr_err("%s: Failed to read MISCCFG_REG\n", __func__);
		return -1;
	}

	misccfg_data = misccfg_data & ~(0x03);
	if (max77833_write_fg(fuelgauge->i2c, MISCCFG_REG, misccfg_data) < 0) {
		pr_info("%s: Failed to write MISCCFG_REG\n", __func__);
		return -1;
	}

	/* SALRT Threshold setting */
	salrt_data = 0xFF00 | soc;
	if (max77833_write_fg(fuelgauge->i2c, SALRT_THRESHOLD_REG, salrt_data) < 0) {
		pr_info("%s: Failed to write SALRT_THRESHOLD_REG\n", __func__);
		return -1;
	}

	if (soc > 0) {
		max77833_update_reg(fuelgauge->pmic, MAX77833_PMIC_REG_FG_INT_MASK,
				    0, MAX77833_FG_SMN_IM);
	}

	/* Reset VALRT Threshold setting */
	if (fuelgauge->vempty_mode != VEMPTY_MODE_SW)
		vol = 0;

	valrt_data = 0xFF00 | (vol / 20);
	if (max77833_write_fg(fuelgauge->i2c, VALRT_THRESHOLD_REG, valrt_data) < 0) {
		pr_info("%s: Failed to write VALRT_THRESHOLD_REG\n", __func__);
		return -1;
	}

	if (vol > 0) {
		max77833_update_reg(fuelgauge->pmic, MAX77833_PMIC_REG_FG_INT_MASK,
				    0, MAX77833_FG_VMN_IM);
	}

	pr_info("[%s] SALRT(0x%04x), VALRT(0x%04x)\n",
		__func__, salrt_data, valrt_data);

	return 1;
}

static void max77833_display_low_batt_comp_cnt(struct max77833_fuelgauge_data *fuelgauge)
{
	pr_info("Check Array(%s): [%d, %d], [%d, %d], ",
			fuelgauge->battery_data->type_str,
			fuelgauge->info.low_batt_comp_cnt[0][0],
			fuelgauge->info.low_batt_comp_cnt[0][1],
			fuelgauge->info.low_batt_comp_cnt[1][0],
			fuelgauge->info.low_batt_comp_cnt[1][1]);
	pr_info("[%d, %d], [%d, %d], [%d, %d]\n",
			fuelgauge->info.low_batt_comp_cnt[2][0],
			fuelgauge->info.low_batt_comp_cnt[2][1],
			fuelgauge->info.low_batt_comp_cnt[3][0],
			fuelgauge->info.low_batt_comp_cnt[3][1],
			fuelgauge->info.low_batt_comp_cnt[4][0],
			fuelgauge->info.low_batt_comp_cnt[4][1]);
}

static void max77833_add_low_batt_comp_cnt(struct max77833_fuelgauge_data *fuelgauge,
				int range, int level)
{
	int i;
	int j;

	/* Increase the requested count value, and reset others. */
	fuelgauge->info.low_batt_comp_cnt[range-1][level/2]++;

	for (i = 0; i < LOW_BATT_COMP_RANGE_NUM; i++) {
		for (j = 0; j < LOW_BATT_COMP_LEVEL_NUM; j++) {
			if (i == range-1 && j == level/2)
				continue;
			else
				fuelgauge->info.low_batt_comp_cnt[i][j] = 0;
		}
	}
}

void max77833_prevent_early_poweroff(struct max77833_fuelgauge_data *fuelgauge,
	int vcell, int *fg_soc)
{
	int soc = 0;
	u16 read_val;

	soc = max77833_fg_read_soc(fuelgauge);

	/* No need to write REMCAP_REP in below normal cases */
	if (soc > POWER_OFF_SOC_HIGH_MARGIN ||
	    vcell > fuelgauge->battery_data->low_battery_comp_voltage)
		return;

	pr_info("%s: soc=%d, vcell=%d\n", __func__, soc, vcell);

	if (vcell > POWER_OFF_VOLTAGE_HIGH_MARGIN) {
		max77833_read_fg(fuelgauge->i2c, FULLCAP_REG, &read_val);
		/* FullCAP * 0.013 */
		max77833_write_fg(fuelgauge->i2c, REMCAP_REP_REG, (read_val * 13 / 1000));
		msleep(200);
		*fg_soc = max77833_fg_read_soc(fuelgauge);
		pr_info("%s: new soc=%d, vcell=%d\n", __func__, *fg_soc, vcell);
	}
}

void max77833_reset_low_batt_comp_cnt(struct max77833_fuelgauge_data *fuelgauge)
{
	memset(fuelgauge->info.low_batt_comp_cnt, 0,
		sizeof(fuelgauge->info.low_batt_comp_cnt));
}

static int max77833_check_low_batt_comp_condition(
	struct max77833_fuelgauge_data *fuelgauge,
	int *nLevel)
{
	int i;
	int j;
	int ret = 0;

	for (i = 0; i < LOW_BATT_COMP_RANGE_NUM; i++) {
		for (j = 0; j < LOW_BATT_COMP_LEVEL_NUM; j++) {
			if (fuelgauge->info.low_batt_comp_cnt[i][j] >=
				MAX_LOW_BATT_CHECK_CNT) {
				max77833_display_low_batt_comp_cnt(fuelgauge);
				ret = 1;
				*nLevel = j*2 + 1;
				break;
			}
		}
	}

	return ret;
}

static int max77833_get_low_batt_threshold(struct max77833_fuelgauge_data *fuelgauge,
				int range, int nCurrent, int level)
{
	int ret = 0;

	ret = fuelgauge->battery_data->low_battery_table[range][MAX77833_OFFSET] +
		((nCurrent *
		fuelgauge->battery_data->low_battery_table[range][MAX77833_SLOPE]) /
		1000);

	return ret;
}

int max77833_low_batt_compensation(struct max77833_fuelgauge_data *fuelgauge,
		int fg_soc, int fg_vcell, int fg_current)
{
	int fg_avg_current = 0;
	int fg_min_current = 0;
	int new_level = 0;
	int i, table_size;

	/* Not charging, Under low battery comp voltage */
	if (fg_vcell <= fuelgauge->battery_data->low_battery_comp_voltage) {
		fg_avg_current = max77833_fg_read_avg_current(fuelgauge,
			SEC_BATTERY_CURRENT_MA);
		fg_min_current = min(fg_avg_current, fg_current);

		table_size =
			sizeof(fuelgauge->battery_data->low_battery_table) /
			(sizeof(s16)*MAX77833_TABLE_MAX);

		for (i = 1; i < CURRENT_RANGE_MAX_NUM; i++) {
			if ((fg_min_current >= fuelgauge->battery_data->
				low_battery_table[i-1][MAX77833_RANGE]) &&
				(fg_min_current < fuelgauge->battery_data->
				low_battery_table[i][MAX77833_RANGE])) {
				if (fg_soc >= 10 && fg_vcell <
					max77833_get_low_batt_threshold(fuelgauge,
					i, fg_min_current, 1)) {
					max77833_add_low_batt_comp_cnt(
						fuelgauge, i, 1);
				} else {
					max77833_reset_low_batt_comp_cnt(fuelgauge);
				}
			}
		}

		if (max77833_check_low_batt_comp_condition(fuelgauge, &new_level)) {
			max77833_fg_low_batt_compensation(fuelgauge, new_level);
			max77833_reset_low_batt_comp_cnt(fuelgauge);

			/* Do not update soc right after
			 * low battery compensation
			 * to prevent from powering-off suddenly
			 */
			pr_info("%s: SOC is set to %d by low compensation!!\n",
				__func__, max77833_fg_read_soc(fuelgauge));
		}
	}

	/* Prevent power off over 3500mV */
	max77833_prevent_early_poweroff(fuelgauge, fg_vcell, &fg_soc);

	return fg_soc;
}

static bool max77833_fuelgauge_recovery_handler(struct max77833_fuelgauge_data *fuelgauge)
{
	if (fuelgauge->info.soc < LOW_BATTERY_SOC_REDUCE_UNIT) {
		fuelgauge->info.is_low_batt_alarm = false;
	} else {
		pr_err("%s: Reduce the Reported SOC by 1%%\n",
			__func__);

		fuelgauge->info.soc -=
			LOW_BATTERY_SOC_REDUCE_UNIT;
		pr_err("%s: New Reduced RepSOC (%d)\n",
			__func__, fuelgauge->info.soc);
	}

	return fuelgauge->info.is_low_batt_alarm;
}

static int max77833_get_fuelgauge_soc(struct max77833_fuelgauge_data *fuelgauge)
{
	union power_supply_propval value;
	int fg_soc = 0;
	int fg_vfsoc;
	int fg_vcell;
	int fg_current;
	int avg_current;

	if (fuelgauge->info.is_low_batt_alarm)
		if (max77833_fuelgauge_recovery_handler(fuelgauge)) {
			fg_soc = fuelgauge->info.soc;
			goto return_soc;
		}

	fg_soc = max77833_get_fuelgauge_value(fuelgauge, MAX77833_FG_LEVEL);
	if (fg_soc < 0) {
		pr_info("Can't read soc!!!");
		fg_soc = fuelgauge->info.soc;
	}

	fg_vcell = max77833_get_fuelgauge_value(fuelgauge, MAX77833_FG_VOLTAGE);
	fg_current = max77833_get_fuelgauge_value(fuelgauge, MAX77833_FG_CURRENT);
	avg_current = max77833_get_fuelgauge_value(fuelgauge, MAX77833_FG_CURRENT_AVG);
	fg_vfsoc = max77833_get_fuelgauge_value(fuelgauge, MAX77833_FG_VF_SOC);

	psy_do_property("battery", get,
		POWER_SUPPLY_PROP_STATUS, value);

	/*  Checks vcell level and tries to compensate SOC if needed.*/
	/*  If jig cable is connected, then skip low batt compensation check. */
	if (fuelgauge->pdata->check_jig_status &&
	    !fuelgauge->pdata->check_jig_status() &&
		value.intval == POWER_SUPPLY_STATUS_DISCHARGING)
		fg_soc = max77833_low_batt_compensation(
			fuelgauge, fg_soc, fg_vcell, fg_current);

	if (fuelgauge->info.is_first_check)
		fuelgauge->info.is_first_check = false;

	if ((fg_vcell < 3400) && (avg_current < 0) && (fg_soc <= 10))
		fg_soc = 0;

	fuelgauge->info.soc = fg_soc;

return_soc:
	pr_debug("%s: soc(%d), low_batt_alarm(%d)\n",
		__func__, fuelgauge->info.soc,
		fuelgauge->info.is_low_batt_alarm);

	return fg_soc;
}

static irqreturn_t max77833_jig_irq_thread(int irq, void *irq_data)
{
	struct max77833_fuelgauge_data *fuelgauge = irq_data;

	if (fuelgauge->pdata->check_jig_status &&
	    fuelgauge->pdata->check_jig_status())
		max77833_fg_reset_capacity_by_jig_connection(fuelgauge);
	else
		pr_info("%s: jig removed\n", __func__);
	return IRQ_HANDLED;
}

bool max77833_fg_init(struct max77833_fuelgauge_data *fuelgauge)
{
	ktime_t	current_time;
	struct timespec ts;
	u16 data;

#if defined(ANDROID_ALARM_ACTIVATED)
	current_time = alarm_get_elapsed_realtime();
	ts = ktime_to_timespec(current_time);
#else
	current_time = ktime_get_boottime();
	ts = ktime_to_timespec(current_time);
#endif

	fuelgauge->info.fullcap_check_interval = ts.tv_sec;

	fuelgauge->info.is_low_batt_alarm = false;
	fuelgauge->info.is_first_check = true;

	/* Init parameters to prevent wrong compensation. */
	fuelgauge->info.previous_fullcap =
		max77833_read_word(fuelgauge->i2c, FULLCAP_REG);
	fuelgauge->info.previous_vffullcap =
		max77833_read_word(fuelgauge->i2c, FULLCAP_NOM_REG);

	if (fuelgauge->pdata->check_jig_status &&
	    fuelgauge->pdata->check_jig_status())
		max77833_fg_reset_capacity_by_jig_connection(fuelgauge);
	else {
		if (fuelgauge->pdata->jig_irq) {
			int ret;
			ret = request_threaded_irq(fuelgauge->pdata->jig_irq,
					NULL, max77833_jig_irq_thread,
					fuelgauge->pdata->jig_irq_attr,
					"jig-irq", fuelgauge);
			if (ret) {
				pr_info("%s: Failed to Request IRQ\n",
					__func__);
			}
		}
	}

	/* NOT using FG for temperature */
	if (fuelgauge->pdata->thermal_source != SEC_BATTERY_THERMAL_SOURCE_FG) {
		if (max77833_read_fg(fuelgauge->i2c, CONFIG_REG, &data) < 0) {
			pr_err ("%s : Failed to read CONFIG_REG\n", __func__);
			return false;
		}
		data |= 0x10;
		if (max77833_write_fg(fuelgauge->i2c, CONFIG_REG, data) < 0) {
			pr_info("%s : Failed to write CONFIG_REG\n", __func__);
			return false;
		}
	}

	/* Enable OCP (0xFF : 10.24A/256) */
	data = 0x00FF;
	max77833_write_fg(fuelgauge->i2c, ISYSTH_REG, data);
	pr_info("%s: Enable OCP - IsysTH 0x%x\n", __func__, data);

	/* Set AvgISys time constant to TaskPeriod*2^(NCURR) */
	data = 0x0006;
	max77833_write_fg(fuelgauge->i2c, FILTERCFG2_REG, data);
	pr_info("%s: NCURR - FILTERCFG2_REG 0x%04x\n", __func__, data);

#if !defined(CONFIG_SEC_FACTORY)
	max77833_fg_periodic_read(fuelgauge);
#endif
	return true;
}

bool max77833_fg_fuelalert_init(struct max77833_fuelgauge_data *fuelgauge,
				int soc, int vol)
{
	/* 1. Set max77833 alert configuration. */
	if (max77833_fg_alert_init(fuelgauge, soc, vol) > 0)
		return true;
	else
		return false;
}

bool max77833_fg_fuelalert_process(void *irq_data)
{
	struct max77833_fuelgauge_data *fuelgauge =
		(struct max77833_fuelgauge_data *)irq_data;
	u16 status_data;

	if (max77833_read_fg(fuelgauge->i2c, STATUS_REG, &status_data) < 0)
		pr_err("%s : Failed to read STATUS_REG\n", __func__);

	if ((status_data & 0x0100) && !lpcharge && !fuelgauge->is_charging) {
		pr_info("%s : Battery Voltage is Very Low!! SW V EMPTY ENABLE\n", __func__);
		fuelgauge->vempty_mode = VEMPTY_MODE_SW_VALERT;
	}

	return true;
}

bool max77833_fg_reset(struct max77833_fuelgauge_data *fuelgauge)
{
	if (!max77833_fg_reset_soc(fuelgauge))
		return true;
	else
		return false;
}

#define CAPACITY_MAX_CONTROL_THRESHOLD 300

static void max77833_fg_get_scaled_capacity(
	struct max77833_fuelgauge_data *fuelgauge,
	union power_supply_propval *val)
{
	union power_supply_propval value, chg_val, chg_val2;
	int max_temp;
#if defined(CONFIG_BATTERY_SWELLING)
	union power_supply_propval swelling_val;
#endif

	psy_do_property("battery", get, POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION, value);
	if (value.intval == POWER_SUPPLY_TYPE_HV_WIRELESS_ETX)
		value.intval = POWER_SUPPLY_TYPE_HV_WIRELESS;
#if defined(CONFIG_BATTERY_SWELLING)
	/* Check whether DUT is in the swelling mode or not */
	psy_do_property("battery", get, POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT, swelling_val);
#endif
	psy_do_property("max77833-charger", get, POWER_SUPPLY_PROP_CURRENT_NOW,
			chg_val);
	psy_do_property("max77833-charger", get, POWER_SUPPLY_PROP_CHARGE_NOW,
			chg_val2);
	pr_info("%s : CABLE TYPE(%d) INPUT CURRENT(%d) CHARGINGE MODE(%s)\n",
		__func__, value.intval, chg_val.intval, chg_val2.strval);

	max_temp = fuelgauge->capacity_max;

	if ((value.intval != POWER_SUPPLY_TYPE_BATTERY) &&
#if defined(CONFIG_BATTERY_SWELLING)
	    (!swelling_val.intval) &&
#endif
	    (!strcmp(chg_val2.strval, "CV Mode")) &&
	    (chg_val.intval >= 1000)) {
		int temp, sample;
		int curr;
		int topoff;
		int capacity_threshold;
		static int cnt;

		curr = max77833_get_fuelgauge_value(fuelgauge, MAX77833_FG_CURRENT_AVG);
		topoff = fuelgauge->pdata->charging_current[value.intval].full_check_current_1st;
		capacity_threshold = topoff + CAPACITY_MAX_CONTROL_THRESHOLD;

		pr_info("%s : curr(%d) topoff(%d) capacity_max(%d)\n", __func__, curr, topoff, max_temp);

		if ((curr < capacity_threshold) && (curr > topoff)) {
			if (!cnt) {
				cnt = 1;
				fuelgauge->standard_capacity = (val->intval < fuelgauge->pdata->capacity_min) ?
					0 : ((val->intval - fuelgauge->pdata->capacity_min) * 999 /
					     (fuelgauge->capacity_max - fuelgauge->pdata->capacity_min));
			} else if (fuelgauge->standard_capacity < 999) {
				temp = (val->intval < fuelgauge->pdata->capacity_min) ?
					0 : ((val->intval - fuelgauge->pdata->capacity_min) * 999 /
					     (fuelgauge->capacity_max - fuelgauge->pdata->capacity_min));

				sample = ((capacity_threshold - curr) * (999 - fuelgauge->standard_capacity)) /
					(capacity_threshold - topoff);

				pr_info("%s : %d = ((%d - %d) * (999 - %d)) / (%d - %d)\n",
					__func__,
					sample, capacity_threshold, curr, fuelgauge->standard_capacity,
					capacity_threshold, topoff);

				if ((temp - fuelgauge->standard_capacity) >= sample) {
					pr_info("%s : TEMP > SAMPLE\n", __func__);
				} else if ((sample - (temp - fuelgauge->standard_capacity)) < 5) {
					pr_info("%s : TEMP < SAMPLE && GAP UNDER 5\n", __func__);
					max_temp -= (sample - (temp - fuelgauge->standard_capacity));
				} else {
					pr_info("%s : TEMP > SAMPLE && GAP OVER 5\n", __func__);
					max_temp -= 5;
				}
				pr_info("%s : TEMP(%d) SAMPLE(%d) CAPACITY_MAX(%d)\n",
					__func__, temp, sample, fuelgauge->capacity_max);
			}
		} else {
			cnt = 0;
		}
	}

	if (max_temp <
		fuelgauge->pdata->capacity_max -
		fuelgauge->pdata->capacity_max_margin) {
		fuelgauge->capacity_max =
			fuelgauge->pdata->capacity_max -
			fuelgauge->pdata->capacity_max_margin;
		pr_debug("%s: capacity_max (%d)", __func__,
			 fuelgauge->capacity_max);
	} else {
		fuelgauge->capacity_max =
			(max_temp >
			fuelgauge->pdata->capacity_max +
			fuelgauge->pdata->capacity_max_margin) ?
			(fuelgauge->pdata->capacity_max +
			fuelgauge->pdata->capacity_max_margin) :
			max_temp;
		pr_debug("%s: capacity_max (%d)", __func__,
			 fuelgauge->capacity_max);
	}

	val->intval = (val->intval < fuelgauge->pdata->capacity_min) ?
		0 : ((val->intval - fuelgauge->pdata->capacity_min) * 1000 /
		     (fuelgauge->capacity_max - fuelgauge->pdata->capacity_min));

	pr_info("%s: scaled capacity (%d.%d)\n",
		 __func__, val->intval/10, val->intval%10);
}

/* capacity is integer */
static void max77833_fg_get_atomic_capacity(
	struct max77833_fuelgauge_data *fuelgauge,
	union power_supply_propval *val)
{

	pr_info("%s : NOW(%d), OLD(%d)\n",
		__func__, val->intval, fuelgauge->capacity_old);

	if (fuelgauge->pdata->capacity_calculation_type &
		SEC_FUELGAUGE_CAPACITY_TYPE_ATOMIC) {
	if (fuelgauge->capacity_old < val->intval)
		val->intval = fuelgauge->capacity_old + 1;
	else if (fuelgauge->capacity_old > val->intval)
		val->intval = fuelgauge->capacity_old - 1;
	}

	/* keep SOC stable in abnormal status */
	if (fuelgauge->pdata->capacity_calculation_type &
		SEC_FUELGAUGE_CAPACITY_TYPE_SKIP_ABNORMAL) {
		if (!fuelgauge->is_charging &&
			fuelgauge->capacity_old < val->intval) {
			pr_err("%s: capacity (old %d : new %d)\n",
				__func__, fuelgauge->capacity_old, val->intval);
			val->intval = fuelgauge->capacity_old;
		}
	}

	/* updated old capacity */
	fuelgauge->capacity_old = val->intval;
}

static int max77833_fg_check_capacity_max(
				struct max77833_fuelgauge_data *fuelgauge, int capacity_max)
{
	int new_capacity_max = capacity_max;

	if (new_capacity_max < (fuelgauge->pdata->capacity_max -
			fuelgauge->pdata->capacity_max_margin - 10)) {
		new_capacity_max =
			(fuelgauge->pdata->capacity_max -
			fuelgauge->pdata->capacity_max_margin);

		pr_info("%s: set capacity max(%d --> %d)\n",
			__func__, capacity_max, new_capacity_max);
	} else if (new_capacity_max > (fuelgauge->pdata->capacity_max +
			fuelgauge->pdata->capacity_max_margin)) {
		new_capacity_max =
			(fuelgauge->pdata->capacity_max +
			fuelgauge->pdata->capacity_max_margin);

		pr_info("%s: set capacity max(%d --> %d)\n",
			__func__, capacity_max, new_capacity_max);
	}

	return new_capacity_max;
}

static int max77833_fg_calculate_dynamic_scale(
	struct max77833_fuelgauge_data *fuelgauge, int capacity)
{
	union power_supply_propval raw_soc_val;
	raw_soc_val.intval = max77833_get_fuelgauge_value(fuelgauge,
						 MAX77833_FG_RAW_SOC) / 10;

	if (raw_soc_val.intval <
		fuelgauge->pdata->capacity_max -
		fuelgauge->pdata->capacity_max_margin) {
		fuelgauge->capacity_max =
			fuelgauge->pdata->capacity_max -
			fuelgauge->pdata->capacity_max_margin;
		pr_debug("%s: capacity_max (%d)", __func__,
			 fuelgauge->capacity_max);
	} else {
		fuelgauge->capacity_max =
			(raw_soc_val.intval >
			fuelgauge->pdata->capacity_max +
			fuelgauge->pdata->capacity_max_margin) ?
			(fuelgauge->pdata->capacity_max +
			fuelgauge->pdata->capacity_max_margin) :
			raw_soc_val.intval;
		pr_debug("%s: raw soc (%d)", __func__,
			 fuelgauge->capacity_max);
	}

	if (capacity != 100) {
		fuelgauge->capacity_max = max77833_fg_check_capacity_max(
			fuelgauge, (fuelgauge->capacity_max * 100 / capacity));
	} else {
		fuelgauge->capacity_max =
			(fuelgauge->capacity_max * 99 / 100);
		fuelgauge->capacity_old = 100;
	}

	pr_info("%s: %d is used for capacity_max, capacity(%d)\n",
		__func__, fuelgauge->capacity_max, capacity);

	return fuelgauge->capacity_max;
}

static void max77833_fg_check_qrtable(struct max77833_fuelgauge_data *fuelgauge)
{
	u16 qrtable20, qrtable30;

	max77833_read_fg(fuelgauge->i2c, QRTABLE20_REG, &qrtable20);
	if (qrtable20 != fuelgauge->battery_data->QResidual20) {
		if (max77833_write_fg(fuelgauge->i2c, QRTABLE20_REG,
				      fuelgauge->battery_data->QResidual20) < 0)
			pr_err("%s: Failed to write QRTABLE20\n", __func__);
	}
	max77833_read_fg(fuelgauge->i2c, QRTABLE30_REG, &qrtable30);
	if (qrtable30 != fuelgauge->battery_data->QResidual30) {
		if (max77833_write_fg(fuelgauge->i2c, QRTABLE30_REG, 
				      fuelgauge->battery_data->QResidual30) < 0)
			pr_err("%s: Failed to write QRTABLE30\n", __func__);
	}
	pr_info("%s: QRTABLE20_REG(0x%04x), QRTABLE30_REG(0x%04x)\n", __func__,
		qrtable20, qrtable30);
}

#if defined(CONFIG_EN_OOPS)
static void max77833_set_full_value(struct max77833_fuelgauge_data *fuelgauge,
				    int cable_type)
{
	u16 ichgterm, misccfg, fullsocthr;

	if ((cable_type == POWER_SUPPLY_TYPE_HV_MAINS) ||
	    (cable_type == POWER_SUPPLY_TYPE_HV_ERR)) {
		ichgterm = fuelgauge->battery_data->ichgterm_2nd;
		misccfg = fuelgauge->battery_data->misccfg_2nd;
		fullsocthr = fuelgauge->battery_data->fullsocthr_2nd;
	} else {
		ichgterm = fuelgauge->battery_data->ichgterm;
		misccfg = fuelgauge->battery_data->misccfg;
		fullsocthr = fuelgauge->battery_data->fullsocthr;
	}

	max77833_write_fg(fuelgauge->i2c, ICHGTERM_REG, ichgterm);
	max77833_write_fg(fuelgauge->i2c, MISCCFG_REG, misccfg);
	max77833_write_fg(fuelgauge->i2c, FULLSOCTHR_REG, fullsocthr);

	pr_info("%s : ICHGTERM(0x%04x) FULLSOCTHR(0x%04x), MISCCFG(0x%04x)\n",
		__func__, ichgterm, misccfg, fullsocthr);
}
#endif

static int calc_ttf(struct max77833_fuelgauge_data *fuelgauge, union power_supply_propval *val)
{
	union power_supply_propval chg_val2;
	union power_supply_propval cable_val;
	int i;
	int cc_time = 0;

	int soc = fuelgauge->raw_capacity;
	int current_now = fuelgauge->current_now;
	int current_avg = fuelgauge->current_avg;
	int charge_current = (current_avg > 0)? current_avg : current_now;
	struct cv_slope *cv_data = fuelgauge->cv_data;
	int design_cap = fuelgauge->battery_data->Capacity;

	if(!cv_data || (val->intval <= 0)) {
		pr_info("%s: no cv_data or val: %d\n", __func__, val->intval);
		return -1;
	}

	psy_do_property("battery", get, POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION,
			cable_val);

	if ((cable_val.intval != POWER_SUPPLY_TYPE_HV_MAINS) &&
		(cable_val.intval != POWER_SUPPLY_TYPE_HV_ERR) &&
		(cable_val.intval != POWER_SUPPLY_TYPE_HV_MAINS_CHG_LIMIT) &&
		(cable_val.intval != POWER_SUPPLY_TYPE_HV_WIRELESS) &&
		(cable_val.intval != POWER_SUPPLY_TYPE_HV_WIRELESS_ETX) &&
		(cable_val.intval != POWER_SUPPLY_TYPE_WIRELESS) &&
		(cable_val.intval != POWER_SUPPLY_TYPE_PMA_WIRELESS)) {
		union power_supply_propval input_info_val;
		int detected_iin;

		input_info_val.intval = CHG_INIT_DETECTED_I_IN;
		psy_do_property("max77833-charger", get,
			POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION, input_info_val);
		detected_iin = input_info_val.intval;

		if (detected_iin > 0) {
			val->intval = detected_iin;
		} else if (detected_iin == -1) {
			return -1;
		}

		pr_debug("%s: use current %d (detected_iin:%d)\n",
			__func__, val->intval, detected_iin);
	}

	/* To prevent overflow if charge current is 30 under, change value*/
	if (charge_current <= 30) {
		charge_current = val->intval;
	}
	psy_do_property("max77833-charger", get, POWER_SUPPLY_PROP_CHARGE_NOW,
			chg_val2);
	if (!strcmp(chg_val2.strval, "CC Mode") || !strcmp(chg_val2.strval, "NONE")) { //CC mode || NONE
		charge_current = val->intval;
	} else if ((!strcmp(chg_val2.strval, "CV Mode") || !strcmp(chg_val2.strval, "EOC")) &&
		   (current_now > 0) && (current_now > current_avg)) {
		charge_current = current_now;
		pr_info("%s : charge current(%d)\n", __func__, charge_current);
	}

	for (i = 0; i < fuelgauge->cv_data_lenth ;i++) {
		if (charge_current >= cv_data[i].fg_current)
			break;
	}
	if (i == fuelgauge->cv_data_lenth)
		i = fuelgauge->cv_data_lenth-1;

	if (cv_data[i].soc < soc || !strcmp(chg_val2.strval, "EOC")) {
		for (i = 0; i < fuelgauge->cv_data_lenth; i++) {
			if (soc <= cv_data[i].soc)
				break;
		}
		if (i == fuelgauge->cv_data_lenth)
			i = fuelgauge->cv_data_lenth-1;
	} else if (!strcmp(chg_val2.strval, "CC Mode") || !strcmp(chg_val2.strval, "NONE")) { //CC mode || NONE
		cc_time = design_cap * (cv_data[i].soc - soc)\
				/ val->intval * 3600 / 1000;
		pr_debug("%s: cc_time: %d\n", __func__, cc_time);
		if (cc_time < 0) {

			cc_time = 0;
		}
	}

        pr_debug("%s: cap: %d, soc: %4d, T: %6d, now: %4d, avg: %4d, cv soc: %4d, i: %4d, cable:%d val: %d, %s\n",
         __func__, design_cap, soc, cv_data[i].time + cc_time, current_now, current_avg, cv_data[i].soc, i, cable_val.intval, val->intval, chg_val2.strval);

        if (cv_data[i].time + cc_time >= 60)
                return cv_data[i].time + cc_time;
        else
                return 60; //minimum 1minutes
}

static void max77833_fg_set_vempty(struct max77833_fuelgauge_data *fuelgauge, int vempty_mode)
{
	u16 data = 0;
	u16 valrt_data;	

	if (!fuelgauge->using_temp_compensation) {
		pr_info("%s: does not use temp compensation, default hw vempty\n", __func__);
		vempty_mode = VEMPTY_MODE_HW;
	}

	fuelgauge->vempty_mode = vempty_mode;
	switch (vempty_mode) {
	case VEMPTY_MODE_SW:
		/* HW Vempty Disable */
		max77833_write_fg(fuelgauge->i2c, VEMPTY_REG, fuelgauge->battery_data->V_empty_origin);		
		/* Reset VALRT Threshold setting (enable) */
		valrt_data = 0xFF00 | (fuelgauge->battery_data->sw_v_empty_vol / 20);
		if (max77833_write_fg(fuelgauge->i2c, VALRT_THRESHOLD_REG, valrt_data) < 0) {
			pr_info("%s: Failed to write VALRT_THRESHOLD_REG\n", __func__);
			return;
		}

		max77833_read_fg(fuelgauge->i2c, VALRT_THRESHOLD_REG, &data);
		pr_info("%s: HW V EMPTY Disable, SW V EMPTY Enable with %d mV (%d) \n",
			__func__, fuelgauge->battery_data->sw_v_empty_vol, (data&0x00ff)*20);
		break;
	default:
		/* HW Vempty Enable */
		max77833_write_fg(fuelgauge->i2c, VEMPTY_REG, fuelgauge->battery_data->V_empty);
		/* Reset VALRT Threshold setting (disable) */
		valrt_data = 0xFF00;
		if (max77833_write_fg(fuelgauge->i2c, VALRT_THRESHOLD_REG, valrt_data) < 0) {
			pr_info("%s: Failed to write VALRT_THRESHOLD_REG\n", __func__);
			return;
		}

		max77833_read_fg(fuelgauge->i2c, VALRT_THRESHOLD_REG, &data);
		pr_info("%s: HW V EMPTY Enable, SW V EMPTY Disable %d mV (%d) \n",
			__func__, 0, (data&0x00ff)*20);
		break;
	}
}

static int max77833_fg_get_property(struct power_supply *psy,
			     enum power_supply_property psp,
			     union power_supply_propval *val)
{
	struct max77833_fuelgauge_data *fuelgauge =
		container_of(psy, struct max77833_fuelgauge_data, psy_fg);
	static int abnormal_current_cnt = 0;
	union power_supply_propval value;
	int fg_vcell;
	int avg_current;

	switch (psp) {
		/* Cell voltage (VCELL, mV) */
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = max77833_get_fuelgauge_value(fuelgauge, MAX77833_FG_VOLTAGE);
		break;
		/* Additional Voltage Information (mV) */
	case POWER_SUPPLY_PROP_VOLTAGE_AVG:
		switch (val->intval) {
		case SEC_BATTERY_VOLTAGE_OCV:
			val->intval = max77833_fg_read_vfocv(fuelgauge);
			break;
		case SEC_BATTERY_VOLTAGE_AVERAGE:
		default:
			val->intval = max77833_fg_read_avg_vcell(fuelgauge);
			break;
		}
		break;
		/* Current */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		switch (val->intval) {
		case SEC_BATTERY_CURRENT_UA:
			val->intval =
				max77833_fg_read_current(fuelgauge,
						SEC_BATTERY_CURRENT_UA);
			break;
		case SEC_BATTERY_CURRENT_MA:
		default:
			fuelgauge->current_now = val->intval = max77833_get_fuelgauge_value(fuelgauge,
							  MAX77833_FG_CURRENT);
			psy_do_property("battery", get,
					POWER_SUPPLY_PROP_STATUS, value);
			/* To save log for abnormal case */
			if (value.intval == POWER_SUPPLY_STATUS_DISCHARGING && val->intval > 0) {
				abnormal_current_cnt++;
				if (abnormal_current_cnt >= 5) {
					pr_info("%s : Inow is increasing in not charging status\n",
						__func__);
					value.intval = fuelgauge->capacity_old + 15;
					psy_do_property("battery", set,
							POWER_SUPPLY_PROP_CAPACITY, value);
					abnormal_current_cnt = 0;
					value.intval = fuelgauge->capacity_old;
					psy_do_property("battery", set,
							POWER_SUPPLY_PROP_CAPACITY, value);
				}
			} else {
				abnormal_current_cnt = 0;
			}
			break;
		}
		break;
		/* Average Current */
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		switch (val->intval) {
		case SEC_BATTERY_CURRENT_UA:
			val->intval =
				max77833_fg_read_avg_current(fuelgauge,
						    SEC_BATTERY_CURRENT_UA);
			break;
		case SEC_BATTERY_CURRENT_MA:
		default:
			fuelgauge->current_avg = val->intval =
				max77833_get_fuelgauge_value(fuelgauge,
						    MAX77833_FG_CURRENT_AVG);
			break;
		}
		break;
		/* Full Capacity */
	case POWER_SUPPLY_PROP_ENERGY_NOW:
		switch (val->intval) {
		case SEC_BATTERY_CAPACITY_DESIGNED:
			val->intval = max77833_get_fuelgauge_value(fuelgauge,
								   MAX77833_FG_FULLCAP);
			break;
		case SEC_BATTERY_CAPACITY_ABSOLUTE:
			val->intval = max77833_get_fuelgauge_value(fuelgauge,
								   MAX77833_FG_MIXCAP);
			break;
		case SEC_BATTERY_CAPACITY_TEMPERARY:
			val->intval = max77833_get_fuelgauge_value(fuelgauge,
								   MAX77833_FG_AVCAP);
			break;
		case SEC_BATTERY_CAPACITY_CURRENT:
			val->intval = max77833_get_fuelgauge_value(fuelgauge,
								   MAX77833_FG_REPCAP);
			break;
		case SEC_BATTERY_CAPACITY_AGEDCELL:
			val->intval = max77833_get_fuelgauge_value(fuelgauge,
							  MAX77833_FG_FULLCAPNOM);
			break;
		case SEC_BATTERY_CAPACITY_CYCLE:
			val->intval = max77833_get_fuelgauge_value(fuelgauge,
							  MAX77833_FG_CYCLE);
			break;
		case SEC_BATTERY_CAPACITY_FULL:
			val->intval = max77833_get_fuelgauge_value(fuelgauge,
							  MAX77833_FG_FULLCAPREP);
			break;			
		}
		break;
		/* SOC (%) */
	case POWER_SUPPLY_PROP_CAPACITY:
		if (val->intval == SEC_FUELGAUGE_CAPACITY_TYPE_RAW) {
			val->intval = max77833_get_fuelgauge_value(fuelgauge,
								   MAX77833_FG_RAW_SOC);
		} else {
			val->intval = max77833_get_fuelgauge_soc(fuelgauge);

			fuelgauge->raw_capacity = val->intval;

			if (fuelgauge->pdata->capacity_calculation_type &
			    (SEC_FUELGAUGE_CAPACITY_TYPE_SCALE |
			     SEC_FUELGAUGE_CAPACITY_TYPE_DYNAMIC_SCALE))
				max77833_fg_get_scaled_capacity(fuelgauge, val);

			/* capacity should be between 0% and 100%
			 * (0.1% degree)
			 */
			if (val->intval > 1000)
				val->intval = 1000;
			if (val->intval < 0)
				val->intval = 0;

			/* get only integer part */
			val->intval /= 10;

			/* SW/HW V Empty setting */
			if (fuelgauge->using_hw_vempty) {
				if (fuelgauge->temperature <= (int)fuelgauge->low_temp_limit) {
					if (fuelgauge->raw_capacity <= 50) {
						if (fuelgauge->vempty_mode != VEMPTY_MODE_HW) {
							max77833_fg_set_vempty(fuelgauge, VEMPTY_MODE_HW);
						}
					} else if (fuelgauge->vempty_mode == VEMPTY_MODE_HW) {
						max77833_fg_set_vempty(fuelgauge, VEMPTY_MODE_SW);
					}
				} else if (fuelgauge->vempty_mode != VEMPTY_MODE_HW) {
					max77833_fg_set_vempty(fuelgauge, VEMPTY_MODE_HW);
				}
			}

			fg_vcell = max77833_get_fuelgauge_value(fuelgauge, MAX77833_FG_VOLTAGE);
			avg_current = max77833_get_fuelgauge_value(fuelgauge, MAX77833_FG_CURRENT_AVG);
			if ((!fuelgauge->is_charging || (fg_vcell < 3000 && avg_current < 0)) &&
			    fuelgauge->vempty_mode == VEMPTY_MODE_SW_VALERT && !lpcharge) {
				pr_info("%s : SW V EMPTY. Decrease SOC\n", __func__);
				val->intval = 0;
			} else if ((fuelgauge->vempty_mode == VEMPTY_MODE_SW_RECOVERY) &&
				   (val->intval == fuelgauge->capacity_old)) {
				fuelgauge->vempty_mode = VEMPTY_MODE_SW;
			}

			/* check whether doing the wake_unlock */
			if ((val->intval > fuelgauge->pdata->fuel_alert_soc) &&
			    fuelgauge->is_fuel_alerted) {
				wake_unlock(&fuelgauge->fuel_salrt_wake_lock);
				max77833_fg_fuelalert_init(fuelgauge,
					  fuelgauge->pdata->fuel_alert_soc,
					  fuelgauge->battery_data->sw_v_empty_vol);
			}

			/* (Only for atomic capacity)
			 * In initial time, capacity_old is 0.
			 * and in resume from sleep,
			 * capacity_old is too different from actual soc.
			 * should update capacity_old
			 * by val->intval in booting or resume.
			 */
			if ((fuelgauge->initial_update_of_soc) &&
				(fuelgauge->vempty_mode == VEMPTY_MODE_HW)) {
				/* updated old capacity */
				fuelgauge->capacity_old = val->intval;
				fuelgauge->initial_update_of_soc = false;
				break;
			}

			if (fuelgauge->pdata->capacity_calculation_type &
			    (SEC_FUELGAUGE_CAPACITY_TYPE_ATOMIC |
			     SEC_FUELGAUGE_CAPACITY_TYPE_SKIP_ABNORMAL))
				max77833_fg_get_atomic_capacity(fuelgauge, val);
		}
		break;
		/* Battery Temperature */
	case POWER_SUPPLY_PROP_TEMP:
		/* Target Temperature */
	case POWER_SUPPLY_PROP_TEMP_AMBIENT:
		val->intval = max77833_get_fuelgauge_value(fuelgauge,
							   MAX77833_FG_TEMPERATURE);
		break;
#if defined(CONFIG_EN_OOPS)
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		return -ENODATA;
#endif
	case POWER_SUPPLY_PROP_ENERGY_FULL:
		{
			int fullcap = max77833_get_fuelgauge_value(fuelgauge, MAX77833_FG_FULLCAPNOM);
			val->intval = fullcap * 100 / fuelgauge->battery_data->Capacity;
			pr_info("%s: asoc(%d), fullcap(0x%x)\n",
				__func__, val->intval, fullcap);
#if !defined(CONFIG_SEC_FACTORY)
			max77833_fg_periodic_read(fuelgauge);
#endif
		}
		break;
	case POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN:
		val->intval = fuelgauge->capacity_max;
		break;
	case POWER_SUPPLY_PROP_TIME_TO_FULL_NOW:
		val->intval = calc_ttf(fuelgauge, val);
		break;
	case POWER_SUPPLY_PROP_POWER_NOW:
		switch (val->intval) {
		case SEC_BATTERY_CURRENT_UA:
			val->intval =
				max77833_fg_read_isys_current(fuelgauge,
							SEC_BATTERY_CURRENT_UA);
			break;
		case SEC_BATTERY_CURRENT_MA:
		default:
			fuelgauge->isys_current_now = val->intval =
				max77833_fg_read_isys_current(fuelgauge,
							SEC_BATTERY_CURRENT_MA);
			break;
		}
		break;
	case POWER_SUPPLY_PROP_POWER_AVG:
		switch (val->intval) {
		case SEC_BATTERY_CURRENT_UA:
			val->intval =
				max77833_fg_read_avg_isys_current(fuelgauge,
							SEC_BATTERY_CURRENT_UA);
			break;
		case SEC_BATTERY_CURRENT_MA:
		default:
			fuelgauge->isys_current_avg = val->intval =
				max77833_fg_read_avg_isys_current(fuelgauge,
							SEC_BATTERY_CURRENT_MA);
			break;
		}
		break;
#if defined(CONFIG_BATTERY_AGE_FORECAST)
	case POWER_SUPPLY_PROP_CAPACITY_LEVEL:
		return -ENODATA;
#endif
	case POWER_SUPPLY_PROP_FILTER_CFG:
		{
			u16 status_data;
			max77833_read_fg(fuelgauge->i2c, FILTER_CFG_REG, &status_data);
			val->intval = status_data;
			pr_debug("%s: FilterCFG=0x%04X\n", __func__, status_data);
			break;
		}
	default:
		return -EINVAL;
	}
	return 0;
}

static int max77833_fg_set_property(struct power_supply *psy,
			     enum power_supply_property psp,
			     const union power_supply_propval *val)
{
	struct max77833_fuelgauge_data *fuelgauge =
		container_of(psy, struct max77833_fuelgauge_data, psy_fg);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		if (fuelgauge->pdata->capacity_calculation_type &
			SEC_FUELGAUGE_CAPACITY_TYPE_DYNAMIC_SCALE) {
#if defined(CONFIG_AFC_CHARGER_MODE)
		max77833_fg_calculate_dynamic_scale(fuelgauge, val->intval);
#else
		max77833_fg_calculate_dynamic_scale(fuelgauge, 100);
#endif
		}
		break;
#if defined(CONFIG_EN_OOPS)
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		max77833_set_full_value(fuelgauge, val->intval);
		break;
#endif
	case POWER_SUPPLY_PROP_ONLINE:
		fuelgauge->cable_type = val->intval;
		if (val->intval == POWER_SUPPLY_TYPE_BATTERY) {
			fuelgauge->is_charging = false;
		} else {
			fuelgauge->is_charging = true;

			/* enable alert */
			if (fuelgauge->vempty_mode >= VEMPTY_MODE_SW_VALERT) {
				max77833_fg_set_vempty(fuelgauge, VEMPTY_MODE_HW);
				fuelgauge->initial_update_of_soc = true;
				max77833_fg_fuelalert_init(fuelgauge,
							   fuelgauge->pdata->fuel_alert_soc,
							   fuelgauge->battery_data->sw_v_empty_vol);
			}

			if (fuelgauge->info.is_low_batt_alarm) {
				pr_info("%s: Reset low_batt_alarm\n",
					 __func__);
				fuelgauge->info.is_low_batt_alarm = false;
			}

			max77833_reset_low_batt_comp_cnt(fuelgauge);
		}
		break;
		/* Battery Temperature */
	case POWER_SUPPLY_PROP_CAPACITY:
		if (val->intval == SEC_FUELGAUGE_CAPACITY_TYPE_RESET) {
			fuelgauge->initial_update_of_soc = true;
			if (!max77833_fg_reset(fuelgauge))
				return -EINVAL;
			else
				break;
		}
	case POWER_SUPPLY_PROP_TEMP:
		if (!fuelgauge->low_temp_compensation_en &&
		    (val->intval <= (int)fuelgauge->low_temp_limit)) {
			max77833_fg_low_temp_compensation(fuelgauge, true);
			fuelgauge->low_temp_compensation_en = true;
		} else if (fuelgauge->low_temp_compensation_en &&
			   (val->intval >= (int)fuelgauge->low_temp_recovery)) {
			max77833_fg_low_temp_compensation(fuelgauge, false);
			fuelgauge->low_temp_compensation_en = false;
		}
		max77833_fg_write_temp(fuelgauge, val->intval);
		max77833_fg_check_qrtable(fuelgauge);
		break;
	case POWER_SUPPLY_PROP_TEMP_AMBIENT:
		break;
	case POWER_SUPPLY_PROP_ENERGY_NOW:
		max77833_fg_reset_capacity_by_jig_connection(fuelgauge);
		break;
	case POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN:
		pr_info("%s: capacity_max changed, %d -> %d\n",
			__func__, fuelgauge->capacity_max, val->intval);
		fuelgauge->capacity_max = max77833_fg_check_capacity_max(fuelgauge, val->intval);
		fuelgauge->initial_update_of_soc = true;
		break;
	case POWER_SUPPLY_PROP_POWER_NOW:
		break;
	case POWER_SUPPLY_PROP_POWER_AVG:
		break;
#if defined(CONFIG_BATTERY_AGE_FORECAST)
	case POWER_SUPPLY_PROP_CAPACITY_LEVEL:
		{
			u16 reg_fullsocthr;
			int val_soc = val->intval;
			if (val->intval > fuelgauge->pdata->full_condition_soc ||
				val->intval <= (fuelgauge->pdata->full_condition_soc - 10)) {
				pr_info("%s: abnormal value(%d). so thr is changed to default(%d)\n",
					__func__, val->intval, fuelgauge->pdata->full_condition_soc);
				val_soc = fuelgauge->pdata->full_condition_soc;
			}

			reg_fullsocthr = val_soc * 256;
			if (max77833_write_fg(fuelgauge->i2c, FULLSOCTHR_REG, reg_fullsocthr) < 0) {
				pr_err("%s: Failed to write FULLSOCTHR_REG\n", __func__);
			} else {
				max77833_read_fg(fuelgauge->i2c, FULLSOCTHR_REG, &reg_fullsocthr);
				pr_info("%s: FullSOCThr %d%%(0x%04X)\n", __func__, val_soc, reg_fullsocthr);
			}
		}
		break;
#endif
	case POWER_SUPPLY_PROP_FILTER_CFG:
		{
			u16 status_data;
			/* Set FilterCFG */
			max77833_read_fg(fuelgauge->i2c, FILTER_CFG_REG, &status_data);
			pr_debug("%s: FilterCFG=0x%04X\n", __func__, status_data);
			status_data &= ~0xF;
			status_data |= (val->intval & 0xF);
			max77833_write_fg(fuelgauge->i2c, FILTER_CFG_REG, status_data);

			max77833_read_fg(fuelgauge->i2c, FILTER_CFG_REG, &status_data);
			pr_debug("%s: FilterCFG=0x%04X\n", __func__, status_data);
			break;
		}
	default:
		return -EINVAL;
	}
	return 0;
}

static void max77833_fg_salrt_irq_work(struct work_struct *work)
{
	struct max77833_fuelgauge_data *fuelgauge =
		container_of(work, struct max77833_fuelgauge_data, salrt_irq_work.work);

	/* process for fuel gauge chip */
	max77833_fg_fuelalert_process(fuelgauge);

	wake_unlock(&fuelgauge->fuel_salrt_wake_lock);
}

static void max77833_fg_valrt_irq_work(struct work_struct *work)
{
	struct max77833_fuelgauge_data *fuelgauge =
		container_of(work, struct max77833_fuelgauge_data, valrt_irq_work.work);

	/* process for fuel gauge chip */
	max77833_fg_fuelalert_process(fuelgauge);

	wake_unlock(&fuelgauge->fuel_valrt_wake_lock);
}

static irqreturn_t max77833_fg_salrt_irq_thread(int irq, void *irq_data)
{
	struct max77833_fuelgauge_data *fuelgauge = irq_data;

	max77833_update_reg(fuelgauge->pmic, MAX77833_PMIC_REG_FG_INT_MASK,
			    MAX77833_FG_SMN_IM, MAX77833_FG_SMN_IM);

	pr_info("%s\n", __func__);

	if (fuelgauge->is_fuel_alerted) {
		return IRQ_HANDLED;
	} else {
		wake_lock(&fuelgauge->fuel_salrt_wake_lock);
		fuelgauge->is_fuel_alerted = true;
		schedule_delayed_work(&fuelgauge->salrt_irq_work, 0);
	}

	return IRQ_HANDLED;
}

static irqreturn_t max77833_fg_valrt_irq_thread(int irq, void *irq_data)
{
	struct max77833_fuelgauge_data *fuelgauge = irq_data;

	max77833_update_reg(fuelgauge->pmic, MAX77833_PMIC_REG_FG_INT_MASK,
			    MAX77833_FG_VMN_IM, MAX77833_FG_VMN_IM);

	pr_info("%s\n", __func__);

	if (fuelgauge->is_fuel_alerted) {
		return IRQ_HANDLED;
	} else {
		wake_lock(&fuelgauge->fuel_valrt_wake_lock);
		fuelgauge->is_fuel_alerted = true;
		schedule_delayed_work(&fuelgauge->valrt_irq_work, 0);
	}

	return IRQ_HANDLED;
}

static ssize_t max77833_fuelgauge_debugfs_write(struct file *file, const char __user *ubuf,
				   size_t count, loff_t *ppos)
{
	struct seq_file *s = file->private_data;
	struct max77833_fuelgauge_data *fuelgauge = s->private;

	char _buf[16], *buf = _buf;
	unsigned long reg, value;
	int ret;

	pr_err("%s: \n", __func__);

	if (count > sizeof(_buf))
		return -EINVAL;

	if (copy_from_user(buf, ubuf, count))
		return -EFAULT;

	buf[sizeof(_buf) - 1] = '\0';
	pr_err("%s: %s\n", __func__, buf);
	buf[6] = 0;
	buf[13] = 0;
	ret = kstrtoul(buf, 0, &reg);
	if (ret) {
		pr_err("%s: reg read err %d\n", __func__, ret);
		return ret;
	}

	ret = kstrtoul(buf+7, 0, &value);
	if (ret) {
		pr_err("%s: value read err %d\n", __func__, ret);
		return ret;
	}
	max77833_write_fg(fuelgauge->i2c, (u16)reg, (u16)value);
	pr_err("%s: reg: 0x%x, value: 0x%x\n", __func__, (int)reg, (int)value);

	return count;
}

static int max77833_fuelgauge_debugfs_show(struct seq_file *s, void *data)
{
	struct max77833_fuelgauge_data *fuelgauge = s->private;
	u16 reg_data;
	u16 reg_addr;
	seq_printf(s, "MAX77833 FUELGAUGE IC :\n");
	seq_printf(s, "===================\n");
	/* 0x0000~0x009E : 2 step */
	reg_addr = 0x0000;
	while (reg_addr <= 0x009E)
	{
		if (!(reg_addr & 0xF))
			seq_printf(s, "\n 0x%04x : ", reg_addr);
		max77833_read_fg(fuelgauge->i2c, reg_addr, &reg_data);
		seq_printf(s, "%04xh,", reg_data);
		reg_addr += 0x02;
	}
	/* 0x0160~0x017E : 2 step */
	reg_addr = 0x0160;
	while (reg_addr <= 0x017E)
	{
		if (!(reg_addr & 0xF))
			seq_printf(s, "\n 0x%04x : ", reg_addr);
		max77833_read_fg(fuelgauge->i2c, reg_addr, &reg_data);
		seq_printf(s,  "%04xh,", reg_data);
		reg_addr += 0x02;
	}
	/* 0x01A0~0x01BE : 2 step */
	reg_addr = 0x01A0;
	while (reg_addr <= 0x01BE)
	{
		if (!(reg_addr & 0xF))
			seq_printf(s, "\n 0x%04x : ", reg_addr);
		max77833_read_fg(fuelgauge->i2c, reg_addr, &reg_data);
		seq_printf(s,  "%04xh,", reg_data);
		reg_addr += 0x02;
	}
	/* 0x07C0~0x07EE : 2 step */
	reg_addr = 0x07C0;
	while (reg_addr <= 0x07EE)
	{
		if (!(reg_addr & 0xF))
			seq_printf(s, "\n 0x%04x : ", reg_addr);
		max77833_read_fg(fuelgauge->i2c, reg_addr, &reg_data);
		seq_printf(s,  "%04xh,", reg_data);
		reg_addr += 0x02;
	}
	/* 0x01D0~0x01D4 : 2 step */
	reg_addr = 0x01D0;
	while (reg_addr <= 0x01D4)
	{
		if (!(reg_addr & 0xF))
			seq_printf(s, "\n 0x%04x : ", reg_addr);
		max77833_read_fg(fuelgauge->i2c, reg_addr, &reg_data);
		seq_printf(s,  "%04xh,", reg_data);
		reg_addr += 0x02;
	}
	/* 0x07F6~0x07FE : 2 step */
	reg_addr = 0x07F6;
	seq_printf(s, "\n 0x%04x : ", reg_addr);
	while (reg_addr <= 0x07FE)
	{
		max77833_read_fg(fuelgauge->i2c, reg_addr, &reg_data);
		seq_printf(s,  "%04xh,", reg_data);
		reg_addr += 0x02;
	}
	seq_printf(s, "\n");

	return 0;
}

static int max77833_fuelgauge_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, max77833_fuelgauge_debugfs_show, inode->i_private);
}

static const struct file_operations max77833_fuelgauge_debugfs_fops = {
	.open           = max77833_fuelgauge_debugfs_open,
	.read           = seq_read,
	.write		= max77833_fuelgauge_debugfs_write,
	.llseek         = seq_lseek,
	.release        = single_release,
};

#ifdef CONFIG_OF
static int max77833_fuelgauge_parse_dt(struct max77833_fuelgauge_data *fuelgauge)
{
	struct device_node *np = of_find_node_by_name(NULL, "max77833-fuelgauge");
	sec_fuelgauge_platform_data_t *pdata = fuelgauge->pdata;
	int ret;
	int i, len;
	const u32 *p;

	/* reset, irq gpio info */
	if (np == NULL) {
		pr_err("%s np NULL\n", __func__);
	} else {
		ret = of_property_read_u32(np, "fuelgauge,capacity_max",
				&pdata->capacity_max);
		if (ret < 0)
			pr_err("%s error reading capacity_max %d\n", __func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,capacity_max_margin",
				&pdata->capacity_max_margin);
		if (ret < 0)
			pr_err("%s error reading capacity_max_margin %d\n", __func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,capacity_min",
				&pdata->capacity_min);
		if (ret < 0)
			pr_err("%s error reading capacity_min %d\n", __func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,capacity_calculation_type",
				&pdata->capacity_calculation_type);
		if (ret < 0)
			pr_err("%s error reading capacity_calculation_type %d\n",
					__func__, ret);
		ret = of_property_read_u32(np, "fuelgauge,fuel_alert_soc",
				&pdata->fuel_alert_soc);
		if (ret < 0)
			pr_err("%s error reading pdata->fuel_alert_soc %d\n",
					__func__, ret);

		pdata->repeated_fuelalert = of_property_read_bool(np,
				"fuelgauge,repeated_fuelalert");

		fuelgauge->using_temp_compensation = of_property_read_bool(np,
						   "fuelgauge,using_temp_compensation");
		if (fuelgauge->using_temp_compensation) {
			ret = of_property_read_u32(np, "fuelgauge,low_temp_limit",
						   &fuelgauge->low_temp_limit);
			if (ret < 0)
				pr_err("%s error reading low temp limit %d\n", __func__, ret);

			ret = of_property_read_u32(np, "fuelgauge,low_temp_recovery",
						   &fuelgauge->low_temp_recovery);
			if (ret < 0)
				pr_err("%s error reading low temp recovery %d\n", __func__, ret);

			pr_info("%s : LOW TEMP LIMIT(%d) RECOVERY(%d)\n",
				__func__, fuelgauge->low_temp_limit, fuelgauge->low_temp_recovery);
		}

		fuelgauge->using_hw_vempty = of_property_read_bool(np,
								   "fuelgauge,using_hw_vempty");
		if (fuelgauge->using_hw_vempty) {
			ret = of_property_read_u32(np, "fuelgauge,v_empty",
						   &fuelgauge->battery_data->V_empty);
			if (ret < 0)
				pr_err("%s error reading v_empty %d\n",
				       __func__, ret);

			ret = of_property_read_u32(np, "fuelgauge,v_empty_origin",
						   &fuelgauge->battery_data->V_empty_origin);
			if(ret < 0)
				pr_err("%s error reading v_empty_origin %d\n",
				       __func__, ret);

			ret = of_property_read_u32(np, "fuelgauge,sw_v_empty_voltage",
						   &fuelgauge->battery_data->sw_v_empty_vol);
			if(ret < 0)
				pr_err("%s error reading sw_v_empty_default_vol %d\n",
					   __func__, ret);
		
			ret = of_property_read_u32(np, "fuelgauge,sw_v_empty_recover_voltage",
						   &fuelgauge->battery_data->sw_v_empty_recover_vol);
			if(ret < 0)
				pr_err("%s error reading sw_v_empty_recover_vol %d\n",
					   __func__, ret);
		
			pr_info("%s : SW V Empty (%d)mV,  SW V Empty recover (%d)mV\n",
				__func__, fuelgauge->battery_data->sw_v_empty_vol, fuelgauge->battery_data->sw_v_empty_recover_vol);

		}

		ret = of_property_read_u32(np, "fuelgauge,qrtable20",
					   &fuelgauge->battery_data->QResidual20);
		if (ret < 0)
			pr_err("%s error reading qrtable20 %d\n",
					__func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,qrtable30",
					   &fuelgauge->battery_data->QResidual30);
		if (ret < 0)
			pr_err("%s error reading qrtabel30 %d\n",
					__func__, ret);

#if defined(CONFIG_EN_OOPS)
		ret = of_property_read_u32(np, "fuelgauge,ichgterm",
					   &fuelgauge->battery_data->ichgterm);
		if (ret < 0)
			pr_err("%s error reading ichgterm %d\n",
					__func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,ichgterm_2nd",
					   &fuelgauge->battery_data->ichgterm_2nd);
		if (ret < 0)
			pr_err("%s error reading ichgterm_2nd %d\n",
					__func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,misccfg",
					   &fuelgauge->battery_data->misccfg);
		if (ret < 0)
			pr_err("%s error reading misccfg %d\n",
					__func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,misccfg_2nd",
					   &fuelgauge->battery_data->misccfg_2nd);
		if (ret < 0)
			pr_err("%s error reading misccfg_2nd %d\n",
					__func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,fullsocthr",
					   &fuelgauge->battery_data->fullsocthr);
		if (ret < 0)
			pr_err("%s error reading fullsocthr %d\n",
					__func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,fullsocthr_2nd",
					   &fuelgauge->battery_data->fullsocthr_2nd);
		if (ret < 0)
			pr_err("%s error reading fullsocthr_2nd %d\n",
					__func__, ret);
#endif

		ret = of_property_read_u32(np, "fuelgauge,capacity",
					   &fuelgauge->battery_data->Capacity);
		if (ret < 0)
			pr_err("%s error reading capacity_calculation_type %d\n",
					__func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,low_battery_comp_voltage",
			   &fuelgauge->battery_data->low_battery_comp_voltage);
		if (ret < 0)
			pr_err("%s error reading capacity_calculation_type %d\n",
					__func__, ret);

		for(i = 0; i < (CURRENT_RANGE_MAX_NUM * MAX77833_TABLE_MAX); i++) {
			ret = of_property_read_u32_index(np,
					 "fuelgauge,low_battery_table",
					 i,
					 &fuelgauge->battery_data->low_battery_table[i/3][i%3]);
			pr_info("[%d]",
				fuelgauge->battery_data->low_battery_table[i/3][i%3]);
			if ((i%3) == 2)
				pr_info("\n");
		}
		p = of_get_property(np, "fuelgauge,cv_data", &len);
		if (p) {
			fuelgauge->cv_data = kzalloc(len,
						  GFP_KERNEL);
			fuelgauge->cv_data_lenth = len / sizeof(struct cv_slope);
			pr_err("%s len: %ld, lenth: %d, %d\n",
					__func__, sizeof(int) * len, len, fuelgauge->cv_data_lenth);
			ret = of_property_read_u32_array(np, "fuelgauge,cv_data",
					 (u32 *)fuelgauge->cv_data, len/sizeof(u32));
			for(i = 0; i < fuelgauge->cv_data_lenth; i++) {
				pr_err("%s  %5d, %5d, %5d\n",
						__func__, fuelgauge->cv_data[i].fg_current,
						fuelgauge->cv_data[i].soc, fuelgauge->cv_data[i].time);
			}
			if (ret) {
				pr_err("%s failed to read fuelgauge->cv_data: %d\n",
						__func__, ret);
				kfree(fuelgauge->cv_data);
				fuelgauge->cv_data = NULL;
			}
		} else {
			pr_err("%s there is not cv_data\n", __func__);
		}

		np = of_find_node_by_name(NULL, "battery");
		ret = of_property_read_u32(np, "battery,thermal_source",
					   &pdata->thermal_source);
		if (ret < 0) {
			pr_err("%s error reading pdata->thermal_source %d\n",
			       __func__, ret);
		}

		p = of_get_property(np, "battery,input_current_limit", &len);
		if (!p)
			return 1;
		len = len / sizeof(u32);

		pdata->charging_current = kzalloc(sizeof(sec_charging_current_t) * len,
						  GFP_KERNEL);

		for(i = 0; i < len; i++) {
			ret = of_property_read_u32_index(np,
				 "battery,input_current_limit", i,
				 &pdata->charging_current[i].input_current_limit);
			ret = of_property_read_u32_index(np,
				 "battery,fast_charging_current", i,
				 &pdata->charging_current[i].fast_charging_current);
			ret = of_property_read_u32_index(np,
				 "battery,full_check_current_1st", i,
				 &pdata->charging_current[i].full_check_current_1st);
			ret = of_property_read_u32_index(np,
				 "battery,full_check_current_2nd", i,
				 &pdata->charging_current[i].full_check_current_2nd);
		}

#if defined(CONFIG_BATTERY_AGE_FORECAST)
		ret = of_property_read_u32(np, "battery,full_condition_soc",
			&pdata->full_condition_soc);
		if (ret) {
			pdata->full_condition_soc = 93;
			pr_info("%s : Full condition soc is Empty\n", __func__);
		}
#endif

		pr_info("%s capacity_max: %d\n"
			"qrtable20: 0x%x, qrtable30 : 0x%x\n"
			"capacity_max_margin: %d, capacity_min: %d\n"
			"calculation_type: 0x%x, fuel_alert_soc: %d,\n"
			"repeated_fuelalert: %d\n",
			__func__, pdata->capacity_max,
			fuelgauge->battery_data->QResidual20,
			fuelgauge->battery_data->QResidual30,
			pdata->capacity_max_margin, pdata->capacity_min,
			pdata->capacity_calculation_type, pdata->fuel_alert_soc,
			pdata->repeated_fuelalert);
	}

	pr_info("[%s][%d][%d]\n",
		__func__, fuelgauge->battery_data->Capacity,
	        fuelgauge->battery_data->low_battery_comp_voltage);

	return 0;
}
#endif

static int __devinit max77833_fuelgauge_probe(struct platform_device *pdev)
{
	struct max77833_dev *max77833 = dev_get_drvdata(pdev->dev.parent);
	struct max77833_platform_data *pdata = dev_get_platdata(max77833->dev);
	struct max77833_fuelgauge_data *fuelgauge;
	int ret = 0;
	union power_supply_propval raw_soc_val;

	pr_info("%s: MAX77833 Fuelgauge Driver Loading\n", __func__);

	fuelgauge = kzalloc(sizeof(*fuelgauge), GFP_KERNEL);
	if (!fuelgauge)
		return -ENOMEM;

	pdata->fuelgauge_data = kzalloc(sizeof(sec_fuelgauge_platform_data_t), GFP_KERNEL);
	if (!pdata->fuelgauge_data) {
		ret = -ENOMEM;
		goto err_free;
	}

	mutex_init(&fuelgauge->fg_lock);

	fuelgauge->dev = &pdev->dev;
	fuelgauge->pdata = pdata->fuelgauge_data;
	fuelgauge->i2c = max77833->fuelgauge;
	fuelgauge->pmic = max77833->i2c;
	fuelgauge->max77833_pdata = pdata;

#if defined(CONFIG_OF)
	fuelgauge->battery_data = kzalloc(sizeof(struct battery_data_t),
					  GFP_KERNEL);
	if(!fuelgauge->battery_data) {
		pr_err("Failed to allocate memory\n");
		ret = -ENOMEM;
		goto err_pdata_free;
	}
	ret = max77833_fuelgauge_parse_dt(fuelgauge);
	if (ret < 0) {
		pr_err("%s not found charger dt! ret[%d]\n",
		       __func__, ret);
	}
#endif

	platform_set_drvdata(pdev, fuelgauge);

	fuelgauge->psy_fg.name		= "max77833-fuelgauge";
	fuelgauge->psy_fg.type		= POWER_SUPPLY_TYPE_UNKNOWN;
	fuelgauge->psy_fg.get_property	= max77833_fg_get_property;
	fuelgauge->psy_fg.set_property	= max77833_fg_set_property;
	fuelgauge->psy_fg.properties	= max77833_fuelgauge_props;
	fuelgauge->psy_fg.num_properties =
		ARRAY_SIZE(max77833_fuelgauge_props);
	fuelgauge->capacity_max = fuelgauge->pdata->capacity_max;
	raw_soc_val.intval = max77833_get_fuelgauge_value(fuelgauge, MAX77833_FG_RAW_SOC) / 10;

	if (raw_soc_val.intval > fuelgauge->capacity_max)
		max77833_fg_calculate_dynamic_scale(fuelgauge, 100);

	(void) debugfs_create_file("max77833-fuelgauge-regs",
		S_IRUGO, NULL, (void *)fuelgauge, &max77833_fuelgauge_debugfs_fops);

	if (!max77833_fg_init(fuelgauge)) {
		pr_err("%s: Failed to Initialize Fuelgauge\n", __func__);
		goto err_data_free;
	}
	
	/* SW/HW init code. SW/HW V Empty mode must be opposite ! */
	fuelgauge->temperature = 300; /* default value */
	pr_info("%s: SW/HW V empty init \n", __func__);
	max77833_fg_set_vempty(fuelgauge, VEMPTY_MODE_HW);

	ret = power_supply_register(&pdev->dev, &fuelgauge->psy_fg);
	if (ret) {
		pr_err("%s: Failed to Register psy_fg\n", __func__);
		goto err_data_free;
	}

	if (fuelgauge->pdata->fuel_alert_soc >= 0 || fuelgauge->battery_data->sw_v_empty_vol >= 0) {
		max77833_fg_fuelalert_init(fuelgauge,
				       fuelgauge->pdata->fuel_alert_soc,
				       fuelgauge->battery_data->sw_v_empty_vol);
	}
	fuelgauge->is_fuel_alerted = false;

	if (fuelgauge->pdata->fuel_alert_soc >= 0) {
		fuelgauge->fg_smn_irq = pdata->irq_base + MAX77833_FG_IRQ_SMN_I;
		wake_lock_init(&fuelgauge->fuel_salrt_wake_lock,
			       WAKE_LOCK_SUSPEND, "fuel_alerted_soc");
		if (fuelgauge->fg_smn_irq) {
			INIT_DELAYED_WORK(&fuelgauge->salrt_irq_work, max77833_fg_salrt_irq_work);
			ret = request_threaded_irq(fuelgauge->fg_smn_irq,
					   NULL, max77833_fg_salrt_irq_thread,
					   IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
					   "fuelgauge-soc-irq", fuelgauge);
			if (ret) {
				pr_err("%s: Failed to Request IRQ\n", __func__);
				goto err_supply_unreg;
			}
		} else {
			pr_err("%s: Failed to Initialize Fuel-alert\n",
			       __func__);
			goto err_supply_unreg;
		}
	}

	if (fuelgauge->battery_data->sw_v_empty_vol >= 0) {
		fuelgauge->fg_vmn_irq = pdata->irq_base + MAX77833_FG_IRQ_VMN_I;
		wake_lock_init(&fuelgauge->fuel_valrt_wake_lock,
			       WAKE_LOCK_SUSPEND, "fuel_alerted_vol");
		if (fuelgauge->fg_vmn_irq) {
			INIT_DELAYED_WORK(&fuelgauge->valrt_irq_work, max77833_fg_valrt_irq_work);
			ret = request_threaded_irq(fuelgauge->fg_vmn_irq,
					   NULL, max77833_fg_valrt_irq_thread,
					   IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
					   "fuelgauge-vol-irq", fuelgauge);
			if (ret) {
				pr_err("%s: Failed to Request IRQ\n", __func__);
				goto err_supply_unreg;
			}
		}
	} else {
		pr_err("%s: Failed to Initialize Fuel-alert\n",
		       __func__);
		goto err_supply_unreg;
	}

	fuelgauge->initial_update_of_soc = true;
	fuelgauge->low_temp_compensation_en = false;

	pr_info("%s: MAX77833 Fuelgauge Driver Loaded\n", __func__);
	return 0;

err_supply_unreg:
	power_supply_unregister(&fuelgauge->psy_fg);
err_data_free:
#if defined(CONFIG_OF)
	kfree(fuelgauge->battery_data);
#endif
err_pdata_free:
	kfree(pdata->fuelgauge_data);
	mutex_destroy(&fuelgauge->fg_lock);
err_free:
	kfree(fuelgauge);

	return ret;
}

static int __devexit max77833_fuelgauge_remove(struct platform_device *pdev)
{
	struct max77833_fuelgauge_data *fuelgauge =
		platform_get_drvdata(pdev);

	if (fuelgauge->pdata->fuel_alert_soc >= 0)
		wake_lock_destroy(&fuelgauge->fuel_salrt_wake_lock);

	if (fuelgauge->battery_data->sw_v_empty_vol >= 0)
		wake_lock_destroy(&fuelgauge->fuel_valrt_wake_lock);

	return 0;
}

static int max77833_fuelgauge_suspend(struct device *dev)
{
	return 0;
}

static int max77833_fuelgauge_resume(struct device *dev)
{
	struct max77833_fuelgauge_data *fuelgauge = dev_get_drvdata(dev);

	fuelgauge->initial_update_of_soc = true;

	return 0;
}

static void max77833_fuelgauge_shutdown(struct device *dev)
{
	struct max77833_fuelgauge_data *fuelgauge = dev_get_drvdata(dev);

	if (fuelgauge->using_hw_vempty)
		max77833_fg_set_vempty(fuelgauge, VEMPTY_MODE_HW);
}

static SIMPLE_DEV_PM_OPS(max77833_fuelgauge_pm_ops, max77833_fuelgauge_suspend,
			 max77833_fuelgauge_resume);

static struct platform_driver max77833_fuelgauge_driver = {
	.driver = {
		   .name = "max77833-fuelgauge",
		   .owner = THIS_MODULE,
#ifdef CONFIG_PM
		   .pm = &max77833_fuelgauge_pm_ops,
#endif
		.shutdown = max77833_fuelgauge_shutdown,
	},
	.probe	= max77833_fuelgauge_probe,
	.remove	= __devexit_p(max77833_fuelgauge_remove),
};

static int __init max77833_fuelgauge_init(void)
{
	pr_info("%s: \n", __func__);
	return platform_driver_register(&max77833_fuelgauge_driver);
}

static void __exit max77833_fuelgauge_exit(void)
{
	platform_driver_unregister(&max77833_fuelgauge_driver);
}
module_init(max77833_fuelgauge_init);
module_exit(max77833_fuelgauge_exit);

MODULE_DESCRIPTION("Samsung MAX77833 Fuel Gauge Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
