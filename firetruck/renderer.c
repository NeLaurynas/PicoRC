// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#include "renderer.h"

#include <hardware/timer.h>
#include <pico/cyw43_arch.h>
#include <stdlib.h>

#include "defines/config.h"
#include "modules/engine/turret_rotation.h"
#include "modules/sound/sound.h"
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

	if (state.shoulder_l != (gamepad->buttons & BUTTON_SHOULDER_L)) state.shoulder_l = (gamepad->buttons & BUTTON_SHOULDER_L);
	if (state.shoulder_r != (gamepad->buttons & BUTTON_SHOULDER_R)) state.shoulder_r = (gamepad->buttons & BUTTON_SHOULDER_R);

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
	if (current_state.shoulder_l != state.shoulder_l || current_state.shoulder_r != state.shoulder_r) {
		if (state.shoulder_l == true) {
			if (state.sound.anim & SOUND_LOOP) {
				state.sound.anim &= ~SOUND_LOOP;
			} else {
				state.sound.anim = state.sound.anim | SOUND_LOOP;
			}
		}

		if (state.shoulder_r == true) state.sound.anim = state.sound.anim | SOUND_WAIL;
		else state.sound.anim &= ~SOUND_WAIL;

		current_state.shoulder_l = state.shoulder_l;
		current_state.shoulder_r = state.shoulder_r;
	}
}

void renderer_init() {
	// turret_rotation_init();
	sound_init();
}

void renderer_loop() {
	const void (*animation_functions[])() = { sound_animation };
	constexpr u8 anim_fn_size = ARRAY_SIZE(animation_functions);

	// ReSharper disable once CppDFAEndlessLoop

	renderer_init();

	for (;;) {
		can_set_state = true;
		const u32 start = time_us_32();

		render_state();
		for (u8 i = 0; i < anim_fn_size; i++) animation_functions[i]();

		const u32 elapsed_us = utils_time_diff_us(start, time_us_32());
		const u32 remaining_us = elapsed_us > RENDER_TICK ? 0 : RENDER_TICK - elapsed_us;

		// utils_printf("x\n");
		if (remaining_us > 0) sleep_us(remaining_us);
	}
}
