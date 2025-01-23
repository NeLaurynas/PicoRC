// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#include "leds.h"

#include <hardware/gpio.h>

#include "utils.h"
#include "defines/config.h"

void leds_init() {
	gpio_init(MOD_LEDS_RED);
	gpio_init(MOD_LEDS_WHITE);
	gpio_set_dir(MOD_LEDS_RED, true);
	gpio_set_dir(MOD_LEDS_WHITE, true);

	gpio_set_drive_strength(MOD_LEDS_WHITE, GPIO_DRIVE_STRENGTH_2MA);
	gpio_set_drive_strength(MOD_LEDS_RED, GPIO_DRIVE_STRENGTH_2MA);
}

void leds_toggle_red(const bool on) {
	gpio_put(MOD_LEDS_RED, on);
}

void leds_toggle_white(const bool on) {
	gpio_put(MOD_LEDS_WHITE, on);
}
