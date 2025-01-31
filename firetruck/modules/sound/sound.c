#define MINIMP3_ONLY_MP3
#define MINIMP3_IMPLEMENTATION

#include "sound.h"

#include <hardware/clocks.h>
#include <hardware/dma.h>
#include <hardware/gpio.h>
#include <hardware/pwm.h>

#include "minimp3.h"
#include "shared_config.h"
#include "state.h"
#include "utils.h"
#include "data/firetruck_siren.h"
#include "defines/config.h"

#define AUDIO_SAMPLE_RATE    22050      // ~22 kHz
#define AUDIO_BITS           9          // 9-bit PWM => 0..511
#define PWM_WRAP             ((1 << AUDIO_BITS) - 1) // 511

/* increase multiplier on both
 *	buffer size should be big enough to not run out of data, causes stall (then sound is gone:
 *		buffers are primed but not in use, so logic doesn't load new data or launch
 *		dma channel and anim_loop doesn't go to off - TODO - fix? but this is like assert - clearly tells problem has been raised)
 *	also should not be too big, because decoding data can also cause stall?
 */
#define DECODED_BUFF_SIZE	(1152*4)
#define MP3_BUFF_SIZE		(610*4)

typedef struct {
	volatile u16 data[DECODED_BUFF_SIZE];
	volatile bool in_use;
	volatile bool primed;
	volatile u16 size;
} dma_buffer_t;

volatile static dma_buffer_t dma_buffer1 = { 0 };
volatile static dma_buffer_t dma_buffer2 = { 0 };
volatile static mp3d_sample_t pcm_buffer[DECODED_BUFF_SIZE] = { 0 };
volatile static u8 mp3_buffer[MP3_BUFF_SIZE] = { 0 };

static mp3dec_t mp3d;
static u8 slice;
static u8 channel;

u16 *utils_pwm_cc_for_16bit(const u8 slice, const u8 channel) {
	assert(channel == 0 || channel == 1);

	return (u16 *)&pwm_hw->slice[slice].cc + channel;
}

static void get_dma_running() {
	dma_buffer1.in_use = true;
	dma_buffer2.in_use = false;
	dma_channel_transfer_from_buffer_now(MOD_SOUND_DMA_CH1, dma_buffer1.data, dma_buffer1.size);
}

static void decode_mp3_into_dma_buffer(volatile dma_buffer_t *dma_buffer, const u8 *mp3_data, u32 *mp3_offset, const u32 mp3_len) {
	assert(dma_buffer->in_use == false);
	const u8 first = dma_buffer == &dma_buffer1 ? 1 : 2;

	i32 chunk_size = (*mp3_offset + MP3_BUFF_SIZE <= mp3_len)
		? MP3_BUFF_SIZE
		: (i32)(mp3_len - *mp3_offset);
	if (chunk_size == 0) return;

	mp3dec_frame_info_t info;
	u32 samples = 0;
	u32 mp3_local_offset = 0;
	dma_buffer->size = 0;

	memcpy(mp3_buffer, &mp3_data[*mp3_offset], chunk_size);

	while (dma_buffer->size + samples <= DECODED_BUFF_SIZE && mp3_local_offset < chunk_size) {
		samples = mp3dec_decode_frame(&mp3d, &mp3_buffer[mp3_local_offset], chunk_size, &pcm_buffer[dma_buffer->size], &info);

		mp3_local_offset += info.frame_bytes;
		dma_buffer->size += samples;
		chunk_size -= info.frame_bytes;

		if (samples > 0 && info.frame_bytes > 0) {
			// decoded ok
		} else if (samples == 0 && info.frame_bytes > 0) {
			utils_printf("Skipped ID3 or invalid data\n");
			break;
		} else if (samples == 0 && info.frame_bytes == 0) {
			utils_printf("Insufficient data\n");
			break;
		}
	}
	*mp3_offset += mp3_local_offset;

	for (u16 i = 0; i < dma_buffer->size; i++) {
		const u32 shifted = pcm_buffer[i] + 32768u; // -32768..32767 -> 0..65535
		dma_buffer->data[i] = (u16)(shifted >> 7); // 0..511 for 9-bit
	}
	if (dma_buffer->size > 0) dma_buffer->primed = true; // data available
}

static void __isr dma_irq_handler() {
	// Check if channel 1 triggered the IRQ
	if (dma_channel_get_irq0_status(MOD_SOUND_DMA_CH1)) {
		// Clear the interrupt
		dma_channel_acknowledge_irq0(MOD_SOUND_DMA_CH1);

		if (dma_buffer1.in_use) {
			dma_buffer1.in_use = false;
			dma_buffer1.primed = false;
			if (dma_buffer2.primed) {
				dma_buffer2.in_use = true;
				dma_channel_set_read_addr(MOD_SOUND_DMA_CH1, dma_buffer2.data, false);
				dma_channel_set_trans_count(MOD_SOUND_DMA_CH1, dma_buffer2.size, true);
			}
		} else if (dma_buffer2.in_use) {
			dma_buffer2.in_use = false;
			dma_buffer2.primed = false;
			if (dma_buffer1.primed) {
				dma_buffer1.in_use = true;
				dma_channel_set_read_addr(MOD_SOUND_DMA_CH1, dma_buffer1.data, false);
				dma_channel_set_trans_count(MOD_SOUND_DMA_CH1, dma_buffer1.size, true);
			}
		}
	}
}

static void anim_loop() {
	static bool init = false;
	static bool first_run = true;
	static u32 mp3_offset = 0;

	if (!init) {
		init = true;
		first_run = true;
		mp3_offset = 0;
	} else if (!dma_buffer1.primed && !dma_buffer2.primed) {
		if (!dma_buffer1.in_use && !dma_buffer2.in_use) {
			init = false;
			state.sound.anim = SOUND_OFF;
			// set pwm level to zero here?
		}
	}

	if (!dma_buffer1.in_use && !dma_buffer1.primed) {
		decode_mp3_into_dma_buffer(&dma_buffer1, firetruck_siren_loop_all_mp3, &mp3_offset, firetruck_siren_loop_all_mp3_len);
	}
	if (!dma_buffer2.in_use && !dma_buffer2.primed) {
		decode_mp3_into_dma_buffer(&dma_buffer2, firetruck_siren_loop_all_mp3, &mp3_offset, firetruck_siren_loop_all_mp3_len);
	}

	if (first_run) {
		get_dma_running();
		first_run = false;
	}
}

void sound_init() {
	// init PWM
	gpio_set_function(MOD_SOUND_PIN, GPIO_FUNC_PWM);
	slice = pwm_gpio_to_slice_num(MOD_SOUND_PIN);
	channel = pwm_gpio_to_channel(MOD_SOUND_PIN);

	auto pwm_c = pwm_get_default_config();
	pwm_config_set_wrap(&pwm_c, PWM_WRAP);

	const float clk_div = (float)clock_get_hz(clk_sys) / ((float)AUDIO_SAMPLE_RATE * (PWM_WRAP + 1));
	utils_printf("SOUND PWM CLK DIV: %f\n", clk_div);
	pwm_config_set_clkdiv(&pwm_c, clk_div);

	pwm_init(slice, &pwm_c, false);
	pwm_set_chan_level(slice, channel, 0);
	pwm_set_enabled(slice, true);

	// init DMA 1
	if (dma_channel_is_claimed(MOD_SOUND_DMA_CH1)) utils_error_mode(41);
	dma_channel_claim(MOD_SOUND_DMA_CH1);

	auto dma_c1 = dma_channel_get_default_config(MOD_SOUND_DMA_CH1);
	channel_config_set_transfer_data_size(&dma_c1, DMA_SIZE_16);
	channel_config_set_read_increment(&dma_c1, true);
	channel_config_set_write_increment(&dma_c1, false);
	channel_config_set_dreq(&dma_c1, pwm_get_dreq(slice));

	dma_channel_configure(MOD_SOUND_DMA_CH1, &dma_c1, utils_pwm_cc_for_16bit(slice, channel), dma_buffer1.data, 0, false);

	// !!! SET IRQ0 - check with MOD_SOUND_IRQ and change if it's 1 !!!
	dma_channel_set_irq0_enabled(MOD_SOUND_DMA_CH1, true);

	irq_set_exclusive_handler(DMA_IRQ(MOD_SOUND_IRQ), dma_irq_handler);
	irq_set_enabled(DMA_IRQ(MOD_SOUND_IRQ), true);
}

void sound_animation() {
	switch (state.sound.anim) {
		case SOUND_OFF:
			pwm_set_chan_level(slice, channel, 0);
			break;
		case SOUND_LOOP:
			anim_loop();
			break;
	}
}
