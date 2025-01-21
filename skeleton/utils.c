// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#include "utils.h"

inline int32_t utils_time_diff_ms(const u32 start_us, const u32 end_us) {
	return (int32_t)(end_us - start_us) / 1000;
}

int32_t utils_time_diff_us(const u32 start_us, const u32 end_us) {
	return (int32_t)(end_us - start_us);
}
