// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#include "turret_ctrl.h"

#include <stdlib.h>
#include <hardware/dma.h>
#include <hardware/gpio.h>
#include <hardware/pwm.h>
#include <pico/time.h>

#include "utils.h"
#include "defines/config.h"

static uint slice1 = 0;
static uint channel1 = 0;
static u32 buffer[1] = { 0 };

void turret_ctrl_init() {
	gpio_init(MOD_TURRET_CTRL_ENABLE1);
	gpio_init(MOD_TURRET_CTRL_ENABLE2);
	gpio_init(MOD_TURRET_CTRL_ENABLE3);
	gpio_init(MOD_TURRET_CTRL_ENABLE4);
	gpio_init(MOD_TURRET_CTRL_PWM2);
	gpio_set_dir(MOD_TURRET_CTRL_ENABLE1, true);
	gpio_set_dir(MOD_TURRET_CTRL_ENABLE2, true);
	gpio_set_dir(MOD_TURRET_CTRL_ENABLE3, true);
	gpio_set_dir(MOD_TURRET_CTRL_ENABLE4, true);
	gpio_set_dir(MOD_TURRET_CTRL_PWM2, true);
	gpio_set_function(MOD_TURRET_CTRL_PWM1, GPIO_FUNC_PWM);

	slice1 = pwm_gpio_to_slice_num(MOD_TURRET_CTRL_PWM1);
	channel1 = pwm_gpio_to_channel(MOD_TURRET_CTRL_PWM1);

	// init PWM
	auto pwm_c1 = pwm_get_default_config();
	pwm_c1.top = 10000;
	pwm_init(slice1, &pwm_c1, false);
	pwm_set_clkdiv(slice1, 20.f); // 3.f for 5 khz frequency (2.f for 7.5 khz 1.f for 15 khz)
	pwm_set_phase_correct(slice1, false);
	pwm_set_enabled(slice1, true);
	sleep_ms(1);

	// init DMA
	if (dma_channel_is_claimed(MOD_TURRET_CTRL_DMA_CH)) utils_error_mode(13);
	dma_channel_claim(MOD_TURRET_CTRL_DMA_CH);
	auto dma_cc_c1 = dma_channel_get_default_config(MOD_TURRET_CTRL_DMA_CH);
	channel_config_set_transfer_data_size(&dma_cc_c1, DMA_SIZE_32);
	channel_config_set_read_increment(&dma_cc_c1, false);
	channel_config_set_write_increment(&dma_cc_c1, false);
	channel_config_set_dreq(&dma_cc_c1, DREQ_FORCE);
	dma_channel_configure(MOD_TURRET_CTRL_DMA_CH, &dma_cc_c1, &pwm_hw->slice[slice1].cc, buffer, 1, false);
	sleep_ms(1);
}

static void adjust_pwm(u16 *pwm) {
	if (*pwm <= 4000) {
		*pwm = *pwm * 1.05;
	} else if (*pwm <= 5000) {
		*pwm = *pwm * (1.05 - 0.025 * ((*pwm - 4000) / 1000.0));
	} else if (*pwm <= 6000) {
		*pwm = *pwm * (1.025 - 0.02 * ((*pwm - 5000) / 1000.0)); // Gradual reduction from 1.05x to 1.02x
	} else if (*pwm <= 9000) {
		*pwm = *pwm * (1.005 - 0.27 * ((*pwm - 6000) / 3000.0)); // Gradual decrease to 0.75x
	}
}

static void set_motor_ctrl(const i16 val, const u16 pwm) {
	const u8 pin1 = MOD_TURRET_CTRL_ENABLE1;
	const u8 pin2 = MOD_TURRET_CTRL_ENABLE2;
	const u8 dma_ch = MOD_TURRET_CTRL_DMA_CH;

	buffer[0] = (channel1 == 1)
		? (buffer[0] & 0x0000FFFF) | ((u32)pwm << 16)
		: (buffer[0] & 0xFFFF0000) | (pwm & 0xFFFF);

	if (val < 0) {
		gpio_put(pin1, false);
		gpio_put(pin2, true);
		dma_channel_transfer_from_buffer_now(dma_ch, buffer, 1);
	} else {
		gpio_put(pin1, val != 0);
		gpio_put(pin2, false);
		dma_channel_transfer_from_buffer_now(dma_ch, buffer, 1);
	}
}

void turret_ctrl_rotate(const i16 val) {
	u16 pwm = utils_scaled_pwm_percentage(val, XY_DEAD_ZONE, XY_MAX) * 100;
	adjust_pwm(&pwm);

	set_motor_ctrl(val, pwm);
}

void turret_ctrl_lift(const i16 val) {
	if (abs(val) <= XY_DEAD_ZONE + 200) {
		gpio_put(MOD_TURRET_CTRL_PWM2, false);
		gpio_put(MOD_TURRET_CTRL_ENABLE3, false);
		gpio_put(MOD_TURRET_CTRL_ENABLE4, false);
	} else if (val < 0) {
		gpio_put(MOD_TURRET_CTRL_PWM2, true);
		gpio_put(MOD_TURRET_CTRL_ENABLE3, false);
		gpio_put(MOD_TURRET_CTRL_ENABLE4, true);
	} else {
		gpio_put(MOD_TURRET_CTRL_PWM2, true);
		gpio_put(MOD_TURRET_CTRL_ENABLE3, true);
		gpio_put(MOD_TURRET_CTRL_ENABLE4, false);
	}
}
