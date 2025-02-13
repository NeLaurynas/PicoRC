// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#ifndef CONFIG_H
#define CONFIG_H

#define RENDER_TICK 10'000

#define XY_DEAD_ZONE 30
#define XY_MAX 510

#define TRIG_DEAD_ZONE 10
#define TRIG_MAX 1000

#define MOD_ENGINE_MAIN_PWM1 15
#define MOD_ENGINE_MAIN_PWM2 14
#define MOD_ENGINE_MAIN_ENABLE1 13
#define MOD_ENGINE_MAIN_ENABLE2 12
#define MOD_ENGINE_MAIN_DMA_CH1 2 // 0 and 1 is used by cyw43 chip or btstack or bluepad32
#define MOD_ENGINE_MAIN_DMA_CH2 3

#define MOD_TURRET_CTRL_DMA_CH 4
#define MOD_TURRET_CTRL_PWM1 16
#define MOD_TURRET_CTRL_PWM2 17 // not really PWM - on/off
#define MOD_TURRET_CTRL_ENABLE1 19
#define MOD_TURRET_CTRL_ENABLE2 18
#define MOD_TURRET_CTRL_ENABLE3 20
#define MOD_TURRET_CTRL_ENABLE4 21

#define MOD_LEDS_WHITE 28
#define MOD_LEDS_RED 5

#endif //CONFIG_H
