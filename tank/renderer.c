// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#include "renderer.h"

#include <stdlib.h>
#include <hardware/timer.h>
#include <pico/cyw43_arch.h>

#include "state.h"
#include "utils.h"
#include "shared_config.h"
#include "defines/config.h"
#include "modules/engine/main_engine.h"
#include "modules/engine/turret_ctrl.h"
#include "modules/leds/leds.h"

volatile static bool can_set_state = true;

void renderer_set_state(uni_gamepad_t *gamepad) {
	if (!can_set_state) return;

	if (state.btn_a != (gamepad->buttons & BUTTON_A)) state.btn_a = (gamepad->buttons & BUTTON_A);
	if (state.btn_x != (gamepad->buttons & BUTTON_X)) state.btn_x = (gamepad->buttons & BUTTON_X);
	if (state.btn_b != (gamepad->buttons & BUTTON_B)) state.btn_b = (gamepad->buttons & BUTTON_B);
	if (state.btn_y != (gamepad->buttons & BUTTON_Y)) state.btn_y = (gamepad->buttons & BUTTON_Y);

	if (state.btn_start != (gamepad->misc_buttons & MISC_BUTTON_START)) state.btn_start = (gamepad->misc_buttons & MISC_BUTTON_START);
	if (state.btn_select != (gamepad->misc_buttons & MISC_BUTTON_SELECT))
		state.btn_select = (gamepad->misc_buttons &
			MISC_BUTTON_SELECT);

	if (state.brake != gamepad->brake) state.brake = gamepad->brake;
	if (state.throttle != gamepad->throttle) state.throttle = gamepad->throttle;

	if (state.x != gamepad->axis_x) state.x = gamepad->axis_x;
	if (state.y != gamepad->axis_y) state.y = gamepad->axis_y;

	if (state.rx != gamepad->axis_rx) state.rx = gamepad->axis_rx;
	if (state.ry != gamepad->axis_ry) state.ry = gamepad->axis_ry;

	can_set_state = false;
}

static void render_state() {
	if (current_state.btn_start != state.btn_start || current_state.btn_select != state.btn_select) {
		current_state.btn_start = state.btn_start;
		current_state.btn_select = state.btn_select;
		const bool toggle_advanced = state.btn_start && state.btn_select;
		if (toggle_advanced) {
			current_state.advanced_mode = !current_state.advanced_mode;
		}
	}

	if (current_state.btn_a != state.btn_a || current_state.btn_y != state.btn_y) {
		current_state.btn_a = state.btn_a;
		current_state.btn_y = state.btn_y;
		if (state.btn_a || state.btn_y) current_state.white_leds = !current_state.white_leds;
		leds_toggle_white(current_state.white_leds);
	}
	if (current_state.btn_x != state.btn_x || current_state.btn_b != state.btn_b) {
		current_state.btn_x = state.btn_x;
		current_state.btn_b = state.btn_b;
		if (state.btn_x || state.btn_b) current_state.red_led = !current_state.red_led;
		leds_toggle_red(current_state.red_led);
	}

	if (current_state.advanced_mode) {
		if (current_state.y != state.y || current_state.ry != state.ry) {
			current_state.y = state.y;
			current_state.ry = state.ry;
			main_engine_advanced(state.y, state.ry);
		}
	} else {
		if (current_state.brake != state.brake || current_state.throttle != state.throttle || current_state.x != state.x) {
			current_state.brake = state.brake;
			current_state.throttle = state.throttle;
			current_state.x = state.x;
			main_engine_basic(state.throttle - state.brake, state.x);
		}
	}

	if (current_state.rx != state.rx) {
		current_state.rx = state.rx;
		turret_ctrl_rotate(state.rx);
	}
	if (current_state.ry != state.ry) {
		current_state.ry = state.ry;
		if (!current_state.advanced_mode) turret_ctrl_lift(state.ry);
	}
}

static void init() {
	main_engine_init();
	leds_init();
	turret_ctrl_init();
}

void renderer_loop() {
	init();

	// ReSharper disable once CppDFAEndlessLoop
	for (;;) {
		can_set_state = true;
		const auto start = time_us_32();

		render_state();

		const auto end = time_us_32();
		const int32_t elapsed_us = utils_time_diff_us(start, end);
		const auto remaining_us = RENDER_TICK - elapsed_us;

		if (remaining_us > 0) sleep_us(remaining_us);
	}
}
