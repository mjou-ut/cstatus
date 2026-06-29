/* random-metric.h – Random metric generator module
 * Provides functions to generate random values for Claude Code statusline metrics.
 */

#ifndef RANDOM_METRIC_H
#define RANDOM_METRIC_H

#include <cjson/cJSON.h>

/* Initialize the random number generator with current time */
void random_metric_init(void);

/* Generate a random value for a specific metric
 * Returns a static string with the formatted value (e.g., "💰 $2.90")
 * Returns NULL if the metric cannot be generated
 */
char *random_metric_generate(cJSON *json, const char *metric);

/* Generate a random value for a randomly selected metric
 * Picks a metric not yet used in this render cycle
 * Returns a static string with the formatted value
 * Returns NULL if no metrics are available
 */
char *random_metric_random(cJSON *json);

/* Reset the used metrics tracker (call at start of each template render) */
void random_metric_reset(void);

#endif /* RANDOM_METRIC_H */
