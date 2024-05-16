#include "ft_ping.h"

/**
 * @brief The IP address of a given hostname.
 *
 * @param host The hostname to resolve. Examples: "localhost", "127.0.0.1"
 * @return The IP address as a string, or NULL if the resolution fails.
 * @note The returned string must be freed by the caller.
 */
char *dns_lookup(const char *host)
{
    struct addrinfo hints;
    struct addrinfo *res;
    char *ip = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, NULL, &hints, &res) != 0)
        return NULL;

    struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
    ip = ft_strdup(inet_ntoa(ipv4->sin_addr));
    freeaddrinfo(res);
    return ip;
}

/**
 * @brief The hostname of a given IP address.
 * @param ip The IP address to resolve.
 * @return The hostname as a string, or NULL if the resolution fails.
 * @note The returned string must be freed by the caller.
 */
char *reverse_dns_lookup(const char *ip)
{
    struct sockaddr_in sa;
    char host[1024];
    char service[20];

    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr(ip);
    getnameinfo((struct sockaddr *)&sa, sizeof(sa), host, 1024, service, 20, 0);
    return ft_strdup(host);
}

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
    ping_args->size = strtol(argr->values[0], &p, 10);
    if (*p)
    {
        printf("%s: invalid size: '%s'\n", progname, argr->values[0]);
        return 1;
    }
    if (ping_args->size < 1 || ping_args->size > IP_MAXPACKET)
    {
        printf("%s: invalid argument: '%s': out of range: 1 <= value <= %d\n",
               progname, argr->values[0], IP_MAXPACKET);
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