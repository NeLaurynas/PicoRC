/*
 * Example of Ping-Pong DMA for 9-bit PWM audio at ~22 kHz from an MP3 (minimp3).
 *
 * Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/sync.h"

#define MINIMP3_NONSTANDARD_BUT_LOGICAL
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"

// The MP3 data you want to play (22 kHz, ~80 kbps)
#include "data/firetruck_siren.h"  // defines firetruck_siren_loop_all_mp3[] and firetruck_siren_loop_all_mp3_len
#include "defines/config.h"

//--------------------------------------------------------------------------------------
// Public interface (per your request)
//--------------------------------------------------------------------------------------
void sound_init(void);
void sound_play(void);

static void __isr dma_irq0_handler(void);
static uint32_t decode_next_chunk_into(uint16_t *out_buf, uint32_t max_samples);

//--------------------------------------------------------------------------------------
// Configurable parameters
//--------------------------------------------------------------------------------------
#define AUDIO_SAMPLE_RATE    22000      // ~22 kHz
#define AUDIO_BITS           9          // 9-bit PWM => 0..511
#define PWM_WRAP             ((1 << AUDIO_BITS) - 1) // 511

// Max decoded samples per frame. For MP3 layer3, 1152 stereo => 2304. We allow a bit extra.
#define MAX_SAMPLES_PER_FRAME  2304

// We'll do ping-pong with two buffers; each buffer can hold up to MAX_SAMPLES_PER_FRAME
static uint16_t dma_buffer[2][MAX_SAMPLES_PER_FRAME];

// We'll read the MP3 in lumps
#define MP3_CHUNK_SIZE        1152

//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------
static mp3dec_t    mp3d;
static uint32_t    mp3_read_offset = 0;

static uint8_t     mp3_chunk_buf[MP3_CHUNK_SIZE];
static short       pcm[MAX_SAMPLES_PER_FRAME];  // 16-bit signed PCM decode buffer

// Two DMA channels for ping-pong
static int dma_chan[2] = {-1, -1};

// For referencing the PWM slice/channel we are driving
static uint  pwm_slice;
static uint  pwm_chan;  // 0 for A, 1 for B

// Track how many samples are in each ping/pong buffer
static uint  samples_in_buffer[2] = {0, 0};

// For “which buffer is currently being prepared in the code”
static uint  current_buffer_index = 0;

static volatile bool dma_in_use[2] = { false, false }; // which channel is playing?

static void pwm_audio_init(uint gpio)
{
    // Set the pin function to PWM
    gpio_set_function(gpio, GPIO_FUNC_PWM);

    // Figure out which slice/channel this GPIO is on
    pwm_slice = pwm_gpio_to_slice_num(gpio);
    pwm_chan  = pwm_gpio_to_channel(gpio);

    // Default config
    pwm_config config = pwm_get_default_config();
    pwm_config_set_wrap(&config, PWM_WRAP);

    const float sys_clk_hz = 18000000.0f;
    float divider = sys_clk_hz / ((float)AUDIO_SAMPLE_RATE * (PWM_WRAP + 1));
    pwm_config_set_clkdiv(&config, divider);

    // Initialize & enable
    pwm_init(pwm_slice, &config, false);
    pwm_set_chan_level(pwm_slice, pwm_chan, 0);
    pwm_set_enabled(pwm_slice, true);
}

static volatile uint16_t * get_slice_cc_ptr(uint slice, uint chan)
{
    // Each slice has a single 32-bit CC register: lower 16 bits for channel A, upper 16 for B.
    volatile uint32_t *cc_reg_32 = &pwm_hw->slice[slice].cc;
    // Now treat that as a 16-bit pointer:
    volatile uint16_t *cc_reg_16 = (volatile uint16_t *)cc_reg_32;
    if (chan == PWM_CHAN_B) {
        cc_reg_16++;  // move to upper half
    }
    return cc_reg_16;
}

static void setup_ping_pong_dma(void)
{
    // Claim two DMA channels
    dma_chan[0] = dma_claim_unused_channel(true);
    dma_chan[1] = dma_claim_unused_channel(true);

    // The 16-bit pointer to the relevant channel (A or B) of the PWM slice
    volatile uint16_t *pwm_compare_16 = get_slice_cc_ptr(pwm_slice, pwm_chan);

    // Configure channel 0
    {
        dma_channel_config c0 = dma_channel_get_default_config(dma_chan[0]);
        channel_config_set_transfer_data_size(&c0, DMA_SIZE_16); // 16-bit
        channel_config_set_read_increment(&c0, true);            // read from buffer increments
        channel_config_set_write_increment(&c0, false);          // always write to same register
        // pace by PWM slice (once per PWM cycle)
        channel_config_set_dreq(&c0, DREQ_PWM_WRAP0 + pwm_slice);

        // **Important**: chain channel 0 -> channel 1
        channel_config_set_chain_to(&c0, dma_chan[1]);

        // We'll initially configure it with 0 transfers; we'll set actual read address + count
        // when we actually start playing.
        dma_channel_configure(
            dma_chan[0],
            &c0,
            pwm_compare_16,      // write address
            NULL,                // read address (will set later)
            0,                   // number of transfers (will set later)
            false                // don't start yet
        );
    }

    // Configure channel 1
    {
        dma_channel_config c1 = dma_channel_get_default_config(dma_chan[1]);
        channel_config_set_transfer_data_size(&c1, DMA_SIZE_16);
        channel_config_set_read_increment(&c1, true);
        channel_config_set_write_increment(&c1, false);
        channel_config_set_dreq(&c1, DREQ_PWM_WRAP0 + pwm_slice);

        // chain channel 1 -> channel 0
        channel_config_set_chain_to(&c1, dma_chan[0]);

        dma_channel_configure(
            dma_chan[1],
            &c1,
            pwm_compare_16,      // always write to same compare reg
            NULL,                // read address (to set later)
            0,                   // count (to set later)
            false
        );
    }

    dma_channel_set_irq0_enabled(dma_chan[0], true);
    dma_channel_set_irq0_enabled(dma_chan[1], true);

    irq_set_exclusive_handler(DMA_IRQ_0, dma_irq0_handler);
    irq_set_enabled(DMA_IRQ_0, true);
}

static void __isr dma_irq0_handler(void)
{
    // Check if channel 0 triggered the IRQ
    if (dma_hw->ints0 & (1u << dma_chan[0])) {
        // Clear the interrupt
        dma_hw->ints0 = (1u << dma_chan[0]);
        dma_in_use[0] = false;

        // We just finished playing buffer 0, so decode next chunk into buffer[0]
        // unless we're done with MP3
        if (mp3_read_offset < firetruck_siren_loop_all_mp3_len) {
            // fill buffer 0
            samples_in_buffer[0] = decode_next_chunk_into(dma_buffer[0], MAX_SAMPLES_PER_FRAME);
            // reconfigure channel 0 with the new read address + length
            dma_channel_set_read_addr(dma_chan[0], dma_buffer[0], false);
            dma_channel_set_trans_count(dma_chan[0], samples_in_buffer[0], false);
        }
        else {
            // No more MP3 data => set length=0 => silent
            dma_channel_set_read_addr(dma_chan[0], dma_buffer[0], false);
            dma_channel_set_trans_count(dma_chan[0], 0, false);
        }
    }

    // Check if channel 1 triggered the IRQ
    if (dma_hw->ints0 & (1u << dma_chan[1])) {
        dma_hw->ints0 = (1u << dma_chan[1]);
        dma_in_use[1] = false;

        // Just finished playing buffer[1], so decode next chunk
        if (mp3_read_offset < firetruck_siren_loop_all_mp3_len) {
            samples_in_buffer[1] = decode_next_chunk_into(dma_buffer[1], MAX_SAMPLES_PER_FRAME);
            dma_channel_set_read_addr(dma_chan[1], dma_buffer[1], false);
            dma_channel_set_trans_count(dma_chan[1], samples_in_buffer[1], false);
        }
        else {
            dma_channel_set_read_addr(dma_chan[1], dma_buffer[1], false);
            dma_channel_set_trans_count(dma_chan[1], 0, false);
        }
    }
}

static uint32_t decode_next_chunk_into(uint16_t *out_buf, uint32_t max_samples)
{
    // 1) Copy up to MP3_CHUNK_SIZE from the big MP3 array
    uint32_t chunk_size = (mp3_read_offset + MP3_CHUNK_SIZE <= firetruck_siren_loop_all_mp3_len)
                            ? MP3_CHUNK_SIZE
                            : (firetruck_siren_loop_all_mp3_len - mp3_read_offset);

    memcpy(mp3_chunk_buf, &firetruck_siren_loop_all_mp3[mp3_read_offset], chunk_size);

    // 2) Decode
    mp3dec_frame_info_t info;
    int samples = mp3dec_decode_frame(&mp3d, mp3_chunk_buf, chunk_size, pcm, &info);

    // Advance offset by the bytes consumed
    mp3_read_offset += info.frame_bytes;

    if (samples <= 0 || info.frame_bytes <= 0) {
        // No valid samples => return 0 => silence
        return 0;
    }

    // minimp3 returns 16-bit signed => scale to 9-bit
    if ((uint32_t)samples > max_samples) {
        samples = max_samples; // clamp
    }

    for (int i = 0; i < samples; i++) {
        int16_t raw_sample  = pcm[i];           // -32768..32767
        uint32_t shifted    = raw_sample + 32768u;  // 0..65535
        uint16_t pwm_value  = (uint16_t)(shifted >> 7); // 0..511 for 9-bit
        out_buf[i]          = pwm_value;
    }

    return samples;
}

void sound_init(void)
{
    // Initialize MP3 decoder
    mp3dec_init(&mp3d);
    mp3_read_offset = 0;

    // Initialize the PWM on GPIO 15 (example). Adjust as needed.
    pwm_audio_init(MOD_SOUND_PIN);

    // Setup two channels in ping-pong
    setup_ping_pong_dma();

	sound_play();
}

void sound_play(void)
{
    samples_in_buffer[0] = decode_next_chunk_into(dma_buffer[0], MAX_SAMPLES_PER_FRAME);
    samples_in_buffer[1] = decode_next_chunk_into(dma_buffer[1], MAX_SAMPLES_PER_FRAME);

    dma_channel_set_read_addr(dma_chan[0], dma_buffer[0], false);
    dma_channel_set_trans_count(dma_chan[0], samples_in_buffer[0], false);

    dma_channel_set_read_addr(dma_chan[1], dma_buffer[1], false);
    dma_channel_set_trans_count(dma_chan[1], samples_in_buffer[1], false);

    // Mark them in_use if they have >0 samples
    dma_in_use[0] = (samples_in_buffer[0] > 0);
    dma_in_use[1] = (samples_in_buffer[1] > 0);

    dma_start_channel_mask((1u << dma_chan[0]));

    // Optionally, just return. The main loop continues; the DMA IRQ refills buffers.
    // If you want to block until done, do something like:
    while (mp3_read_offset < firetruck_siren_loop_all_mp3_len) {
        // We still have MP3 data left
        tight_loop_contents(); // do any other tasks
    }

    bool still_playing = true;
    while (still_playing) {
        still_playing = dma_in_use[0] || dma_in_use[1];
        tight_loop_contents();
    }

    // Optionally set PWM compare to 0 or a neutral level
    pwm_set_chan_level(pwm_slice, pwm_chan, 0);
}

void sound_animation() {
}
