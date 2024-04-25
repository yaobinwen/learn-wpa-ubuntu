#include "includes.h"
#include "common.h"
#include "config.h"
#include "wpa_supplicant_i.h"

#include "wpa_supplicant_print.h"

void wpa_supplicant_print(const struct wpa_supplicant *wpa_s)
{
    char buf_sched_scan_plans[128] = {0};

    // TODO(ywen): Print scheduled plan
    const struct sched_scan_plan *_plans = wpa_s->sched_scan_plans;
    for (int i = 0; i < wpa_s->sched_scan_plans_num; ++i)
    {
        size_t len = strlen(buf_sched_scan_plans);
        snprintf(buf_sched_scan_plans + len, 128 - len, "(%u, %u)", _plans[i].interval, _plans[i].iterations);
    }

    wpa_printf(MSG_DEBUG, "wpa_supplicant state:");
    wpa_printf(MSG_DEBUG, "\t ifname: %s", wpa_s->ifname);
    wpa_printf(MSG_DEBUG, "\t sched_scan_plans: %s", buf_sched_scan_plans);
    wpa_printf(MSG_DEBUG, "\t sched_scan_plans_num: %lu", wpa_s->sched_scan_plans_num);
}
