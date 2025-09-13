#ifndef SHARED_CONFIG_H
#define SHARED_CONFIG_H

#include <stdint.h>

// TYPEDEFS
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;

#define DBG true

// MCP
/* MCP Layout (one byte) for controlled pins:
 *   bit 7: MCP 1 or MCP 2
 *   bit 6: Bank A or Bank B
 *   rest: 6-bit number (max val 63)
 *    7   6   5   4   3   2   1   0
 *  | M | B |       number          |
 *  ie: #define MOD_NUM_LEDG (u8)((0 << 7) | (1 << 6) | 1) // MCP - first bit - MCP 1, second bit - Bank B - pin 1
 */
#define MOD_MCP_PIN_SDA				16
#define MOD_MCP_PIN_SCL				17
#define MOD_MCP_I2C_PORT			(&i2c0_inst)
#define MOD_MCP_ADDR1				0x20
#define MOD_MCP_ADDR2				0x21
#define MOD_MCP_GPIO_CACHE_MS		6
#define MOD_MCP_WRITE_RETRY_COUNT	2

// MP3
#define MOD_MP3_PIN 15
#define MOD_MP3_DMA_CH1 2
#define MOD_MP3_IRQ 0

// WSLEDS
#define MOD_WSLEDS_LED_COUNT	64
#define MOD_WSLEDS_PIO			pio0
#define MOD_WSLEDS_SM			1
#define MOD_WSLEDS_DMA_CH		1
#define MOD_WSLEDS_PIN			18

// VOLTAGE MONITOR
#define MOD_VMON_RES_POS	296600.0f // 300k ohms
#define MOD_VMON_RES_NEG	68000.0f // 68k ohms
#define MOD_VMON_VREF		3.297f // Pico uses 3.3 volts for reference
#define MOD_VMON_ADC_PIN	26
#define MOD_VMON_ADC		0
#define MOD_VMON_DEFAULT_REF 9.92f

// MISC
#define INTERNAL_LED 0

#endif //SHARED_CONFIG_H
