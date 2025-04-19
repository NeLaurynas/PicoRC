// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#include "engines.h"

#include "state.h"
#include "utils.h"
#include "defines/config.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/time.h"

static u16 slice1, slice2 = 0;
static u16 channel1, channel2 = 0;
static float steer_reduce = 0.7f;
static float not_full_beans_reduce = 0.70f;

void engines_init() {
	gpio_init(MOD_ENGINES_ENABLE_1);
	gpio_init(MOD_ENGINES_ENABLE_2);
	gpio_set_dir(MOD_ENGINES_ENABLE_1, true);
	gpio_set_dir(MOD_ENGINES_ENABLE_2, true);
	gpio_set_function(MOD_ENGINES_PWM_1, GPIO_FUNC_PWM);
	gpio_set_function(MOD_ENGINES_PWM_2, GPIO_FUNC_PWM);

	slice1 = pwm_gpio_to_slice_num(MOD_ENGINES_PWM_1);
	slice2 = pwm_gpio_to_slice_num(MOD_ENGINES_PWM_2);
	channel1 = pwm_gpio_to_channel(MOD_ENGINES_PWM_1);
	channel2 = pwm_gpio_to_channel(MOD_ENGINES_PWM_2);

	// init PWM 1
	constexpr auto top = 1000;
	const auto clk_div = utils_calculate_pwm_divider(top, 15);
	utils_printf("ENGINES PWM CLK DIV: %f\n", clk_div);
	auto pwm_c1 = pwm_get_default_config();
	pwm_c1.top = top;
	pwm_init(slice1, &pwm_c1, false);
	pwm_set_clkdiv(slice1, clk_div);
	pwm_set_phase_correct(slice1, false);
	pwm_set_enabled(slice1, true);

	// init PWM 2
	auto pwm_c2 = pwm_get_default_config();
	pwm_c2.top = top;
	pwm_init(slice2, &pwm_c2, false);
	pwm_set_clkdiv(slice2, clk_div);
	pwm_set_phase_correct(slice2, false);
	pwm_set_enabled(slice2, true);

	// init DMA
	if (dma_channel_is_claimed(MOD_ENGINES_DMA_1)) utils_error_mode(10);
	dma_channel_claim(MOD_ENGINES_DMA_1);
	auto dma_c1 = dma_channel_get_default_config(MOD_ENGINES_DMA_1);
	channel_config_set_transfer_data_size(&dma_c1, DMA_SIZE_16);
	channel_config_set_read_increment(&dma_c1, false);
	channel_config_set_write_increment(&dma_c1, false);
	channel_config_set_dreq(&dma_c1, DREQ_FORCE);
	dma_channel_configure(MOD_ENGINES_DMA_1, &dma_c1, utils_pwm_cc_for_16bit(slice1, channel1), nullptr, 1, false);

	if (dma_channel_is_claimed(MOD_ENGINES_DMA_2)) utils_error_mode(11);
	dma_channel_claim(MOD_ENGINES_DMA_2);
	auto dma_c2 = dma_channel_get_default_config(MOD_ENGINES_DMA_2);
	channel_config_set_transfer_data_size(&dma_c2, DMA_SIZE_16);
	channel_config_set_read_increment(&dma_c2, false);
	channel_config_set_write_increment(&dma_c2, false);
	channel_config_set_dreq(&dma_c2, DREQ_FORCE);
	dma_channel_configure(MOD_ENGINES_DMA_2, &dma_c2, utils_pwm_cc_for_16bit(slice2, channel2), nullptr, 1, false);
	sleep_ms(1);
}

static void adjust_pwm(i32 *pwm) {
	if (*pwm == 0) return;
	if (*pwm >= 900) return;

	u32 out;
	if (*pwm <= 100) out = 4UL * *pwm;
	else out = 400UL + (5UL * (*pwm - 100UL)) / 8UL;

	*pwm = (i32)out;
}

static void set_motor_ctrl(const bool drive_engine, const i32 val, u16 pwm) {
	const u8 pin1 = drive_engine ? MOD_ENGINES_ENABLE_1 : MOD_ENGINES_ENABLE_2;
	const u8 dma_ch = drive_engine ? MOD_ENGINES_DMA_1 : MOD_ENGINES_DMA_2;

	if (drive_engine) {
		if (!current_state.full_beans) pwm = (u16)((float)pwm * not_full_beans_reduce);
	} else {
		pwm = (u16)((float)pwm * steer_reduce);
	}

	utils_printf("pinL %d, val: %d, pwm: %d\n", pin1, val, pwm);

	if (val < 0) {
		gpio_put(pin1, false);
		dma_channel_transfer_from_buffer_now(dma_ch, &pwm, 1);
	} else {
		gpio_put(pin1, val != 0);
		dma_channel_transfer_from_buffer_now(dma_ch, &pwm, 1);
	}
}

void engines_drive(const i32 val) {
	i32 pwm = utils_scaled_pwm_percentage(val, TRIG_DEAD_ZONE, TRIG_MAX) * 10;
	adjust_pwm(&pwm);

	set_motor_ctrl(true, val, pwm);
}

void engines_steer(const i32 val) {
	i32 pwm = utils_scaled_pwm_percentage(val, MOD_ENGINE_XY_DEAD_ZONE, MOD_ENGINE_XY_MAX) * 10;
	adjust_pwm(&pwm);

	set_motor_ctrl(false, val, pwm);
}
