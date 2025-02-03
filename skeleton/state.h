// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#ifndef STATE_Htypedef uint32_t u32;
#define STATE_H

#include <shared_config.h>

typedef struct {
	i32 x;
	i32 y;

	i32 rx;
	i32 ry;

	bool btn_a;
	bool btn_x;
	bool btn_b;
	bool btn_y;

	bool shoulder_l;
	bool shoulder_r;

	i32 throttle;
	i32 brake;
} State;

typedef struct {
	i32 x;
	i32 y;

	i32 rx;
	i32 ry;

	bool btn_a;
	bool btn_a_toggle;
	bool btn_x;
	bool btn_b;
	bool btn_y;

	bool shoulder_l;
	bool shoulder_r;

	i32 throttle;
	i32 brake;
} CurrentState;

volatile extern State state;
volatile extern CurrentState current_state;

#endif //STATE_H
