// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#ifndef CONFIG_H
#define CONFIG_H

#include <pico/types.h>

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;

#define DBG true

#define RENDER_TICK 10'000

#define MOD_ENGINE_XY_DEAD_ZONE 40
#define MOD_ENGINE_XY_MAX 510

#define MOD_TUR_ROT_1_PIN 13
#define MOD_TUR_ROT_2_PIN 12
#define MOD_TUR_ROT_3_PIN 11
#define MOD_TUR_ROT_4_PIN 10
#define MOD_TUR_ROT_DMA_CH3 2
#define MOD_TUR_ROT_DMA_CH4 3

#define INTERNAL_LED 0

#endif //CONFIG_H
