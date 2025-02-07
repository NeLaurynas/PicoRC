// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#ifndef CONFIG_H
#define CONFIG_H

#define RENDER_TICK 10'000

#define MOD_ENGINE_XY_DEAD_ZONE 40
#define MOD_ENGINE_XY_MAX 490

#define TRIG_DEAD_ZONE 10
#define TRIG_MAX 1000

#define MOD_SOUND_PIN 21
#define MOD_SOUND_DMA_CH 2
#define MOD_SOUND_IRQ 0

#define MOD_EMERGENCY_LEDS1 16
#define MOD_EMERGENCY_LEDS2 17

#define MOD_ENGINE_PWM1 14 // drive
#define MOD_ENGINE_PWM2 9 // turn
#define MOD_ENGINE_DMA_CH1 3 // drive
#define MOD_ENGINE_DMA_CH2 4 // turn
#define MOD_ENGINE_ENABLE2 13 // drive
#define MOD_ENGINE_ENABLE1 12 // drive
#define MOD_ENGINE_ENABLE3 10 // turn
#define MOD_ENGINE_ENABLE4 11 // turn

#endif //CONFIG_H
