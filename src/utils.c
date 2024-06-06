#include "ft_ping.h"

/**
 * Parses the count argument and stores it in the ping_args structure.
 *
 * @param ping_args - Pointer to the t_ping_args structure to store the parsed count.
 * @param argr - Pointer to the t_argr structure containing the count argument value.
 * @param progname - The name of the program.
 * @return 0 if the count argument is successfully parsed and stored, other otherwise.
 */
int parse_count_arg(t_ping_options *ping_args, t_argr *argr, const char *progname)
{
    char *p;
    ping_args->count = strtol(argr->values[0], &p, 10);
    if (errno == ERANGE)
    {
        printf("%s: invalid argument: '%s': %s\n",
               progname, argr->values[0], strerror(errno));
        return 1;
    }
    if (*p)
    {
        printf("%s: invalid count: '%s'\n", progname, argr->values[0]);
        return 1;
    }
    if (ping_args->count < 1)
    {
        printf("%s: invalid argument: '%s': out of range: 1 <= value <= %lld\n",
               progname, argr->values[0], LLONG_MAX);
        return 1;
    }
    return 0;
}

int parse_size_arg(t_ping_options *ping_args, t_argr *argr, const char *progname)
{
    char *p;
    ping_args->size = strtoul(argr->values[0], &p, 10);
    if (*p)
    {
        printf("%s: invalid size: '%s'\n", progname, argr->values[0]);
        return 1;
    }
    if (ping_args->size > PING_MAX_DATALEN)
    {
        printf("%s: option value too big: %s\n",
               progname, argr->values[0]);
        return 1;
    }
    return 0;
}

int parse_interval_arg(t_ping_options *ping_args, t_argr *argr, const char *progname)
{
    char *p;
    ping_args->interval = strtof(argr->values[0], &p) * 1000000;
    if (*p)
    {
        printf("%s: invalid interval: '%s'\n", progname, argr->values[0]);
        return 1;
    }
    if (ping_args->interval < 0.2)
    {
        printf("%s: invalid argument: '%s': out of range: 0 <= value <= 9223372036854775807\n",
               progname, argr->values[0]);
        return 1;
    }
    return 0;
}

int parse_ttl_arg(t_ping_options *ping_args, t_argr *argr, const char *progname)
{
    char *p;
    ping_args->ttl = strtol(argr->values[0], &p, 10);
    if (errno == ERANGE)
    {
        printf("%s: invalid argument: '%s': %s\n",
               progname, argr->values[0], strerror(errno));
        return 1;
    }
    if (*p)
    {
        printf("%s: invalid ttl: '%s'\n", progname, argr->values[0]);
        return 1;
    }
    if (ping_args->ttl < 1 || ping_args->ttl > 255)
    {
        printf("%s: invalid argument: '%s': out of range: 1 <= value <= 255\n",
               progname, argr->values[0]);
        return 1;
    }
    return 0;
}

/**
 * @brief Subtract two timeval structs.
 *
 * @param out The result of the subtraction.
 * @param in The timeval struct to subtract from out.
 */
void tvsub(struct timeval *out, struct timeval *in)
{
    if ((out->tv_usec -= in->tv_usec) < 0)
    {
        --out->tv_sec;
        out->tv_usec += 1000000;
    }
    out->tv_sec -= in->tv_sec;
}

void calculate_timeout(struct timeval *timeout, struct timeval *last, struct timeval *interval)
{
    struct timeval now;

    gettimeofday(&now, NULL);
    timeout->tv_sec = last->tv_sec + interval->tv_sec - now.tv_sec;
    timeout->tv_usec = last->tv_usec + interval->tv_usec - now.tv_usec;

    while (timeout->tv_usec < 0)
    {
        timeout->tv_usec += 1000000;
        timeout->tv_sec--;
    }
    while (timeout->tv_usec >= 1000000)
    {
        timeout->tv_usec -= 1000000;
        timeout->tv_sec++;
    }

    if (timeout->tv_sec < 0)
        timeout->tv_sec = timeout->tv_usec = 0;
}

/**
 * Calculates the Internet Checksum (ICMP checksum) for the given data.
 *
 * @param addr The address of the data.
 * @param len The length of the data in bytes.
 * @return The calculated ICMP checksum.
 */
uint16_t icmp_cksum(uint16_t *icmph, int len)
{
    uint16_t ret = 0;
    uint32_t sum = 0;
    uint16_t odd_byte;

    while (len > 1)
    {
        sum += *icmph++;
        len -= 2;
    }

    if (len == 1)
    {
        *(uint8_t *)(&odd_byte) = *(uint8_t *)icmph;
        sum += odd_byte;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    ret = ~sum;

    return ret;
}