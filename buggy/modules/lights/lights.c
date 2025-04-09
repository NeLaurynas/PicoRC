// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#include "lights.h"

#include <hardware/gpio.h>
#include <hardware/pwm.h>

#include "shared_config.h"
#include "state.h"
#include "utils.h"
#include "defines/config.h"

static i32 cfg_blinkers_top = 100;
static u32 cfg_blinkers_slice_left;
static u32 cfg_blinkers_channel_left;
static u32 cfg_blinkers_slice_right;
static u32 cfg_blinkers_channel_right;

static u32 state_blinkers_frame = 0; // why not in global state?

void lights_init() {
	// init blinkers PWM 1
	gpio_set_function(MOD_LIGHTS_BLINKERS_LEFT, GPIO_FUNC_PWM);
	gpio_set_drive_strength(MOD_LIGHTS_BLINKERS_LEFT, GPIO_DRIVE_STRENGTH_8MA);
	cfg_blinkers_channel_left = pwm_gpio_to_channel(MOD_LIGHTS_BLINKERS_LEFT);
	cfg_blinkers_slice_left = pwm_gpio_to_slice_num(MOD_LIGHTS_BLINKERS_LEFT);
	auto pwm_blinkers_left_c = pwm_get_default_config();
	const auto blinkers_div = utils_calculate_pwm_divider(cfg_blinkers_top, 1);
	utils_printf("BLINKERS PWM CLK DIV: %f\n", blinkers_div);
	pwm_config_set_clkdiv(&pwm_blinkers_left_c, blinkers_div);
	pwm_init(cfg_blinkers_slice_left, &pwm_blinkers_left_c, true);
	pwm_set_wrap(cfg_blinkers_slice_left, cfg_blinkers_top);
	// init blinkers PWM 2
	gpio_set_function(MOD_LIGHTS_BLINKERS_RIGHT, GPIO_FUNC_PWM);
	gpio_set_drive_strength(MOD_LIGHTS_BLINKERS_RIGHT, GPIO_DRIVE_STRENGTH_8MA);
	cfg_blinkers_channel_right = pwm_gpio_to_channel(MOD_LIGHTS_BLINKERS_RIGHT);
	cfg_blinkers_slice_right = pwm_gpio_to_slice_num(MOD_LIGHTS_BLINKERS_RIGHT);
	auto pwm_blinkers_right_c = pwm_get_default_config();
	pwm_config_set_clkdiv(&pwm_blinkers_right_c, blinkers_div);
	pwm_init(cfg_blinkers_slice_right, &pwm_blinkers_right_c, true);
	pwm_set_wrap(cfg_blinkers_slice_right, cfg_blinkers_top);
}

void lights_set_blinkers(const bool left, const bool on) {
	if (!on) {
		if (!current_state.lights.blinkers_left && !current_state.lights.blinkers_right) state_blinkers_frame = 0;
		if (left) pwm_set_chan_level(cfg_blinkers_slice_left, cfg_blinkers_channel_left, 0);
		else pwm_set_chan_level(cfg_blinkers_slice_right, cfg_blinkers_channel_right, 0);
	}
}

void lights_inc_head() {
}

static void blinkers_anim() {
	// Mazda Homura (焔) style
	constexpr i32 total_frames = 100;
	constexpr i32 tim_0 = 7; // fade in until
	constexpr i32 tim_1 = 50; // hold until
	constexpr i32 tim_2 = total_frames - 12 - tim_1; // fade out until

	if (state_blinkers_frame < tim_1) {
		const auto level = utils_proportional_reduce(cfg_blinkers_top, state_blinkers_frame, tim_0, false);
		if (current_state.lights.blinkers_left) pwm_set_chan_level(cfg_blinkers_slice_left, cfg_blinkers_channel_left, level);
		if (current_state.lights.blinkers_right) pwm_set_chan_level(cfg_blinkers_slice_right, cfg_blinkers_channel_right, level);
	} else {
		const auto level = utils_proportional_reduce(cfg_blinkers_top, state_blinkers_frame - tim_1, tim_2, true);
		if (current_state.lights.blinkers_left) pwm_set_chan_level(cfg_blinkers_slice_left, cfg_blinkers_channel_left, level);
		if (current_state.lights.blinkers_right) pwm_set_chan_level(cfg_blinkers_slice_right, cfg_blinkers_channel_right, level);
	}

	state_blinkers_frame = (state_blinkers_frame + 1) % total_frames;
}

void lights_animation() {
	if (current_state.lights.blinkers_left || current_state.lights.blinkers_right) blinkers_anim();
}
