// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#include <hardware/timer.h>
#include <pico/cyw43_arch.h>
#include <stdlib.h>

#include "defines/config.h"
#include "renderer.h"
#include "shared_config.h"
#include "state.h"
#include "utils.h"

volatile static bool can_set_state = true;

void renderer_set_state(uni_gamepad_t *gamepad) {
	if (!can_set_state) return;

	if (state.btn_a != (gamepad->buttons & BUTTON_A)) state.btn_a = (gamepad->buttons & BUTTON_A);
	if (state.btn_x != (gamepad->buttons & BUTTON_X)) state.btn_x = (gamepad->buttons & BUTTON_X);
	if (state.btn_b != (gamepad->buttons & BUTTON_B)) state.btn_b = (gamepad->buttons & BUTTON_B);
	if (state.btn_y != (gamepad->buttons & BUTTON_Y)) state.btn_y = (gamepad->buttons & BUTTON_Y);

	if (state.brake != gamepad->brake) state.brake = gamepad->brake;
	if (state.throttle != gamepad->throttle) state.throttle = gamepad->throttle;

	if (state.x != gamepad->axis_x) state.x = gamepad->axis_x;
	if (state.y != gamepad->axis_y) state.y = gamepad->axis_y;

	if (state.rx != gamepad->axis_rx) {
		state.rx = gamepad->axis_rx;
		// turret_rotation_rotate(state.rx);
	}
	if (state.ry != gamepad->axis_ry) state.ry = gamepad->axis_ry;

	can_set_state = false;
}

static void render_state() {
	// get early
	if (current_state.btn_a != state.btn_a) {
		if (state.btn_a) {
			utils_printf("!!!! pressed btn A\n");
		}
		// cyw43_arch_gpio_put(INTERNAL_LED, btn); // this will fuck you up, cyw43 can be used only from thread it was init'ed
		// toggle?
		current_state.btn_a = state.btn_a;
	}
}

static void init() {
	// turret_rotation_init();
}

void renderer_loop() {
	init();

	// ReSharper disable once CppDFAEndlessLoop
	for (;;) {
		can_set_state = true;
		const auto start = time_us_32();

		render_state();

		const auto elapsed_us = utils_time_diff_us(start, time_us_32());
		const auto remaining_us = elapsed_us > RENDER_TICK ? 0 : RENDER_TICK - elapsed_us;

		// utils_printf("x\n");
		if (remaining_us > 0) sleep_us(remaining_us);
	}
}
