// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LIGHTS_H
#define LIGHTS_H

/* Resistors:
 * 3 mm yellow: 100 ohm
 * 5 mm red: 100 ohm
 * 5 mm white: 10 ohm
 * 5 mm blue: 10 ohm
 */

void lights_init();

void lights_set_blinkers(bool left, bool on);

void lights_inc_head();

void lights_animation();

#endif //LIGHTS_H
