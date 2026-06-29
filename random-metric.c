/* random-metric.c – Random metric generator module
 * Implements functions to generate random values for Claude Code statusline metrics.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "random-metric.h"

/* Internal RNG state */
static unsigned int seed = 0;

static void random_seed(void)
{
    if (seed == 0) {
        seed = (unsigned int)time(NULL);
    }
}

static double random_double(void)
{
    random_seed();
    seed = seed * 1103515245 + 12345;
    return (double)(seed / 65536) / 65536.0; /* 0.0 to 1.0 */
}

/* Available metrics that $random can pick from */
static const char *random_metrics[] = {
    "exceeds_200k_tokens",
    "context_window.remaining_percentage",
    "context_window.used_percentage",
    "cost.total_cost_usd",
    "cost.total_duration_ms",
    "cost.total_api_duration_ms",
    "cost.total_lines_added",
    "cost.total_lines_removed",
    "context_window.total_input_tokens",
    "context_window.total_output_tokens",
    "context_window.context_window_size",
    "rate_limits.five_hour.used_percentage",
    "rate_limits.seven_day.used_percentage",
    "rate_limits.five_hour.resets_at",
    "rate_limits.seven_day.resets_at",
    NULL
};

/* Track which metrics have been used in current render */
#define MAX_USED_METRICS 32
static char used_metrics[MAX_USED_METRICS][128];
static int used_count = 0;

static int is_metric_used(const char *metric)
{
    for (int i = 0; i < used_count; i++) {
        if (!strcmp(used_metrics[i], metric)) return 1;
    }
    return 0;
}

static void mark_metric_used(const char *metric)
{
    if (used_count < MAX_USED_METRICS) {
        strncpy(used_metrics[used_count], metric, 127);
        used_metrics[used_count][127] = '\0';
        used_count++;
    }
}

static const char *get_random_metric(void)
{
    /* Count available metrics */
    int count = 0;
    for (int i = 0; random_metrics[i]; i++) count++;

    /* Find available metrics (not yet used) */
    int available[MAX_USED_METRICS];
    int avail_count = 0;
    for (int i = 0; i < count; i++) {
        if (!is_metric_used(random_metrics[i])) {
            available[avail_count++] = i;
        }
    }

    /* If all metrics used, reset and pick again */
    if (avail_count == 0) {
        used_count = 0;
        for (int i = 0; i < MAX_USED_METRICS; i++) used_metrics[i][0] = '\0';
        for (int i = 0; i < count; i++) {
            if (!is_metric_used(random_metrics[i])) {
                available[avail_count++] = i;
            }
        }
    }

    /* Pick random available metric */
    int idx = available[(int)(random_double() * avail_count)];
    const char *metric = random_metrics[idx];
    mark_metric_used(metric);
    return metric;
}

static char *random_metric(cJSON *json, const char *metric) {
    static char buf[256];
    buf[0] = '\0';
    double v = 0;

    random_seed();

    /* Boolean metrics */
    if (!strcmp(metric, "exceeds_200k_tokens")) {
        cJSON *node = cJSON_GetObjectItemCaseSensitive(json, "exceeds_200k_tokens");
        if (!node || !cJSON_IsTrue(node)) return NULL;
        if (random_double() > 0.5) {
            snprintf(buf, sizeof buf, "⚠️ 200k");
            return buf;
        }
        return NULL;
    }

    /* Percentage metrics (0-100) */
    if (!strcmp(metric, "context_window.remaining_percentage")) {
        double pct = random_double() * 100.0;
        snprintf(buf, sizeof buf, "📊 %d%%", (int)pct);
        return buf;
    }
    if (!strcmp(metric, "context_window.used_percentage")) {
        double pct = random_double() * 100.0;
        snprintf(buf, sizeof buf, "📈 %d%%", (int)pct);
        return buf;
    }
    if (!strcmp(metric, "rate_limits.five_hour.used_percentage")) {
        double pct = random_double() * 100.0;
        snprintf(buf, sizeof buf, "⏱️ 5h: %d%%", (int)pct);
        return buf;
    }
    if (!strcmp(metric, "rate_limits.seven_day.used_percentage")) {
        double pct = random_double() * 100.0;
        snprintf(buf, sizeof buf, "📊 7d: %d%%", (int)pct);
        return buf;
    }

    /* Cost metric */
    if (!strcmp(metric, "cost.total_cost_usd")) {
        double cost = random_double() * 10.0; /* $0.00 to $10.00 */
        snprintf(buf, sizeof buf, "💰 $%.2f", cost);
        return buf;
    }

    /* Duration metric */
    if (!strcmp(metric, "cost.total_duration_ms")) {
        double dur = random_double() * 300000.0; /* 0 to 5 min */
        long mins = (long)(dur / 60000), secs = (long)(dur / 1000) % 60;
        if (mins > 0) snprintf(buf, sizeof buf, "⏱️ %ldm %lds", mins, secs);
        else          snprintf(buf, sizeof buf, "⏱️ %lds", secs);
        return buf;
    }

    /* Token count metrics */
    if (!strcmp(metric, "context_window.total_input_tokens")) {
        double tokens = random_double() * 200000.0;
        snprintf(buf, sizeof buf, "📥 %ld", (long)tokens);
        return buf;
    }
    if (!strcmp(metric, "context_window.total_output_tokens")) {
        double tokens = random_double() * 200000.0;
        snprintf(buf, sizeof buf, "📤 %ld", (long)tokens);
        return buf;
    }
    if (!strcmp(metric, "context_window.context_window_size")) {
        double tokens = random_double() * 200000.0;
        snprintf(buf, sizeof buf, "📏 %ld", (long)tokens);
        return buf;
    }

    /* Lines metrics */
    if (!strcmp(metric, "cost.total_lines_added")) {
        double lines = random_double() * 500.0;
        snprintf(buf, sizeof buf, "➕ +%ld", (long)lines);
        return buf;
    }
    if (!strcmp(metric, "cost.total_lines_removed")) {
        double lines = random_double() * 500.0;
        snprintf(buf, sizeof buf, "➖ -%ld", (long)lines);
        return buf;
    }

    /* Rate limit resets (random future time) */
    if (!strcmp(metric, "rate_limits.five_hour.resets_at")) {
        time_t now = time(NULL);
        time_t offset = (time_t)(random_double() * 86400); /* 0 to 24 hours */
        time_t reset = now + offset;
        struct tm *tm = localtime(&reset);
        strftime(buf, sizeof buf, "⏰ 5h: %m/%d %H:%M", tm);
        return buf;
    }
    if (!strcmp(metric, "rate_limits.seven_day.resets_at")) {
        time_t now = time(NULL);
        time_t offset = (time_t)(random_double() * 86400); /* 0 to 24 hours */
        time_t reset = now + offset;
        struct tm *tm = localtime(&reset);
        strftime(buf, sizeof buf, "📅 7d: %m/%d %H:%M", tm);
        return buf;
    }

    return NULL;
}

/* Public API implementations */

void random_metric_init(void)
{
    seed = 0; /* Force re-seed on next call */
}

char *random_metric_generate(cJSON *json, const char *metric)
{
    return random_metric(json, metric);
}

char *random_metric_random(cJSON *json)
{
    const char *metric = get_random_metric();
    if (metric) {
        return random_metric(json, metric);
    }
    return NULL;
}

void random_metric_reset(void)
{
    used_count = 0;
    for (int i = 0; i < MAX_USED_METRICS; i++) {
        used_metrics[i][0] = '\0';
    }
}
