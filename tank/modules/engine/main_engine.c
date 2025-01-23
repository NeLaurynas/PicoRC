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

	// code assumes both slices are on the same channel, so it won't work, reconfigure connections
	hard_assert(channel1 == channel2);

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

static void set_motor_ctrl(const i16 val, const u16 pwm, const bool is_left_motor) {
	const u8 pin1 = is_left_motor ? MOD_ENGINE_MAIN_ENABLE1 : MOD_ENGINE_MAIN_ENABLE3;
	const u8 pin2 = is_left_motor ? MOD_ENGINE_MAIN_ENABLE2 : MOD_ENGINE_MAIN_ENABLE4;
	const u8 dma_ch = is_left_motor ? MOD_ENGINE_MAIN_DMA_CH1 : MOD_ENGINE_MAIN_DMA_CH2;

	const auto pwm_channel = dma_ch == MOD_ENGINE_MAIN_DMA_CH1 ? channel1 : channel2;

	buffer[0] = (pwm_channel == 1)
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

static void adjust_pwm(u16 *pwm, const bool less) {
	auto start = less ? 1.0 : 1.15;
	if (*pwm <= 4000) {
		*pwm = *pwm * start; // Boost by 15%
	} else if (*pwm <= 5000) {
		start = start - 0.05;
		*pwm = *pwm * (start * ((*pwm - 4000) / 1000.0)); // Gradual reduction from 1.15x to 1.05x
	} else if (*pwm <= 6000) {
		start = start - 0.03;
		*pwm = *pwm * (start * ((*pwm - 5000) / 1000.0)); // Gradual reduction from 1.05x to 1.02x
	} else if (*pwm <= 9000) {
		start = start - 0.26;
		*pwm = *pwm * (start * ((*pwm - 6000) / 3000.0)); // Gradual decrease to 0.75x
	}
}

void main_engine_advanced(const i16 left, const i16 right) {
	u16 pwm_left = utils_scaled_pwm_percentage(left, XY_DEAD_ZONE, XY_MAX) * 100;
	u16 pwm_right = utils_scaled_pwm_percentage(right, XY_DEAD_ZONE, XY_MAX) * 100;
	adjust_pwm(&pwm_left, false);
	adjust_pwm(&pwm_right, false);

	set_motor_ctrl(left, pwm_left, true);
	set_motor_ctrl(right, pwm_right, false);
}

void main_engine_basic(i16 gas, i16 steer) {
	const bool go_left = steer < 0;
	const bool go_forward = gas > 0;
	const auto steer_perc = utils_scaled_pwm_percentage(steer, XY_DEAD_ZONE, XY_MAX);

	auto gas_left = gas;
	auto gas_right = gas;

	i16 *gas_active = go_left ? &gas_left : &gas_right;

	const i8 sign = go_forward ? -1 : 1;
	const u8 steer_baseline = (steer_perc <= 75) ? steer_perc : (steer_perc - 75);
	const u8 steer_range = (steer_perc <= 75) ? 75 : 25;
	const u8 scaled_value = utils_scaled_pwm_percentage(steer_baseline, 0, steer_range);

	if (steer_perc <= 75) {
		*gas_active -= (*gas_active * scaled_value) / 100;
	} else {
		*gas_active = sign * TRIG_MAX * scaled_value / 100;
	}

	u16 pwm_left = utils_scaled_pwm_percentage(gas_left, TRIG_DEAD_ZONE, TRIG_MAX) * 100;
	u16 pwm_right = utils_scaled_pwm_percentage(gas_right, TRIG_DEAD_ZONE, TRIG_MAX) * 100;
	adjust_pwm(&pwm_left, true);
	adjust_pwm(&pwm_right, true);

	set_motor_ctrl(gas_left, pwm_left, true);
	set_motor_ctrl(gas_right, pwm_right, false);
}
