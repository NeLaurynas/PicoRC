// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LIGHTS_H
#define LIGHTS_H

/* Resistors:
 * 3 mm yellow: 100 ohm
 * 5 mm red: 100 ohm
 * 5 mm white: 0 ohm
 * 5 mm blue: 10 ohm
 */

void lights_init();

void lights_set_blinkers(bool left, bool on);

void lights_head_up();

void lights_tail();

void lights_animation();

#endif //LIGHTS_H
