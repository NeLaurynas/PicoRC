// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#include "engines.h"

#include <hardware/dma.h>
#include <hardware/gpio.h>
#include <hardware/pwm.h>
#include <pico/time.h>

#include "utils.h"
#include "defines/config.h"

#define PWM_DIVIDER 20.0f

static u16 slice1, slice2 = 0;
static u16 channel1, channel2 = 0;
static u16 buffers[2] = { 0 };

void engines_init() {
	// mask maybe next time, no?
	gpio_init(MOD_ENGINE_ENABLE1);
	gpio_init(MOD_ENGINE_ENABLE2);
	gpio_init(MOD_ENGINE_ENABLE3);
	gpio_init(MOD_ENGINE_ENABLE4);
	gpio_set_dir(MOD_ENGINE_ENABLE1, true);
	gpio_set_dir(MOD_ENGINE_ENABLE2, true);
	gpio_set_dir(MOD_ENGINE_ENABLE3, true);
	gpio_set_dir(MOD_ENGINE_ENABLE4, true);
	gpio_set_function(MOD_ENGINE_PWM1, GPIO_FUNC_PWM);
	gpio_set_function(MOD_ENGINE_PWM1, GPIO_FUNC_PWM);

	slice1 = pwm_gpio_to_slice_num(MOD_ENGINE_PWM1);
	slice2 = pwm_gpio_to_slice_num(MOD_ENGINE_PWM2);
	channel1 = pwm_gpio_to_channel(MOD_ENGINE_PWM1);
	channel2 = pwm_gpio_to_channel(MOD_ENGINE_PWM2);

	// init PWM 1
	auto pwm_c1 = pwm_get_default_config();
	pwm_c1.top = 10000;
	pwm_init(slice1, &pwm_c1, false);
	pwm_set_clkdiv(slice1, PWM_DIVIDER);
	pwm_set_phase_correct(slice1, false);
	pwm_set_enabled(slice1, true);

	// init PWM 2
	auto pwm_c2 = pwm_get_default_config();
	pwm_c2.top = 10000;
	pwm_init(slice2, &pwm_c2, false);
	pwm_set_clkdiv(slice2, PWM_DIVIDER);
	pwm_set_phase_correct(slice2, false);
	pwm_set_enabled(slice2, true);

	// init DMA
	if (dma_channel_is_claimed(MOD_ENGINE_DMA_CH1)) utils_error_mode(10);
	dma_channel_claim(MOD_ENGINE_DMA_CH1);
	auto dma_c1 = dma_channel_get_default_config(MOD_ENGINE_DMA_CH1);
	channel_config_set_transfer_data_size(&dma_c1, DMA_SIZE_16);
	channel_config_set_read_increment(&dma_c1, false);
	channel_config_set_write_increment(&dma_c1, false);
	channel_config_set_dreq(&dma_c1, DREQ_FORCE);
	dma_channel_configure(MOD_ENGINE_DMA_CH1, &dma_c1, utils_pwm_cc_for_16bit(slice1, channel1), &buffers[0], 1, false);

	if (dma_channel_is_claimed(MOD_ENGINE_DMA_CH2)) utils_error_mode(11);
	dma_channel_claim(MOD_ENGINE_DMA_CH2);
	auto dma_c2 = dma_channel_get_default_config(MOD_ENGINE_DMA_CH2);
	channel_config_set_transfer_data_size(&dma_c2, DMA_SIZE_16);
	channel_config_set_read_increment(&dma_c2, false);
	channel_config_set_write_increment(&dma_c2, false);
	channel_config_set_dreq(&dma_c2, DREQ_FORCE);
	dma_channel_configure(MOD_ENGINE_DMA_CH2, &dma_c2, utils_pwm_cc_for_16bit(slice2, channel2), &buffers[1], 1, false);
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

static void set_motor_ctrl(const bool drive_engine, const i16 val, const u16 pwm) {
	const u8 pin1 = drive_engine ? MOD_ENGINE_ENABLE1 : MOD_ENGINE_ENABLE3;
	const u8 pin2 = drive_engine ? MOD_ENGINE_ENABLE2 : MOD_ENGINE_ENABLE4;
	const u8 dma_ch = drive_engine ? MOD_ENGINE_DMA_CH1 : MOD_ENGINE_DMA_CH2;
	const u16 *buffer = drive_engine ? &buffers[0] : &buffers[1];

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

void engines_drive(const i16 val) {
	u16 pwm = utils_scaled_pwm_percentage(val, TRIG_DEAD_ZONE, TRIG_MAX) * 100;
	adjust_pwm(&pwm);

	set_motor_ctrl(true, val, pwm);
}

void engines_turn(const i16 val) {
	u16 pwm = utils_scaled_pwm_percentage(val, MOD_ENGINE_XY_DEAD_ZONE, MOD_ENGINE_XY_MAX) * 100;
	adjust_pwm(&pwm);

	set_motor_ctrl(false, val, pwm);
}
