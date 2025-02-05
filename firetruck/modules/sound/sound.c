// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#include "sound.h"

#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <hardware/pwm.h>

#include "shared_config.h"
#include "state.h"
#include "utils.h"
#include "defines/config.h"

#define FREQUENCY			1000000.0f
#define FREQ_MIN            300.0f   // Lower bound of frequency in Hz
#define FREQ_MAX            1200.0f  // Upper bound of frequency in Hz
#define RISING_STEP         5.0f     // Larger step for a faster rising sweep
#define FALLING_STEP        3.0f     // Smaller step for a slower falling sweep
#define RISING_STEP_STEEP   75.0f    // for WAIL
#define FALLING_STEP_STEEP  55.0f    // for WAIL

#define LOOP_FOR_SECONDS	15

static u32 channel;
static u32 slice;
static bool rising = true;
static float freq = FREQ_MIN;
static u32 looping_for = 0;

void sound_init() {
	gpio_set_function(MOD_SOUND_PIN, GPIO_FUNC_PWM);

	slice = pwm_gpio_to_slice_num(MOD_SOUND_PIN);
	channel = pwm_gpio_to_channel(MOD_SOUND_PIN);

	auto config = pwm_get_default_config();

	const auto clk_div = (float)clock_get_hz(clk_sys) / FREQUENCY;
	utils_printf("SOUND PWM CLK DIV: %f\n", clk_div);
	pwm_config_set_clkdiv(&config, clk_div);

	pwm_init(slice, &config, true);
}

void anim_loop() {
	const u16 wrap = (u16)(FREQUENCY / freq) - 1;
	pwm_set_wrap(slice, wrap);
	const u16 level = (wrap + 1) / 2;
	pwm_set_chan_level(slice, channel, level);

	if (state.sound.anim & SOUND_WAIL) {
		if (rising) freq += RISING_STEP_STEEP;
		else freq -= FALLING_STEP_STEEP;
	} else {
		if (rising) freq += RISING_STEP;
		else freq -= FALLING_STEP;
	}

	if (rising && freq > FREQ_MAX) {
		rising = false;
		freq = FREQ_MAX;
	} else if (!rising && freq < FREQ_MIN) {
		rising = true;
		freq = FREQ_MIN;
	}

	looping_for += RENDER_TICK;
	if (looping_for >= LOOP_FOR_SECONDS * 1'000'000) state.sound.anim = SOUND_OFF;
}

void sound_animation() {
	if ((state.sound.anim & (SOUND_LOOP | SOUND_WAIL) && !state.sound.off)) {
		anim_loop();
	} else if (state.sound.anim == SOUND_OFF) {
		pwm_set_chan_level(slice, channel, 0);
		rising = true;
		freq = FREQ_MIN;
		looping_for = 0;
	}
}
