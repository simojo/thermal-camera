/*
 * wifi_scan.h
 *
 * @copyright Copyright (C) 2025 Simon J. Jones <github@simonjjones.com>
 * Licensed under the Apache License, Version 2.0.
 */

#ifndef __WIFI_SCAN_H
#define __WIFI_SCAN_H

int scan_result(void *env, const cyw43_ev_scan_result_t *result);

/*
 * scan_worker_fn
 *
 * @brief Start a wifi scan
 */
void scan_worker_fn(async_context_t *context, async_at_time_worker_t *worker);

#endif
