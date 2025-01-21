// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

#include "defines/config.h"

#if defined(DBG) && DBG
#define utils_printf(...) printf(__VA_ARGS__)
#else
#define utils_printf(...) (void)0
#endif

int32_t utils_time_diff_ms(u32 start_us, u32 end_us);

int32_t utils_time_diff_us(u32 start_us, u32 end_us);

#endif //UTILS_H
