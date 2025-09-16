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
#include "utils.h"
#include "shared_modules/cpu_cores/cpu_cores.h"

#include "defines/config.h"

#undef PICO_FLASH_ASSERT_ON_UNSAFE
#define PICO_FLASH_ASSERT_ON_UNSAFE 0

static btstack_timer_source_t btstack_qpoll;
static void qpoll_handler(btstack_timer_source_t *ts) {
	i32 command;
	while (queue_try_remove(&mod_cpu_core0_queue, &command)) if (command == CPU_CORES_CMD_SHUTDOWN) btstack_run_loop_trigger_exit();
	btstack_run_loop_set_timer(ts, 100);
	btstack_run_loop_add_timer(ts);
}
void btstack_handler_init() {
	cpu_cores_init_from_core0();
	btstack_run_loop_set_timer_handler(&btstack_qpoll, &qpoll_handler);
	btstack_run_loop_set_timer(&btstack_qpoll, 100);
	btstack_run_loop_add_timer(&btstack_qpoll);
}


struct uni_platform* get_rc_platform(void);

int main() {
	// set_sys_clock_khz(25'000, false);
	set_sys_clock_khz(48'000, false);

	stdio_init_all();
	sleep_ms(4000);

#if DBG
	printf("let'sa go\n");
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

	btstack_handler_init();

	multicore_launch_core1(renderer_loop);

	// does not return
	btstack_run_loop_execute();
	utils_printf("btstack exited\n");

	utils_printf("wifi and bt shutdown\n");
	hci_power_control(HCI_POWER_OFF);
	cyw43_arch_deinit();
	cpu_cores_shutdown_from_core0();
}
