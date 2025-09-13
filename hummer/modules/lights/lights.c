// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#include "lights.h"

#include <hardware/gpio.h>
#include <hardware/pwm.h>

#include "shared_config.h"
#include "state.h"
#include "utils.h"
#include "defines/config.h"

static i32 cfg_all_pwm_top = 100;
static u32 cfg_blinkers_slice_left;
static u32 cfg_blinkers_channel_left;
static u32 cfg_blinkers_slice_right;
static u32 cfg_blinkers_channel_right;
static u32 cfg_head_slice;
static u32 cfg_head_channel;
static u32 cfg_tail_slice;
static u32 cfg_tail_channel;

static u32 state_blinkers_frame = 0; // why not in global state?

void lights_init() {
	// // init blinkers PWM 1
	// gpio_set_function(MOD_LIGHTS_BLINKERS_LEFT, GPIO_FUNC_PWM);
	// gpio_set_drive_strength(MOD_LIGHTS_BLINKERS_LEFT, GPIO_DRIVE_STRENGTH_8MA);
	// cfg_blinkers_channel_left = pwm_gpio_to_channel(MOD_LIGHTS_BLINKERS_LEFT);
	// cfg_blinkers_slice_left = pwm_gpio_to_slice_num(MOD_LIGHTS_BLINKERS_LEFT);
	// auto pwm_blinkers_left_c = pwm_get_default_config();
	const auto all_div = utils_calculate_pwm_divider(cfg_all_pwm_top, 2);
	// utils_printf("BLINKERS PWM CLK DIV: %f\n", all_div);
	// pwm_config_set_clkdiv(&pwm_blinkers_left_c, all_div);
	// pwm_init(cfg_blinkers_slice_left, &pwm_blinkers_left_c, true);
	// pwm_set_wrap(cfg_blinkers_slice_left, cfg_all_pwm_top);
	// // init blinkers PWM 2
	// gpio_set_function(MOD_LIGHTS_BLINKERS_RIGHT, GPIO_FUNC_PWM);
	// gpio_set_drive_strength(MOD_LIGHTS_BLINKERS_RIGHT, GPIO_DRIVE_STRENGTH_8MA);
	// cfg_blinkers_channel_right = pwm_gpio_to_channel(MOD_LIGHTS_BLINKERS_RIGHT);
	// cfg_blinkers_slice_right = pwm_gpio_to_slice_num(MOD_LIGHTS_BLINKERS_RIGHT);
	// auto pwm_blinkers_right_c = pwm_get_default_config();
	// pwm_config_set_clkdiv(&pwm_blinkers_right_c, all_div);
	// pwm_init(cfg_blinkers_slice_right, &pwm_blinkers_right_c, true);
	// pwm_set_wrap(cfg_blinkers_slice_right, cfg_all_pwm_top);
	//
	// // init head PWM
	// gpio_set_function(MOD_LIGHTS_HEAD, GPIO_FUNC_PWM);
	// gpio_set_drive_strength(MOD_LIGHTS_HEAD, GPIO_DRIVE_STRENGTH_12MA);
	// cfg_head_channel = pwm_gpio_to_channel(MOD_LIGHTS_HEAD);
	// cfg_head_slice = pwm_gpio_to_slice_num(MOD_LIGHTS_HEAD);
	// auto pwm_head_c = pwm_get_default_config();
	// pwm_config_set_clkdiv(&pwm_head_c, all_div);
	// pwm_init(cfg_head_slice, &pwm_head_c, true);
	// pwm_set_wrap(cfg_head_slice, cfg_all_pwm_top);

	// init tail PWM
	gpio_set_function(MOD_LIGHTS_TAIL, GPIO_FUNC_PWM);
	auto drive_str = gpio_get_drive_strength(MOD_LIGHTS_TAIL);
	utils_printf("drive str: %d\n", drive_str);
	cfg_tail_channel = pwm_gpio_to_channel(MOD_LIGHTS_TAIL);
	cfg_tail_slice = pwm_gpio_to_slice_num(MOD_LIGHTS_TAIL);
	auto pwm_tail_c = pwm_get_default_config();
	pwm_config_set_clkdiv(&pwm_tail_c, all_div);
	pwm_init(cfg_tail_slice, &pwm_tail_c, true);
	pwm_set_wrap(cfg_tail_slice, cfg_all_pwm_top);
	// gpio_init(MOD_LIGHTS_TAIL);
	// gpio_set_dir(MOD_LIGHTS_TAIL, true);
}

void lights_set_blinkers(const bool left, const bool on) {
	if (!on) {
		if (!current_state.lights.blinkers_left && !current_state.lights.blinkers_right) state_blinkers_frame = 0;
		if (left) pwm_set_chan_level(cfg_blinkers_slice_left, cfg_blinkers_channel_left, 0);
		else pwm_set_chan_level(cfg_blinkers_slice_right, cfg_blinkers_channel_right, 0);
	}
}

void lights_head_up() {
	switch (current_state.lights.head) {
		case 0:
			pwm_set_chan_level(cfg_head_slice, cfg_head_channel, 0);
			break;
		case 1:
			pwm_set_chan_level(cfg_head_slice, cfg_head_channel, 15); // TODO: maybe 20?
			break;
		case 2:
			pwm_set_chan_level(cfg_head_slice, cfg_head_channel, 100);
			break;
		default:
			utils_printf("!!! lights_head_up default reached");
	}
}

void lights_tail() {
	i32 level;
	if (!current_state.lights.tail && !current_state.lights.braking) level = 0;
	else {
		if (current_state.lights.braking) level = 100;
		else level = 2;
	}

	pwm_set_chan_level(cfg_tail_slice, cfg_tail_channel, level);
}

static void blinkers_anim() {
	// Mazda Homura (焔) style
	constexpr i32 total_frames = 100;
	constexpr i32 tim_0 = 7; // fade in until
	constexpr i32 tim_1 = 50; // hold until
	constexpr i32 tim_2 = total_frames - 12 - tim_1; // fade out until

	if (state_blinkers_frame < tim_1) {
		const auto level = utils_proportional_reduce(cfg_all_pwm_top, state_blinkers_frame, tim_0, false);
		if (current_state.lights.blinkers_left) pwm_set_chan_level(cfg_blinkers_slice_left, cfg_blinkers_channel_left, level);
		if (current_state.lights.blinkers_right) pwm_set_chan_level(cfg_blinkers_slice_right, cfg_blinkers_channel_right, level);
	} else {
		const auto level = utils_proportional_reduce(cfg_all_pwm_top, state_blinkers_frame - tim_1, tim_2, true);
		if (current_state.lights.blinkers_left) pwm_set_chan_level(cfg_blinkers_slice_left, cfg_blinkers_channel_left, level);
		if (current_state.lights.blinkers_right) pwm_set_chan_level(cfg_blinkers_slice_right, cfg_blinkers_channel_right, level);
	}

	state_blinkers_frame = (state_blinkers_frame + 1) % total_frames;
}

void lights_animation() {
	if (current_state.lights.blinkers_left || current_state.lights.blinkers_right) blinkers_anim();
}
