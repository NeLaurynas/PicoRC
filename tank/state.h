// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#ifndef STATE_H
#define STATE_H

#include <shared_config.h>

typedef struct {
	i16 x;
	i16 y;

	i16 rx;
	i16 ry;

	bool btn_a;
	bool btn_x;
	bool btn_b;
	bool btn_y;

	bool btn_start;
	bool btn_select;

	bool shoulder_l;
	bool shoulder_r;

	u16 throttle;
	u16 brake;
} State;

typedef struct {
	i16 x;
	i16 y;

	i16 rx;
	i16 ry;

	bool btn_a;
	bool btn_x;
	bool btn_b;
	bool btn_y;

	bool btn_start;
	bool btn_select;

	bool white_leds;
	bool red_led;

	bool advanced_mode;

	bool shoulder_l;
	bool shoulder_r;

	u16 throttle;
	u16 brake;
} CurrentState;

volatile extern State state;
volatile extern CurrentState current_state;

#endif //STATE_H
