// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#ifndef CONFIG_H
#define CONFIG_H

#define RENDER_TICK 10'000

#define MOD_ENGINE_XY_DEAD_ZONE 40
#define MOD_ENGINE_XY_MAX 510

#define TRIG_DEAD_ZONE 10
#define TRIG_MAX 1000

#define MOD_LIGHTS_BLINKERS_LEFT   18
#define MOD_LIGHTS_BLINKERS_RIGHT  20
#define MOD_LIGHTS_HEAD            22
#define MOD_LIGHTS_TAIL            12 // two 200 Ω resistors from + to led

#define MOD_ENGINES_PWM_1		15
#define MOD_ENGINES_PWM_2		11
#define MOD_ENGINES_PWM_3		17
#define MOD_ENGINES_ENABLE_DRIVE_1	14
#define MOD_ENGINES_ENABLE_STEER	13

#endif //CONFIG_H
