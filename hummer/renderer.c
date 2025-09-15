// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#include <stdio.h>
#include <stdlib.h>
#include <hardware/timer.h>
#include <pico/cyw43_arch.h>

#include "renderer.h"
#include "shared_config.h"
#include "state.h"
#include "str.h"
#include "utils.h"
#include "defines/config.h"
#include "modules/engines/engines.h"
#include "modules/lights/lights.h"
#include "shared_modules/cpu_cores/cpu_cores.h"
#include "shared_modules/storage/storage.h"
#include "shared_modules/v_monitor/v_monitor.h"

#include "defines/app_settings_t.h"
#include "pico/flash.h"
#include "pico/unique_id.h"

volatile static bool can_set_state = true;

void renderer_set_state(const uni_gamepad_t *gamepad) {
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

static inline void blinkers_off(void) {
	current_state.lights.blinkers_left = false;
	current_state.lights.blinkers_right = false;
	lights_set_blinkers(true, false);
	lights_set_blinkers(false, false);
}

static inline void set_hazards(const bool on) {
	current_state.lights.hazards = on;
	blinkers_off();
}

static inline void set_blinker(const bool left, const bool on) {
	if (on) set_hazards(false);
	current_state.lights.blinkers_left = left ? on : false;
	current_state.lights.blinkers_right = left ? false : on;
	lights_set_blinkers(true, current_state.lights.blinkers_left);
	lights_set_blinkers(false, current_state.lights.blinkers_right);
}

static void render_state() {
	// --- DPAD
	if (current_state.pad_down != state.pad_down) {
		if (state.pad_down) {
			if (current_state.lights.hazards) set_hazards(false);
			else if (current_state.lights.blinkers_left || current_state.lights.blinkers_right) blinkers_off();
			else set_hazards(true);
		}
		current_state.pad_down = state.pad_down;
	}

	if (current_state.pad_left != state.pad_left) {
		if (state.pad_left) {
			if (current_state.lights.hazards) set_hazards(false);
			else if (current_state.lights.blinkers_right) set_blinker(false, false);
			else set_blinker(true, !current_state.lights.blinkers_left);

		}
		current_state.pad_left = state.pad_left;
	}

	if (current_state.pad_right != state.pad_right) {
		if (state.pad_right) {
			if (current_state.lights.hazards) set_hazards(false);
			else if (current_state.lights.blinkers_left) set_blinker(true, false);
			else set_blinker(false, !current_state.lights.blinkers_right);

		}
		current_state.pad_right = state.pad_right;
	}

	if (current_state.pad_up != state.pad_up) {
		if (state.pad_up) {
			current_state.lights.head = (current_state.lights.head + 1) % 3;
			state.lights.tail = current_state.lights.head > 0 ? true : false;
			lights_head_up();
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

	// --- THROTTLE and BRAKE
	if (current_state.brake != state.brake || current_state.throttle != state.throttle) {
		const auto level = state.throttle - state.brake;
		state.lights.braking = level + TRIG_DEAD_ZONE < 0 ? true : false;
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
			cpu_cores_send_shutdown_to_core0_from_core1();
		}
		current_state.btn_a = state.btn_a;
	}

	if (current_state.lights.tail != state.lights.tail || current_state.lights.braking != state.lights.braking) {
		current_state.lights.tail = state.lights.tail;
		current_state.lights.braking = state.lights.braking;
		lights_tail();
	}
}

static void init() {
	flash_safe_execute_core_init();

	v_monitor_init();
	lights_init();
	engines_init();

	utils_crc_init();
	if (!storage_init()) {
		// first boot?
		app_settings.boot_count = 1;
		char tmp[32];
		pico_get_unique_board_id_string(tmp, 32);
		str_set(app_settings.device_name, 32, tmp);
		str_set(app_settings.app_name, 32, "Hummer");
		storage_save(&app_settings, sizeof app_settings);
	} else {
		storage_load(&app_settings, sizeof app_settings);

		app_settings.boot_count = app_settings.boot_count + 1;
		char tmp[32];
		pico_get_unique_board_id_string(tmp, 32);
		str_set(app_settings.device_name, 32, tmp);
		// snprintf(tmp, sizeof tmp, "Pico 2 debug #%u", (unsigned)app_settings.boot_count);
		// str_set(app_settings.device_name, 32, tmp);

		storage_save(&app_settings, sizeof app_settings);
	}

	utils_printf("%s -> %s boot count: %u\n", app_settings.device_name, app_settings.app_name, app_settings.boot_count);
}

void renderer_loop() {
	static void (*const animation_functions[])(void) = { v_monitor_anim, lights_animation };
	constexpr i32 animation_fn_size = ARRAY_SIZE(animation_functions);

	init();

	for (;;) {
		can_set_state = true;
		const auto start = time_us_32();

		render_state();
		for (auto i = 0; i < animation_fn_size; i++) animation_functions[i]();

		const auto elapsed_us = utils_time_diff_us(start, time_us_32());
		const auto remaining_us = elapsed_us > RENDER_TICK ? 0 : RENDER_TICK - elapsed_us;

		if (remaining_us > 0) sleep_us(remaining_us);

		static constexpr i32 TICKS = 50; // every 100 or 200 ms
		static i32 frame = 0;

		// todo: uncomment...
		 if (frame == 0) {
		 	// const auto voltage = v_monitor_voltage(false);
		 	// if (voltage < MOD_VMON_DEFAULT_REF) {
		 		// shut down...
		 		// engines_drive(0);
		 		// engines_steer(0);
		 		// cpu_cores_send_shutdown_to_core0_from_core1();
				// TODO: lights also off
		 		// return;
		 	// }

		 	// // memory test
		 	// size_t size = (15 * 1024) + 256;
		 	// void *buffer = malloc(size);
		 	// if (buffer == NULL) {
		 	// 	printf("Allocation of %zu bytes failed\n", size);
		 	// } else {
		 	// 	printf("Allocation succeeded (%d kB)\n", size / 1024);
		 	// 	// free(buffer);
		 	// }
			 //
		 	// dump_heap();
		 	// size_t highest = largest_mallocable();
		 	// printf("Largest allocatable: %u kB\n", highest / 1024);
		 }

		frame = (frame + 1) % TICKS;
	}
}
