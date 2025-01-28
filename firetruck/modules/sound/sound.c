// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#include <stdio.h>
#include <string.h>
#include <hardware/dma.h>
#include <hardware/irq.h>
#include <hardware/pwm.h>
#include <pico/stdlib.h>
#include <hardware/clocks.h>

#define MINIMP3_ONLY_MP3
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"
#include "state.h"

#include "utils.h"
#include "data/firetruck_siren.h"
#include "defines/config.h"

#define AUDIO_SAMPLE_RATE   22050U       // ~22 kHz
#define AUDIO_BITS           9          // 9-bit PWM => 0..511
#define PWM_WRAP             ((1 << AUDIO_BITS) - 1)
// PCM_BUFFER_SIZE should be big enough to make DMA busy before next animation tick (10 ms, could be more - x2 works, x3 to be safe)
#define PCM_BUFFER_SIZE         (1152*3)
// MP3_BUFFER_SIZE should be big enough for MP3 data to fill PCM_BUFFER_SIZE
#define MP3_BUFFER_SIZE         (610*3)

typedef struct {
	u16 buffer[PCM_BUFFER_SIZE];
	bool done;
	u32 sample_size;
} dma_buffer_t;

static dma_buffer_t dma_buffer1 = { 0 };
static dma_buffer_t dma_buffer2 = { 0 };

static mp3d_sample_t pcm_buffer[PCM_BUFFER_SIZE] = { 0 };
static u8 mp3_buffer[MP3_BUFFER_SIZE] = { 0 };

static mp3dec_t mp3d;
static u8 slice;
static u8 channel;

static const u16 *dma_source_ptr = nullptr;
static u32 dma_sample_count = 0;

static void dma_start(const dma_buffer_t *buffer) {
	dma_source_ptr = buffer->buffer;
	dma_sample_count = buffer->sample_size;

	dma_channel_transfer_from_buffer_now(MOD_SOUND_DMA_CH, dma_source_ptr, dma_sample_count);
}

static void dma_irq_handler(void) {
	dma_hw->ints0 = 1u << MOD_SOUND_DMA_CH; // clear interrupt

	const u16 *finished_buffer = dma_source_ptr;

	if (finished_buffer == dma_buffer1.buffer) {
		dma_buffer1.done = true;
		// launch second buffer eh
		if (!dma_buffer2.done) dma_start(&dma_buffer2);
	} else if (finished_buffer == dma_buffer2.buffer) {
		dma_buffer2.done = true;
		if (!dma_buffer1.done) dma_start(&dma_buffer1);
	}
}

void sound_init(void) {
	// MP3 decoder
	mp3dec_init(&mp3d);

	// PWM
	gpio_set_function(MOD_SOUND_PIN, GPIO_FUNC_PWM);
	slice = pwm_gpio_to_slice_num(MOD_SOUND_PIN);
	channel = pwm_gpio_to_channel(MOD_SOUND_PIN);
	pwm_config config = pwm_get_default_config();
	pwm_config_set_wrap(&config, PWM_WRAP);

	const float sys_clk_hz = 18000000.0f;
	float divider = sys_clk_hz / ((float)AUDIO_SAMPLE_RATE * (PWM_WRAP + 1));
	pwm_config_set_clkdiv(&config, divider);
	// float clk_div = 100.f;
	utils_printf("SOUND PWM CLK DIV: %f\n", divider);
	pwm_config_set_clkdiv(&config, divider);

	pwm_init(slice, &config, false);
	pwm_set_chan_level(slice, channel, 0);
	pwm_set_enabled(slice, true);

	// DMA
	if (dma_channel_is_claimed(MOD_SOUND_DMA_CH)) utils_error_mode(41);
	dma_channel_claim(MOD_SOUND_DMA_CH);

	dma_channel_config dma_c = dma_channel_get_default_config(MOD_SOUND_DMA_CH);
	channel_config_set_transfer_data_size(&dma_c, DMA_SIZE_16);
	channel_config_set_read_increment(&dma_c, true);
	channel_config_set_write_increment(&dma_c, false);
	channel_config_set_dreq(&dma_c, (DREQ_PWM_WRAP0 + slice));

	uint16_t *cc_register_for_16bits = (uint16_t *)&pwm_hw->slice[slice].cc + channel;
	dma_channel_configure(MOD_SOUND_DMA_CH, &dma_c, cc_register_for_16bits, nullptr, 0, false);

	dma_channel_set_irq0_enabled(MOD_SOUND_DMA_CH, true); // irq0 for 0..3 channels, so sound channel...
	irq_set_exclusive_handler(DMA_IRQ_0, dma_irq_handler);
	irq_set_enabled(DMA_IRQ_0, true);

	state.sound.anim = SOUND_LOOP; // debug
}

static void load_dma_buffer(const bool first_buffer, u32 *offset) {
	dma_buffer_t *buffer = first_buffer ? &dma_buffer1 : &dma_buffer2;
	u16 samples = 0;
	buffer->sample_size = 0;
	mp3dec_frame_info_t info = { 0 };
	u32 local_offset = 0;

	u32 mp3_chunk_size = (*offset + MP3_BUFFER_SIZE <= firetruck_siren_loop_all_mp3_len)
		? MP3_BUFFER_SIZE
		: (firetruck_siren_loop_all_mp3_len - *offset);
	memcpy(mp3_buffer, &firetruck_siren_loop_all_mp3[*offset], mp3_chunk_size);

	while (*offset < firetruck_siren_loop_all_mp3_len && buffer->sample_size + samples <= PCM_BUFFER_SIZE && mp3_chunk_size > 0) {
		samples = mp3dec_decode_frame(&mp3d, &mp3_buffer[local_offset], (i32)mp3_chunk_size, &pcm_buffer[buffer->sample_size], &info);
 		if (samples > 0 && info.frame_bytes > 0) {
			// utils_printf("OK!\n");
		} else if (samples == 0 && info.frame_bytes > 0) {
			utils_printf("Skipped ID3 or invalid data\n");
			break;
		} else if (samples == 0 && info.frame_bytes == 0) {
			break;
			utils_printf("Insufficient data\n");
		}
		buffer->sample_size += samples;
		*offset += info.frame_bytes;
		local_offset += info.frame_bytes;
		mp3_chunk_size -= info.frame_bytes;
	}

	for (int i = 0; i < samples; i++) {
		const i16 raw_sample = pcm_buffer[i]; // -32768..32767
		const u32 shifted = raw_sample + 32768u; // 0..65535
		const u16 pwm_value = (uint16_t)(shifted >> 7); // 0..511 for 9-bit
		buffer->buffer[i] = pwm_value;
	}

	buffer->done = false;
}

// so this keeps buffers topped up, irq handler launches another buffer then...
static void anim_loop() {
	static bool init = false;
	static bool no_more_buffer = true;
	static bool first_run = true;
	static u32 offset = 0;

	if (!init) {
		init = true;
		no_more_buffer = false;
		first_run = true;
		offset = 0;
		dma_buffer1.done = true;
		dma_buffer2.done = true;
	} else if (offset >= firetruck_siren_loop_all_mp3_len) {
		// if (no_more_buffer) // TODO: set no more buffer
		// init = false;
		// state.sound.anim = SOUND_OFF; // todo: other buffer might be available?
		// utils_printf("sound off!\n");
		// pwm_set_gpio_level(MOD_SOUND_DMA_CH, 0); //todo
		return;
	}

	if (dma_buffer1.done) load_dma_buffer(true, &offset);
	if (dma_buffer2.done) load_dma_buffer(false, &offset);

	if (first_run) {
		first_run = false;
		utils_printf("go\n");
		dma_start(&dma_buffer1);
	}
}

void sound_animation() {
	if (state.sound.anim & SOUND_LOOP) {
		anim_loop();
	}
}
