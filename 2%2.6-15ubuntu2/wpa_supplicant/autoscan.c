/*
 * WPA Supplicant - auto scan
 * Copyright (c) 2012, Intel Corporation. All rights reserved.
 * Copyright 2015	Intel Deutschland GmbH
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"

#include "common.h"
#include "config.h"
#include "wpa_supplicant_i.h"
#include "bss.h"
#include "scan.h"
#include "autoscan.h"
#include "wpa_supplicant_print.h"

static const struct autoscan_ops *autoscan_modules[] = {
#ifdef CONFIG_AUTOSCAN_EXPONENTIAL
	&autoscan_exponential_ops,
#endif /* CONFIG_AUTOSCAN_EXPONENTIAL */
#ifdef CONFIG_AUTOSCAN_PERIODIC
	&autoscan_periodic_ops,
#endif /* CONFIG_AUTOSCAN_PERIODIC */
	NULL};

static void request_scan(struct wpa_supplicant *wpa_s)
{
	int ret;

	wpa_s->scan_req = MANUAL_SCAN_REQ;

	ret = wpa_supplicant_req_sched_scan(wpa_s);
	wpa_printf(MSG_DEBUG, "[autoscan][request_scan] wpa_supplicant_req_sched_scan returned: ret=%d", ret);
	if (ret)
	{
		wpa_printf(MSG_DEBUG, "[autoscan][request_scan] wpa_supplicant_req_sched_scan failed! Calling wpa_supplicant_req_scan");
		wpa_supplicant_req_scan(wpa_s, wpa_s->scan_interval, 0);
	}
	else
	{
		wpa_printf(MSG_DEBUG, "[autoscan][request_scan] wpa_supplicant_req_sched_scan succeeded!");
	}
}

int autoscan_init(struct wpa_supplicant *wpa_s, int req_scan)
{
	const char *name = wpa_s->conf->autoscan;
	const char *params;
	size_t nlen;
	int i;
	const struct autoscan_ops *ops = NULL;
	struct sched_scan_plan *scan_plans;

	/* Give preference to scheduled scan plans if supported/configured */
	if (wpa_s->sched_scan_plans)
	{
		wpa_printf(MSG_INFO, "[autoscan][autoscan_init] does nothing because scheduled scan plans is configured.");
		return 0;
	}
	else
	{
		wpa_printf(MSG_INFO, "[autoscan][autoscan_init] scheduled scan plans is NOT configured.");
	}

	if (wpa_s->autoscan && wpa_s->autoscan_priv)
		return 0;

	if (name == NULL)
		return 0;

	params = os_strchr(name, ':');
	if (params == NULL)
	{
		params = "";
		nlen = os_strlen(name);
	}
	else
	{
		nlen = params - name;
		params++;
	}

	for (i = 0; autoscan_modules[i]; i++)
	{
		if (os_strncmp(name, autoscan_modules[i]->name, nlen) == 0)
		{
			ops = autoscan_modules[i];
			break;
		}
	}

	if (ops == NULL)
	{
		wpa_printf(MSG_ERROR, "[autoscan][autoscan_init]: Could not find module "
							  "matching the parameter '%s'",
				   name);
		return -1;
	}

	size_t size_of_sched_scan_plans = sizeof(*wpa_s->sched_scan_plans);

	// NOTE(ywen): They are both of 8 bytes and look like the size of one plan.
	wpa_printf(MSG_DEBUG, "[autoscan][autoscan_init] size of wpa_s->sched_scan_plans: %lu", size_of_sched_scan_plans);
	wpa_printf(MSG_DEBUG, "[autoscan][autoscan_init] size of struct sched_scan_plan: %lu", sizeof(struct sched_scan_plan));

	scan_plans = os_malloc(size_of_sched_scan_plans);
	if (!scan_plans)
		return -1;

	wpa_s->autoscan_params = NULL;

	wpa_s->autoscan_priv = ops->init(wpa_s, params);
	if (!wpa_s->autoscan_priv)
	{
		os_free(scan_plans);
		return -1;
	}

	// NOTE(ywen): Debug: Verify that in the case of periodic autoscan, the returned object is
	// a struct autoscan_periodic_data.
	{
		// NOTE(ywen): Interesting... struct autoscan_periodic_data is only visible in
		// autoscan_periodic.c. So how is wpa_s->autoscan_priv used?
		struct autoscan_periodic_data
		{
			int periodic_interval;
		} *data = (struct autoscan_periodic_data *)(wpa_s->autoscan_priv);
		wpa_printf(MSG_DEBUG, "[autoscan][autoscan_init] periodic autoscan is done. Interval: %d", data->periodic_interval);
	}

	// NOTE(ywen): The plan is hard-coded?
	scan_plans[0].interval = 5;
	scan_plans[0].iterations = 0;
	os_free(wpa_s->sched_scan_plans);
	wpa_s->sched_scan_plans = scan_plans;
	wpa_s->sched_scan_plans_num = 1;
	wpa_s->autoscan = ops;

	wpa_printf(MSG_DEBUG, "[autoscan][autoscan_init]: Initialized module '%s' with "
						  "parameters '%s'",
			   ops->name, params);
	if (!req_scan)
		return 0;

	/*
	 * Cancelling existing scan requests, if any.
	 */
	wpa_supplicant_cancel_sched_scan(wpa_s);
	wpa_supplicant_cancel_scan(wpa_s);

	/*
	 * Firing first scan, which will lead to call autoscan_notify_scan.
	 */
	request_scan(wpa_s);

	wpa_supplicant_print(wpa_s);

	return 0;
}

void autoscan_deinit(struct wpa_supplicant *wpa_s)
{
	if (wpa_s->autoscan && wpa_s->autoscan_priv)
	{
		wpa_printf(
			MSG_DEBUG,
			"[autoscan][autoscan_deinit]: Deinitializing module '%s'",
			wpa_s->autoscan->name);
		wpa_s->autoscan->deinit(wpa_s->autoscan_priv);
		wpa_s->autoscan = NULL;
		wpa_s->autoscan_priv = NULL;

		wpa_s->scan_interval = 5;

		os_free(wpa_s->sched_scan_plans);
		wpa_s->sched_scan_plans = NULL;
		wpa_s->sched_scan_plans_num = 0;
	}
}

int autoscan_notify_scan(struct wpa_supplicant *wpa_s,
						 struct wpa_scan_results *scan_res)
{
	int interval;

	wpa_printf(
		MSG_DEBUG,
		"[autoscan][autoscan_notify_scan]: '%p', %p, %p",
		wpa_s->autoscan,
		(void *)wpa_s->autoscan,
		(void *)wpa_s->autoscan_priv);

	if (wpa_s->autoscan && wpa_s->autoscan_priv)
	{
		// NOTE(ywen): So what? This function has never been called?
		wpa_printf(MSG_DEBUG, "[autoscan][autoscan_notify_scan]: '%s'", wpa_s->autoscan->name);

		interval = wpa_s->autoscan->notify_scan(wpa_s->autoscan_priv,
												scan_res);

		if (interval <= 0)
		{
			wpa_printf(
				MSG_DEBUG,
				"[autoscan][autoscan_notify_scan]: interval (%d) < 0. Return -1",
				interval);
			return -1;
		}

		wpa_s->scan_interval = interval;
		wpa_s->sched_scan_plans[0].interval = interval;

		wpa_supplicant_print(wpa_s);

		request_scan(wpa_s);
	}

	return 0;
}
