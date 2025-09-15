// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#ifndef HUMMER_APP_SETTINGS_T_H
#define HUMMER_APP_SETTINGS_T_H

#include "shared_config.h"
#include "shared_modules/storage/storage.h"

/* When updating fields - there might be old structs in storage, so probably:
 * create v2
 * load v1, set v2 as much as possible
 * erase storage
 * save v2
 */

typedef struct {
	u16 boot_count;
	char device_name[32];
	char app_name[32];
} app_settings_t;

static_assert(sizeof(app_settings_t) <= MOD_STORAGE_PAYLOAD_BYTES, "app_settings_t should not exceed MOD_STORAGE_PAYLOAD_BYTES");

app_settings_t app_settings = { 0 };

#endif //HUMMER_APP_SETTINGS_T_H
