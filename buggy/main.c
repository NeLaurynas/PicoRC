// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#include <stdio.h>
#include <hardware/clocks.h>
#include <pico/cyw43_arch.h>
#include <pico/cyw43_driver.h>
#include <pico/stdio.h>
#include <pico/time.h>
#include "boards/pico2_w.h"
#include <btstack_run_loop.h>
#include <uni.h>
#include <pico/multicore.h>

#include "renderer.h"
#include "sdkconfig.h"

#include "defines/config.h"

#undef PICO_FLASH_ASSERT_ON_UNSAFE
#define PICO_FLASH_ASSERT_ON_UNSAFE 0

struct uni_platform* get_rc_platform(void);

int main() {
	set_sys_clock_khz(25'000, false);

	stdio_init_all();

#if DBG
	sleep_ms(2000);
	printf("Slept for 2 secondss\n");
#endif

	// initialize CYW43 driver architecture (will enable BT if/because CYW43_ENABLE_BLUETOOTH == 1)
	if (cyw43_arch_init_with_country(CYW43_COUNTRY_LITHUANIA)) {
		loge("failed to initialise cyw43_arch\n");
		return -1;
	}
	cyw43_set_pio_clock_divisor(1, 0);
	cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
	uni_platform_set_custom(get_rc_platform());
	uni_init(0, NULL);

	multicore_launch_core1(renderer_loop);

	// does not return
	btstack_run_loop_execute();
}
