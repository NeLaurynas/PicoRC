// emergency_led_alternating.c
// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#include "emergency_leds.h"

#include <hardware/gpio.h>
#include <hardware/pwm.h>

#include "state.h"
#include "defines/config.h"

#define LED_PWM_WRAP           255
#define LED_BRIGHTNESS_MIN     0.0f
#define LED_BRIGHTNESS_MAX     ((float)LED_PWM_WRAP)
#define LED_RISING_STEP        18.0f
#define LED_FALLING_STEP       9.0f

#define BLINK_ON_TICKS         3
#define BLINK_OFF_TICKS        6
#define BLINKS_PER_SEQUENCE    3

static u32 slice1, channel1;
static u32 slice2, channel2;

static int normal_active_led = 0;
static float normal_brightness = LED_BRIGHTNESS_MIN;
static bool normal_rising = true;
static int wail_active_led = 0;
static int blink_count = 0;
static int blink_phase = 1;
static int tick_count = 0;

void emergency_leds_init() {
	// init PWM 1
	gpio_set_function(MOD_EMERGENCY_LEDS1, GPIO_FUNC_PWM);
	slice1 = pwm_gpio_to_slice_num(MOD_EMERGENCY_LEDS1);
	channel1 = pwm_gpio_to_channel(MOD_EMERGENCY_LEDS1);

	auto pwm_c1 = pwm_get_default_config();
	pwm_config_set_clkdiv(&pwm_c1, 1.0f);
	pwm_init(slice1, &pwm_c1, true);
	pwm_set_wrap(slice1, LED_PWM_WRAP);
	pwm_set_chan_level(slice1, channel1, 0);

	// init PWM 2
	gpio_set_function(MOD_EMERGENCY_LEDS2, GPIO_FUNC_PWM);
	slice2 = pwm_gpio_to_slice_num(MOD_EMERGENCY_LEDS2);
	channel2 = pwm_gpio_to_channel(MOD_EMERGENCY_LEDS2);

	auto pwm_c2 = pwm_get_default_config();
	pwm_config_set_clkdiv(&pwm_c2, 1.0f);
	pwm_init(slice2, &pwm_c2, true);
	pwm_set_wrap(slice2, LED_PWM_WRAP);
	pwm_set_chan_level(slice2, channel2, 0);
}

static void normal_anim_loop() {
	if (normal_rising) {
		normal_brightness += LED_RISING_STEP;
		if (normal_brightness >= LED_BRIGHTNESS_MAX) {
			normal_brightness = LED_BRIGHTNESS_MAX;
			normal_rising = false;
		}
	} else {
		normal_brightness -= LED_FALLING_STEP;
		if (normal_brightness <= LED_BRIGHTNESS_MIN) {
			normal_brightness = LED_BRIGHTNESS_MIN;
			normal_rising = true;
			normal_active_led = 1 - normal_active_led;
		}
	}

	if (normal_active_led == 0) {
		pwm_set_chan_level(slice1, channel1, (u16)normal_brightness);
		pwm_set_chan_level(slice2, channel2, 0);
	} else {
		pwm_set_chan_level(slice1, channel1, 0);
		pwm_set_chan_level(slice2, channel2, (u16)normal_brightness);
	}
}

static void wail_anim_loop() {
	if (wail_active_led == 0) {
		pwm_set_chan_level(slice1, channel1, (blink_phase == 1) ? LED_PWM_WRAP : 0);
		pwm_set_chan_level(slice2, channel2, 0);
	} else {
		pwm_set_chan_level(slice2, channel2, (blink_phase == 1) ? LED_PWM_WRAP : 0);
		pwm_set_chan_level(slice1, channel1, 0);
	}

	tick_count++;
	if (blink_phase == 1) {
		if (tick_count >= BLINK_ON_TICKS) {
			tick_count = 0;
			blink_phase = 0;
		}
	} else {
		if (tick_count >= BLINK_OFF_TICKS) {
			tick_count = 0;
			blink_count++;
			if (blink_count >= BLINKS_PER_SEQUENCE) {
				blink_count = 0;
				wail_active_led = 1 - wail_active_led;
			}
			blink_phase = 1;
		}
	}
}

void emergency_leds_animation() {
	if (state.sound.anim == SOUND_OFF) {
		pwm_set_chan_level(slice1, channel1, 0);
		pwm_set_chan_level(slice2, channel2, 0);

		normal_active_led = 0;
		normal_brightness = LED_BRIGHTNESS_MIN;
		normal_rising = true;

		wail_active_led = 0;
		blink_count = 0;
		blink_phase = 1;
		tick_count = 0;
		return;
	}

	if (state.sound.anim & SOUND_WAIL) {
		wail_anim_loop();
	} else if (state.sound.anim & SOUND_LOOP) {
		normal_anim_loop();
	}
}
