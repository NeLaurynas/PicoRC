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
#include "modules/lights/lights.h"

volatile static bool can_set_state = true;

void renderer_set_state(uni_gamepad_t *gamepad) {
	if (!can_set_state) return;

	if (state.btn_a != (gamepad->buttons & BUTTON_A)) state.btn_a = (gamepad->buttons & BUTTON_A);
	if (state.btn_x != (gamepad->buttons & BUTTON_X)) state.btn_x = (gamepad->buttons & BUTTON_X);
	if (state.btn_b != (gamepad->buttons & BUTTON_B)) state.btn_b = (gamepad->buttons & BUTTON_B);
	if (state.btn_y != (gamepad->buttons & BUTTON_Y)) state.btn_y = (gamepad->buttons & BUTTON_Y);
	if (state.pad_left != (gamepad->dpad & DPAD_LEFT)) state.pad_left = (gamepad->dpad & DPAD_LEFT);
	if (state.pad_right != (gamepad->dpad & DPAD_RIGHT)) state.pad_right = (gamepad->dpad & DPAD_RIGHT);
	if (state.pad_up != (gamepad->dpad & DPAD_UP)) state.pad_up = (gamepad->dpad & DPAD_UP);
	if (state.pad_down != (gamepad->dpad & DPAD_DOWN)) state.pad_down = (gamepad->dpad & DPAD_DOWN);

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
	// --- DPAD
	if (current_state.pad_left != state.pad_left) {
		if (state.pad_left) {
			current_state.lights.blinkers_left = !current_state.lights.blinkers_left;
			lights_set_blinkers(true, current_state.lights.blinkers_left);
		}
		current_state.pad_left = state.pad_left;
	}

	if (current_state.pad_right != state.pad_right) {
		if (state.pad_right) {
			current_state.lights.blinkers_right = !current_state.lights.blinkers_right;
			lights_set_blinkers(false, current_state.lights.blinkers_right);
		}
		current_state.pad_right = state.pad_right;
	}

	if (current_state.pad_up != state.pad_up) {
		if (state.pad_up) {
			current_state.lights.head = (current_state.lights.head + 1) % 3;
			lights_head_up();
		}
		current_state.pad_up = state.pad_up;
	}

	if (current_state.pad_down != state.pad_down) {
		if (state.pad_down) state.lights.tail = !state.lights.tail;
		current_state.pad_down = state.pad_down;
	}

	// --- THROTTLE and BRAKE
	if (current_state.brake != state.brake || current_state.throttle != state.throttle) {
		const auto level = state.throttle - state.brake;
		if (level + TRIG_DEAD_ZONE < 0) state.lights.braking = true;
		else state.lights.braking = false;
		// engines_drive((state.throttle - state.brake) * -1);
		current_state.brake = state.brake;
		current_state.throttle = state.throttle;
	}

	if (current_state.btn_a != state.btn_a) {
		if (state.btn_a) {
			utils_printf("!!!! pressed btn A\n");
		}
		// cyw43_arch_gpio_put(INTERNAL_LED, btn); // this will fuck you up, cyw43 can be used only from thread it was init'ed
		// toggle?
		current_state.btn_a = state.btn_a;
	}

	if (current_state.lights.tail != state.lights.tail || current_state.lights.braking != state.lights.braking) {
		current_state.lights.tail = state.lights.tail;
		current_state.lights.braking = state.lights.braking;
		lights_tail();
	}
}

static void init() {
	lights_init();
}

void renderer_loop() {
	const void (*animation_functions[])() = { lights_animation };
	constexpr i32 animation_fn_size = ARRAY_SIZE(animation_functions);

	init();

	// ReSharper disable once CppDFAEndlessLoop
	for (;;) {
		can_set_state = true;
		const auto start = time_us_32();

		render_state();
		for (auto i = 0; i < animation_fn_size; i++) animation_functions[i]();

		const auto elapsed_us = utils_time_diff_us(start, time_us_32());
		const auto remaining_us = elapsed_us > RENDER_TICK ? 0 : RENDER_TICK - elapsed_us;

		// utils_printf("x\n");
		if (remaining_us > 0) sleep_us(remaining_us);
	}
}
