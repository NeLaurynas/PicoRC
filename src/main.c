// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#include <stdio.h>
#include <hardware/clocks.h>
#include <pico/stdio.h>
#include "boards/pico2_w.h"
#include <pico/cyw43_arch.h>
#include <pico/time.h>
#include <pico/cyw43_driver.h>

#include "defines/config.h"

int main() {
	set_sys_clock_khz(96'000, false);

	stdio_init_all();
	cyw43_arch_init_with_country(CYW43_COUNTRY_LITHUANIA);
	// cyw43_set_pio_clock_divisor(2, 0);

	static bool on = true;
	for (;;) {
		cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on);
		on = !on;
		sleep_ms(1000);
		const auto freq_hz = clock_get_hz(clk_sys);
		// printf("asdf %d - %d\n", freq_hz, result);

		const float freq_mhz = (float)freq_hz / 1'000'000.0f;
		printf("System clock: %d Hz, %.2f MHz\n", freq_hz, freq_mhz);
	}
}
