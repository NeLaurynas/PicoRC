// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#include "main_engine.h"

#include <hardware/dma.h>
#include <hardware/gpio.h>
#include <hardware/pwm.h>
#include <pico/time.h>

#include "utils.h"
#include "defines/config.h"

static u8 slice1 = 0;
static u8 slice2 = 0;
static u8 channel1 = 0;
static u8 channel2 = 0;
static u32 buffer[1] = { 0 };

void main_engine_init() {
	gpio_init(MOD_ENGINE_MAIN_ENABLE1);
	gpio_init(MOD_ENGINE_MAIN_ENABLE2);
	gpio_init(MOD_ENGINE_MAIN_ENABLE3);
	gpio_init(MOD_ENGINE_MAIN_ENABLE4);
	gpio_set_dir(MOD_ENGINE_MAIN_ENABLE1, true);
	gpio_set_dir(MOD_ENGINE_MAIN_ENABLE2, true);
	gpio_set_dir(MOD_ENGINE_MAIN_ENABLE3, true);
	gpio_set_dir(MOD_ENGINE_MAIN_ENABLE4, true);
	gpio_set_function(MOD_ENGINE_MAIN_PWM1, GPIO_FUNC_PWM);
	gpio_set_function(MOD_ENGINE_MAIN_PWM2, GPIO_FUNC_PWM);

	slice1 = pwm_gpio_to_slice_num(MOD_ENGINE_MAIN_PWM1);
	slice2 = pwm_gpio_to_slice_num(MOD_ENGINE_MAIN_PWM2);
	channel1 = pwm_gpio_to_channel(MOD_ENGINE_MAIN_PWM1);
	channel2 = pwm_gpio_to_channel(MOD_ENGINE_MAIN_PWM2);

	// init PWM
	auto pwm_c1 = pwm_get_default_config();
	pwm_c1.top = 10000;
	pwm_init(slice1, &pwm_c1, false);
	pwm_set_clkdiv(slice1, 20.f); // 3.f for 5 khz frequency (2.f for 7.5 khz 1.f for 15 khz)
	pwm_set_phase_correct(slice1, false);
	pwm_set_enabled(slice1, true);

	auto pwm_c2 = pwm_get_default_config();
	pwm_c2.top = 10000;
	pwm_init(slice2, &pwm_c2, false);
	pwm_set_clkdiv(slice2, 20.f);
	pwm_set_phase_correct(slice2, false);
	pwm_set_enabled(slice2, true);
	sleep_ms(1);

	// init DMA
	if (dma_channel_is_claimed(MOD_ENGINE_MAIN_DMA_CH1)) utils_error_mode(10);
	dma_channel_claim(MOD_ENGINE_MAIN_DMA_CH1);
	auto dma_cc_c1 = dma_channel_get_default_config(MOD_ENGINE_MAIN_DMA_CH1);
	channel_config_set_transfer_data_size(&dma_cc_c1, DMA_SIZE_32);
	channel_config_set_read_increment(&dma_cc_c1, false);
	channel_config_set_write_increment(&dma_cc_c1, false);
	channel_config_set_dreq(&dma_cc_c1, DREQ_FORCE);
	dma_channel_configure(MOD_ENGINE_MAIN_DMA_CH1, &dma_cc_c1, &pwm_hw->slice[slice1].cc, buffer, 1, false);

	if (dma_channel_is_claimed(MOD_ENGINE_MAIN_DMA_CH2)) utils_error_mode(11);
	dma_channel_claim(MOD_ENGINE_MAIN_DMA_CH2);
	auto dma_cc_c2 = dma_channel_get_default_config(MOD_ENGINE_MAIN_DMA_CH2);
	channel_config_set_transfer_data_size(&dma_cc_c2, DMA_SIZE_32);
	channel_config_set_read_increment(&dma_cc_c2, false);
	channel_config_set_write_increment(&dma_cc_c2, false);
	channel_config_set_dreq(&dma_cc_c2, DREQ_FORCE);
	dma_channel_configure(MOD_ENGINE_MAIN_DMA_CH2, &dma_cc_c2, &pwm_hw->slice[slice2].cc, buffer, 1, false);
	sleep_ms(1);
}

static void set_motor_ctrl(const bool neg, const i16 val, const u16 pwm, const u8 pin1, const u8 pin2, const u8 dma_ch) {
	const auto pwm_channel = dma_ch == MOD_ENGINE_MAIN_DMA_CH1 ? channel1 : channel2;
	buffer[0] = (pwm_channel == 1)
		? (buffer[0] & 0x0000FFFF) | ((u32)pwm << 16)
		: (buffer[0] & 0xFFFF0000) | (pwm & 0xFFFF);

	if (neg) {
		gpio_put(pin1, false);
		gpio_put(pin2, true);
		dma_channel_transfer_from_buffer_now(dma_ch, buffer, 1);
	} else {
		gpio_put(pin1, val != 0);
		gpio_put(pin2, false);
		dma_channel_transfer_from_buffer_now(dma_ch, buffer, 1);
	}
}

static void adjust_pwm(u16 *pwm) {
	if (*pwm <= 4000) {
		*pwm = *pwm * 1.15;  // Boost by 15%
	} else if (*pwm <= 5000) {
		*pwm = *pwm * (1.15 - 0.05 * ((*pwm - 4000) / 1000.0));  // Gradual reduction from 1.15x to 1.05x
	} else if (*pwm <= 6000) {
		*pwm = *pwm * (1.05 - 0.03 * ((*pwm - 5000) / 1000.0));  // Gradual reduction from 1.05x to 1.02x
	} else if (*pwm <= 9000) {
		*pwm = *pwm * (1.02 - 0.27 * ((*pwm - 6000) / 3000.0));  // Gradual decrease to 0.75x
	}
}

void main_engine_vals(const i16 left, const i16 right) {
	const bool neg_l = left < 0;
	const bool neg_r = right < 0;
	u16 pwm_left = utils_scaled_pwm_percentage(left, MOD_ENGINE_XY_DEAD_ZONE, MOD_ENGINE_XY_MAX) * 100;
	u16 pwm_right = utils_scaled_pwm_percentage(right, MOD_ENGINE_XY_DEAD_ZONE, MOD_ENGINE_XY_MAX) * 100;
	adjust_pwm(&pwm_left);
	adjust_pwm(&pwm_right);
	// utils_printf("R: %d, pwm: %d\n", right, pwm_right);

	set_motor_ctrl(neg_l, left, pwm_left, MOD_ENGINE_MAIN_ENABLE1, MOD_ENGINE_MAIN_ENABLE2, MOD_ENGINE_MAIN_DMA_CH1);
	set_motor_ctrl(neg_r, right, pwm_right, MOD_ENGINE_MAIN_ENABLE3, MOD_ENGINE_MAIN_ENABLE4, MOD_ENGINE_MAIN_DMA_CH2);
}
