// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#include "turret_rotation.h"

#include <stdlib.h>
#include <hardware/dma.h>
#include <hardware/gpio.h>
#include <hardware/pwm.h>
#include <pico/time.h>

#include "utils.h"
#include "defines/config.h"

static uint slice3 = 0;
static uint slice4 = 0;
static uint channel3 = 0;
static uint channel4 = 0;
static u32 buffer_cc3[1] = { 0 };
static u32 buffer_cc4[1] = { 0 };

void turret_rotation_init() {
	return;
	gpio_init(MOD_TUR_ROT_1_PIN);
	gpio_init(MOD_TUR_ROT_2_PIN);
	gpio_set_dir(MOD_TUR_ROT_1_PIN, true);
	gpio_set_dir(MOD_TUR_ROT_2_PIN, true);
	gpio_set_function(MOD_TUR_ROT_3_PIN, GPIO_FUNC_PWM);
	gpio_set_function(MOD_TUR_ROT_4_PIN, GPIO_FUNC_PWM);

	slice3 = pwm_gpio_to_slice_num(MOD_TUR_ROT_3_PIN);
	channel3 = pwm_gpio_to_channel(MOD_TUR_ROT_3_PIN);
	slice4 = pwm_gpio_to_slice_num(MOD_TUR_ROT_4_PIN);
	channel4 = pwm_gpio_to_channel(MOD_TUR_ROT_4_PIN);

	// init PWM
	auto pwm_c3 = pwm_get_default_config();
	pwm_c3.top = 10000;
	pwm_init(slice3, &pwm_c3, false);
	pwm_set_clkdiv(slice3, 3.f); // 3.f for 5 khz frequency (2.f for 7.5 khz 1.f for 15 khz)
	pwm_set_phase_correct(slice3, false);
	pwm_set_enabled(slice3, true);

	auto pwm_c4 = pwm_get_default_config();
	pwm_c4.top = 10000;
	pwm_init(slice4, &pwm_c4, false);
	pwm_set_clkdiv(slice4, 3.f);
	pwm_set_phase_correct(slice4, false);
	pwm_set_enabled(slice4, true);
	sleep_ms(1);

	// init DMA
	dma_channel_claim(MOD_TUR_ROT_DMA_CH3);
	auto dma_cc_c3 = dma_channel_get_default_config(MOD_TUR_ROT_DMA_CH3);
	channel_config_set_transfer_data_size(&dma_cc_c3, DMA_SIZE_32);
	channel_config_set_read_increment(&dma_cc_c3, false);
	channel_config_set_write_increment(&dma_cc_c3, false);
	channel_config_set_dreq(&dma_cc_c3, DREQ_FORCE);
	dma_channel_configure(MOD_TUR_ROT_DMA_CH3, &dma_cc_c3, &pwm_hw->slice[slice3].cc, buffer_cc3, 1, false);

	dma_channel_claim(MOD_TUR_ROT_DMA_CH4);
	auto dma_cc_c4 = dma_channel_get_default_config(MOD_TUR_ROT_DMA_CH4);
	channel_config_set_transfer_data_size(&dma_cc_c4, DMA_SIZE_32);
	channel_config_set_read_increment(&dma_cc_c4, false);
	channel_config_set_write_increment(&dma_cc_c4, false);
	channel_config_set_dreq(&dma_cc_c4, DREQ_FORCE);
	dma_channel_configure(MOD_TUR_ROT_DMA_CH4, &dma_cc_c4, &pwm_hw->slice[slice4].cc, buffer_cc4, 1, false);
	sleep_ms(1);
}

static u16 get_scaled_pwm_percentage(i16 val) {
	const u16 x = abs(val);
	if (x <= MOD_ENGINE_XY_DEAD_ZONE) {
		return 0;
	} if (x >= MOD_ENGINE_XY_MAX) {
		return 10000;
	}

	return ((x - MOD_ENGINE_XY_DEAD_ZONE) * 100 / (MOD_ENGINE_XY_MAX - MOD_ENGINE_XY_DEAD_ZONE)) * 100;
}

void turret_rotation_rotate(i16 val) {
	return;
	const bool neg = val < 0;
	const auto pwm_perc = get_scaled_pwm_percentage(val);
	if (neg) {
		// disable one direction
		gpio_put(MOD_TUR_ROT_2_PIN, false);
		buffer_cc3[0] = 0;
		dma_channel_transfer_from_buffer_now(MOD_TUR_ROT_DMA_CH3, buffer_cc3, 1);
		// enable other direction
		gpio_put(MOD_TUR_ROT_1_PIN, true);
		buffer_cc4[0] = channel4 == 1 ? pwm_perc << 16 : pwm_perc;
		dma_channel_transfer_from_buffer_now(MOD_TUR_ROT_DMA_CH4, buffer_cc4, 1);
	} else {
		// disable one direction
		gpio_put(MOD_TUR_ROT_1_PIN, false);
		buffer_cc4[0] = 0;
		dma_channel_transfer_from_buffer_now(MOD_TUR_ROT_DMA_CH4, buffer_cc4, 1);
		// enable other direction
		gpio_put(MOD_TUR_ROT_2_PIN, val != 0);
		buffer_cc3[0] = channel3 == 1 ? pwm_perc << 16 : pwm_perc;
		dma_channel_transfer_from_buffer_now(MOD_TUR_ROT_DMA_CH3, buffer_cc3, 1);
	}
}
