// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#include "renderer.h"

#include <stdlib.h>
#include <hardware/timer.h>
#include <pico/cyw43_arch.h>

#include "state.h"
#include "shared_config.h"
#include "utils.h"
#include "defines/config.h"
#include "modules/engine/turret_rotation.h"
#include "modules/sound/sound.h"

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
		if (state.btn_a == true) {
			utils_printf("!!!! pressed btn A\n");
			state.sound.anim = SOUND_LOOP;
		}
		// cyw43_arch_gpio_put(INTERNAL_LED, btn); // this will fuck you up, cyw43 can be used only from thread it was init'ed
		// toggle?
		current_state.btn_a = state.btn_a;
	}
}

static void init() {
	// turret_rotation_init();
	sound_init();
	// soundtwo_init();
	// soundtwo_play();
}

void renderer_loop() {
#if DBG
	static int64_t acc_elapsed_us = 0;
#endif

	const void (*animation_functions[])() = { sound_animation };
	constexpr u8 anim_fn_size = ARRAY_SIZE(animation_functions);

	init();

	// ReSharper disable once CppDFAEndlessLoop
	for (;;) {
		can_set_state = true;
		const auto start = time_us_32();

		render_state();
		for (u8 i = 0; i < anim_fn_size; i++) animation_functions[i]();

		auto end = time_us_32();
		auto elapsed_us = utils_time_diff_us(start, end);
		auto remaining_us = RENDER_TICK - elapsed_us;

#if DBG
		acc_elapsed_us += (remaining_us + elapsed_us);

		if (acc_elapsed_us >= 10 * 1'000'000) { // 10 seconds
			const float elapsed_ms = elapsed_us / 1000.0f;
			utils_printf("render took: %.2f ms (%ld us)\n", elapsed_ms, elapsed_us);
			// utils_print_onboard_temp();

			size_t allocated = 300 * 1024;
			// so 480 kb is free for sure
			// char *ptr = malloc(allocated);
			// if (ptr != NULL) { // seems to panic and not return null
			// 	printf("Successfully allocated: %zu KB\n", allocated / 1024);
			// } else {
			// 	printf("Failed to allocate %zu KB\n", allocated / 1024);
			// 	break;
			// }
			// free(ptr);
			// printf("Free'd: %zu KB\n", allocated / 1024);

			acc_elapsed_us = 0;
			// recalculate because printf is slow
			end = time_us_32();
			elapsed_us = utils_time_diff_us(start, end);
			remaining_us = RENDER_TICK - elapsed_us;
		}
#endif
		// utils_printf("x\n");
		if (remaining_us > 0) sleep_us(remaining_us);
	}
}
