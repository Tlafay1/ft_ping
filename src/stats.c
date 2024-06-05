#include "ft_ping.h"

void calculate_stats(t_ping_stats *stats, struct timeval *sent)
{
    if (stats->min == -1)
        stats->min = sent->tv_sec * 1000.0 + sent->tv_usec / 1000.0;
    else
        stats->min = stats->min < sent->tv_sec * 1000.0 + sent->tv_usec / 1000.0 ? stats->min : sent->tv_sec * 1000.0 + sent->tv_usec / 1000.0;
    if (stats->max == -1)
        stats->max = sent->tv_sec * 1000.0 + sent->tv_usec / 1000.0;
    else
        stats->max = stats->max > sent->tv_sec * 1000.0 + sent->tv_usec / 1000.0 ? stats->max : sent->tv_sec * 1000.0 + sent->tv_usec / 1000.0;
    if (stats->sum == -1)
        stats->sum = sent->tv_sec * 1000.0 + sent->tv_usec / 1000.0;
    else
        stats->sum += sent->tv_sec * 1000.0 + sent->tv_usec / 1000.0;
}