// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#ifndef RENDERER_H
#define RENDERER_H
#include <controller/uni_gamepad.h>

void renderer_set_state(const uni_gamepad_t *gamepad);
void renderer_loop();

#endif //RENDERER_H
