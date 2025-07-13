// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#include <stdlib.h>
#include <hardware/timer.h>
#include <pico/cyw43_arch.h>

#include "renderer.h"
#include "shared_config.h"
#include "state.h"
#include "utils.h"
#include "defines/config.h"
#include "modules/engines/engines.h"
#include "shared_modules/v_monitor/v_monitor.h"

volatile static bool can_set_state = true;

void renderer_set_state(uni_gamepad_t *gamepad) {
	if (!can_set_state) return;

	if (state.btn_a != (gamepad->buttons & BUTTON_A)) state.btn_a = (gamepad->buttons & BUTTON_A);
	if (state.btn_x != (gamepad->buttons & BUTTON_X)) state.btn_x = (gamepad->buttons & BUTTON_X);
	if (state.btn_b != (gamepad->buttons & BUTTON_B)) state.btn_b = (gamepad->buttons & BUTTON_B);
	if (state.btn_y != (gamepad->buttons & BUTTON_Y)) state.btn_y = (gamepad->buttons & BUTTON_Y);

	if (state.btn_start != (gamepad->misc_buttons & MISC_BUTTON_START)) state.btn_start = (gamepad->misc_buttons & MISC_BUTTON_START);
	if (state.btn_select != (gamepad->misc_buttons & MISC_BUTTON_SELECT)) state.btn_select = (gamepad->misc_buttons &
		MISC_BUTTON_SELECT);

	if (state.pad_left != (gamepad->dpad & DPAD_LEFT)) state.pad_left = (gamepad->dpad & DPAD_LEFT);
	if (state.pad_right != (gamepad->dpad & DPAD_RIGHT)) state.pad_right = (gamepad->dpad & DPAD_RIGHT);
	if (state.pad_up != (gamepad->dpad & DPAD_UP)) state.pad_up = (gamepad->dpad & DPAD_UP);
	if (state.pad_down != (gamepad->dpad & DPAD_DOWN)) state.pad_down = (gamepad->dpad & DPAD_DOWN);

	if (state.brake != gamepad->brake) state.brake = gamepad->brake;
	if (state.throttle != gamepad->throttle) state.throttle = gamepad->throttle;

	if (state.x != gamepad->axis_x) state.x = gamepad->axis_x;

	can_set_state = false;
}

static void render_state() {
	// --- DPAD
	if (current_state.pad_left != state.pad_left) {
		if (state.pad_left) {
			current_state.lights.blinkers_left = !current_state.lights.blinkers_left;
			// lights_set_blinkers(true, current_state.lights.blinkers_left);
		}
		current_state.pad_left = state.pad_left;
	}

	if (current_state.pad_right != state.pad_right) {
		if (state.pad_right) {
			current_state.lights.blinkers_right = !current_state.lights.blinkers_right;
			// lights_set_blinkers(false, current_state.lights.blinkers_right);
		}
		current_state.pad_right = state.pad_right;
	}

	if (current_state.pad_up != state.pad_up) {
		if (state.pad_up) {
			current_state.lights.head = (current_state.lights.head + 1) % 3;
			// lights_head_up();
		}
		current_state.pad_up = state.pad_up;
	}

	if (current_state.btn_start != state.btn_start || current_state.btn_select != state.btn_select) {
		current_state.btn_start = state.btn_start;
		current_state.btn_select = state.btn_select;
		if (state.btn_start && state.btn_select) {
			current_state.full_beans = !current_state.full_beans;
			utils_printf("SET FULL BEANS: %d\n", current_state.full_beans);
		}
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
		engines_drive(state.throttle - state.brake);
		current_state.brake = state.brake;
		current_state.throttle = state.throttle;
	}

	if (current_state.x != state.x) {
		engines_steer(state.x);
		current_state.x = state.x;
	}

	if (current_state.btn_a != state.btn_a) {
		if (state.btn_a) {
			utils_printf("!!!! pressed btn A\n");
			// TODO: hand brake!
		}
		current_state.btn_a = state.btn_a;
	}

	if (current_state.lights.tail != state.lights.tail || current_state.lights.braking != state.lights.braking) {
		current_state.lights.tail = state.lights.tail;
		current_state.lights.braking = state.lights.braking;
		// lights_tail();
	}
}

static void init() {
	v_monitor_init();
	// lights_init();
	engines_init();
}

void renderer_loop() {
	const void (*animation_functions[])() = { v_monitor_anim };
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

		static constexpr i32 TICKS = 50; // every 100 or 200 ms
		static i32 frame = 0;

		if (frame == 0) {
			const auto voltage = v_monitor_voltage(false);
			if (voltage < MOD_VMON_DEFAULT_REF) {
				// shut down...
				engines_drive(0);
				engines_steer(0);
				return;
			}
		}

		frame = (frame + 1) % TICKS;
	}
}
