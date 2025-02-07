// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#ifndef STATE_H
#define STATE_H

#include <shared_config.h>

// ENUMS
/* FLAGS:
 * check if set - sound & SOUND_HORN
 * set - sound = SOUND_HORN | SOUND_LOOP
 * remove flag - sound &= ~SOUND_HORN
 * toggle flag - sound ^= SOUND_HORN
 * only flag - sound == SOUND_LOOP
 */
typedef enum {
	SOUND_OFF = 0,
	SOUND_HORN = 1 << 0, // no horn lamao
	SOUND_LOOP = 1 << 1,
	SOUND_WAIL = 1 << 2,
} sound_anim_t;

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

	struct {
		sound_anim_t anim;
		bool off;
	} sound;
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
