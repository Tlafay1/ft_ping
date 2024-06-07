#include "ft_ping.h"

void calculate_stats(t_ping_stats *stats, struct timeval *sent)
{
    double timediff = sent->tv_sec * 1000.0 + sent->tv_usec / 1000.0;
    if (stats->min == -1)
        stats->min = timediff;
    else
        stats->min = stats->min < timediff ? stats->min : timediff;
    if (stats->max == -1)
        stats->max = timediff;
    else
        stats->max = stats->max > timediff ? stats->max : timediff;
    if (stats->sum == -1)
        stats->sum = timediff;
    else
        stats->sum += timediff;
    if (stats->sum_square == -1)
        stats->sum_square = timediff * timediff;
    else
        stats->sum_square += timediff * timediff;
}