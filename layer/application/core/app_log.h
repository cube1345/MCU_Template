/* SPDX-License-Identifier: MIT */
#ifndef APP_LOG_H
#define APP_LOG_H

#include <stdio.h>

/* Master switch for UART debug and diagnostic printf output. */
#ifndef APP_LOG_ENABLE
#define APP_LOG_ENABLE 0
#endif

#if ((APP_LOG_ENABLE != 0) && (APP_LOG_ENABLE != 1))
#error "APP_LOG_ENABLE must be 0 or 1"
#endif

/* The on-demand CPU/stack/heap monitor is a diagnostic-only task. */
#ifndef APP_SYSTEM_STATS_ENABLE
#define APP_SYSTEM_STATS_ENABLE APP_LOG_ENABLE
#endif

#if ((APP_SYSTEM_STATS_ENABLE != 0) && \
     (APP_SYSTEM_STATS_ENABLE != 1))
#error "APP_SYSTEM_STATS_ENABLE must be 0 or 1"
#endif

#if (APP_LOG_ENABLE == 1)
#define APP_LOG_PRINTF(...) printf(__VA_ARGS__)
#else
/* Keep arguments type-checked and referenced without evaluating them. */
#define APP_LOG_PRINTF(...) ((void)sizeof(printf(__VA_ARGS__)))
#define printf(...) APP_LOG_PRINTF(__VA_ARGS__)
#endif

#endif /* APP_LOG_H */
