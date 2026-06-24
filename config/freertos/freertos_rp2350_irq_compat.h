#pragma once

/*
 * Raspberry Pi's Pico FreeRTOS port is still kept under the RP2040 port path
 * and uses the RP2040 SIO IRQ names. RP2350 exposes the multicore FIFO as one
 * per-core NVIC IRQ number named SIO_IRQ_FIFO, so keep the port's
 * SIO_IRQ_PROC0 + get_core_num() expression building against SDK 2.x headers.
 */

#include "pico.h"

#if defined(PICO_RP2350) && PICO_RP2350
#include "hardware/irq.h"
#include "pico/multicore.h"

#if !defined(SIO_IRQ_PROC0)
#define SIO_IRQ_PROC0 (SIO_IRQ_FIFO - get_core_num())
#endif

#if !defined(SIO_IRQ_PROC1)
#define SIO_IRQ_PROC1 SIO_IRQ_FIFO
#endif
#endif
