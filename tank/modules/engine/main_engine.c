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
	gpio_set_dir(MOD_ENGINE_MAIN_ENABLE1, true);
	gpio_set_dir(MOD_ENGINE_MAIN_ENABLE2, true);
	gpio_set_function(MOD_ENGINE_MAIN_PWM1, GPIO_FUNC_PWM);
	gpio_set_function(MOD_ENGINE_MAIN_PWM2, GPIO_FUNC_PWM);

	slice1 = pwm_gpio_to_slice_num(MOD_ENGINE_MAIN_PWM1);
	slice2 = pwm_gpio_to_slice_num(MOD_ENGINE_MAIN_PWM2);
	channel1 = pwm_gpio_to_channel(MOD_ENGINE_MAIN_PWM1);
	channel2 = pwm_gpio_to_channel(MOD_ENGINE_MAIN_PWM2);

	// init PWM
	auto pwm_c1 = pwm_get_default_config();
	pwm_c1.top = 100;
	pwm_init(slice1, &pwm_c1, false);
	const auto clk_div = utils_calculate_pio_clk_div(0.5f);
	utils_printf("MAIN ENGINE CLK DIV: %f", clk_div);
	pwm_set_clkdiv(slice1, clk_div); // 3.f for 5 khz frequency (2.f for 7.5 khz 1.f for 15 khz)
	pwm_set_phase_correct(slice1, false);
	pwm_set_enabled(slice1, true);

	auto pwm_c2 = pwm_get_default_config();
	pwm_c2.top = 100;
	pwm_init(slice2, &pwm_c2, false);
	pwm_set_clkdiv(slice2, clk_div);
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

static void set_motor_ctrl(const i32 val, const u16 pwm, const bool is_left_motor) {
	const u8 pin1 = is_left_motor ? MOD_ENGINE_MAIN_ENABLE1 : MOD_ENGINE_MAIN_ENABLE2;
	const u8 dma_ch = is_left_motor ? MOD_ENGINE_MAIN_DMA_CH1 : MOD_ENGINE_MAIN_DMA_CH2;

	const auto pwm_channel = dma_ch == MOD_ENGINE_MAIN_DMA_CH1 ? channel1 : channel2;

	buffer[0] = (pwm_channel == 1)
		? (buffer[0] & 0x0000FFFF) | ((u32)pwm << 16)
		: (buffer[0] & 0xFFFF0000) | (pwm & 0xFFFF);

	if (val < 0) {
		gpio_put(pin1, true);
		dma_channel_transfer_from_buffer_now(dma_ch, buffer, 1);
	} else {
		gpio_put(pin1, false);
		dma_channel_transfer_from_buffer_now(dma_ch, buffer, 1);
	}
}

static void adjust_pwm(u16 *pwm) {
	if (*pwm == 0) return;
	if (*pwm >= 90) return;

	u16 out;
	if (*pwm <= 10) out = 4UL * *pwm;
	else out = 40UL + (5UL * (*pwm - 10UL)) / 8UL;

	*pwm = out;
}

void main_engine_advanced(const i32 left, const i32 right) {
	u16 pwm_left = utils_scaled_pwm_percentage(left, XY_DEAD_ZONE, XY_MAX);
	u16 pwm_right = utils_scaled_pwm_percentage(right, XY_DEAD_ZONE, XY_MAX);
	if (left < 0) pwm_left += 1;
	if (right < 0) pwm_right += 1;
	adjust_pwm(&pwm_left);
	adjust_pwm(&pwm_right);
	utils_printf("%d<<>>%d\n", pwm_left, pwm_right);

	set_motor_ctrl(left, pwm_left, true);
	set_motor_ctrl(right, pwm_right, false);
}

void main_engine_basic(const i32 gas, const i32 steer) {
	const bool go_left = steer < 0;
	const bool go_forward = gas > 0;
	const auto steer_perc = utils_scaled_pwm_percentage(steer, XY_DEAD_ZONE, XY_MAX);

	auto gas_left = gas;
	auto gas_right = gas;

	i32 *gas_active = go_left ? &gas_left : &gas_right;

	const i8 sign = go_forward ? -1 : 1;
	const u8 steer_baseline = (steer_perc <= 75) ? steer_perc : (steer_perc - 75);
	const u8 steer_range = (steer_perc <= 75) ? 75 : 25;
	const u8 scaled_value = utils_scaled_pwm_percentage(steer_baseline, 0, steer_range);

	if (steer_perc <= 75) {
		*gas_active -= (*gas_active * scaled_value) / 100;
	} else {
		*gas_active = sign * TRIG_MAX * scaled_value / 100;
	}

	u16 pwm_left = utils_scaled_pwm_percentage(gas_left, TRIG_DEAD_ZONE, TRIG_MAX);
	u16 pwm_right = utils_scaled_pwm_percentage(gas_right, TRIG_DEAD_ZONE, TRIG_MAX);
	adjust_pwm(&pwm_left);
	adjust_pwm(&pwm_right);
	utils_printf("%d<<%d>>%d\n", pwm_left, gas, pwm_right);

	set_motor_ctrl(gas_left, pwm_left, true);
	set_motor_ctrl(gas_right, pwm_right, false);
}
