// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#include <stdio.h>
#include <hardware/clocks.h>
#include <pico/cyw43_arch.h>
#include <pico/cyw43_driver.h>
#include <pico/stdio.h>
#include <pico/time.h>
#include "boards/pico2_w.h"

#include "defines/config.h"

int main() {
	set_sys_clock_khz(18'000, false);

	stdio_init_all();
	cyw43_arch_init_with_country(CYW43_COUNTRY_LITHUANIA);
	cyw43_set_pio_clock_divisor(1, 0);

	// step 1 - acquire connection
	// step 2 - read inputz

	static bool on = true;
	for (;;) {
		cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on);
		on = !on;
		sleep_ms(1000);
	}
}
