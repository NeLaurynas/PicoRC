// Copyright (C) 2025 Laurynas 'Deviltry' Ekekeke
// SPDX-License-Identifier: BSD-3-Clause

#ifndef SOUND_H
#define SOUND_H

#define DMA_IRQ(irq) (irq == 0 ? DMA_IRQ_0 : DMA_IRQ_1)

void sound_init();

void sound_animation();

#endif //SOUND_H
