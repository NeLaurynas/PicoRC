// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#ifndef STATE_H
#define STATE_H

#include <shared_config.h>

// ENUMS

typedef struct {
	i32 x;
	i32 y;

	i32 rx;
	i32 ry;

	bool btn_a;
	bool btn_x;
	bool btn_b;
	bool btn_y;

	bool btn_start;
	bool btn_select;

	bool shoulder_l;
	bool shoulder_r;

	bool pad_up;
	bool pad_down;
	bool pad_left;
	bool pad_right;

	i32 throttle;
	i32 brake;

	struct {
		bool braking;
		i32 tail;
	} lights;
} State;

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

	bool pad_up;
	bool pad_down;
	bool pad_left;
	bool pad_right;

	bool btn_start;
	bool btn_select;

	i32 throttle;
	i32 brake;

	bool full_beans;

	struct {
		bool blinkers_left;
		bool blinkers_right;
		bool braking;

		i32 head;
		i32 tail;

		bool hazards;
	} lights;
} CurrentState;

volatile extern State state;
volatile extern CurrentState current_state;

#endif //STATE_H
