// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#include "engines.h"

#include "state.h"
#include "utils.h"
#include "defines/config.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "shared_modules/v_monitor/v_monitor.h"

static u16 slice1, slice2, slice3 = 0;
static u16 channel1, channel2, channel3 = 0;

void engines_init() {
	gpio_init(MOD_ENGINES_ENABLE_DRIVE_1);
	gpio_init(MOD_ENGINES_ENABLE_STEER);
	gpio_set_dir(MOD_ENGINES_ENABLE_DRIVE_1, true);
	gpio_set_dir(MOD_ENGINES_ENABLE_STEER, true);
	gpio_set_function(MOD_ENGINES_PWM_1, GPIO_FUNC_PWM);
	gpio_set_function(MOD_ENGINES_PWM_2, GPIO_FUNC_PWM);
	gpio_set_function(MOD_ENGINES_PWM_3, GPIO_FUNC_PWM);

	slice1 = pwm_gpio_to_slice_num(MOD_ENGINES_PWM_1);
	slice2 = pwm_gpio_to_slice_num(MOD_ENGINES_PWM_2);
	slice3 = pwm_gpio_to_slice_num(MOD_ENGINES_PWM_3);
	channel1 = pwm_gpio_to_channel(MOD_ENGINES_PWM_1);
	channel2 = pwm_gpio_to_channel(MOD_ENGINES_PWM_2);
	channel3 = pwm_gpio_to_channel(MOD_ENGINES_PWM_3);

	// init PWM 1
	constexpr auto top = 1000;
	const auto clk_div = utils_calculate_pwm_divider(top, 15);
	utils_printf("ENGINES PWM CLK DIV: %f\n", clk_div);
	auto pwm_c1 = pwm_get_default_config();
	pwm_c1.top = top;
	pwm_init(slice1, &pwm_c1, false);
	pwm_set_clkdiv(slice1, clk_div);
	pwm_set_phase_correct(slice1, false);
	pwm_set_enabled(slice1, true);

	// init PWM 2
	auto pwm_c2 = pwm_get_default_config();
	pwm_c2.top = top;
	pwm_init(slice2, &pwm_c2, false);
	pwm_set_clkdiv(slice2, clk_div);
	pwm_set_phase_correct(slice2, false);
	pwm_set_enabled(slice2, true);

	// init PWM 3
	auto pwm_c3 = pwm_get_default_config();
	pwm_c3.top = top;
	pwm_init(slice3, &pwm_c3, false);
	pwm_set_clkdiv(slice3, clk_div);
	pwm_set_phase_correct(slice3, false);
	pwm_set_enabled(slice3, true);

}

static void adjust_pwm(i32 *pwm) {
	if (*pwm == 0) return;
	if (*pwm >= 900) return;

	u32 out;
	if (*pwm <= 100) out = 4UL * *pwm;
	else out = 400UL + (5UL * (*pwm - 100UL)) / 8UL;

	*pwm = (i32)out;
}

static inline float beans_reduce_by_battery_level() {
	const float voltage = v_monitor_voltage(false);
	constexpr float limit = MOD_VMON_DEFAULT_REF + 0.3f;
	return (voltage <= limit) ? 1.0f : (limit / voltage);
}

static void set_motor_ctrl(const bool drive_engine, const i32 val, u16 pwm) {
	static constexpr u16 pwm_zero = 0;
	static constexpr float steer_beans_reduce = 0.50f;
	static constexpr float not_full_beans_reduce = 0.45f;
	static constexpr float full_beans_reduce = 0.75f;

	pwm = (u16)((float)pwm * beans_reduce_by_battery_level());

	if (drive_engine) {
		if (!current_state.full_beans) pwm = (u16)((float)pwm * not_full_beans_reduce);
		else pwm = (u16)((float)pwm * full_beans_reduce);

		if (val > 0) {
			gpio_put(MOD_ENGINES_ENABLE_DRIVE_1, true);
			pwm_set_chan_level(slice3, channel3, pwm_zero);
			pwm_set_chan_level(slice1, channel1, pwm);
		} else if (val < 0) {
			gpio_put(MOD_ENGINES_ENABLE_DRIVE_1, true);
			pwm_set_chan_level(slice1, channel1, pwm_zero);
			pwm_set_chan_level(slice3, channel3, pwm);
		} else {
			gpio_put(MOD_ENGINES_ENABLE_DRIVE_1, false);
			pwm_set_chan_level(slice1, channel1, pwm_zero);
			pwm_set_chan_level(slice3, channel3, pwm_zero);
		}
	} else {
		pwm = (u16)((float)pwm * steer_beans_reduce);
		gpio_put(MOD_ENGINES_ENABLE_STEER, val > 0 ? false : val != 0);
		pwm_set_chan_level(slice2, channel2, pwm);
	}
}

void engines_drive(const i32 val) {
	i32 pwm = utils_scaled_pwm_percentage(val, TRIG_DEAD_ZONE, TRIG_MAX) * 10;
	adjust_pwm(&pwm);

	set_motor_ctrl(true, val, pwm);
}

void engines_steer(const i32 val) {
	utils_printf("%ld\n", val);
	i32 pwm = utils_scaled_pwm_percentage(val, MOD_ENGINE_XY_DEAD_ZONE, MOD_ENGINE_XY_MAX) * 10;
	adjust_pwm(&pwm);

	set_motor_ctrl(false, val, pwm);
}
