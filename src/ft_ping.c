#include "ft_ping.h"

bool g_kill = false;

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

/**
 * Initializes a PING structure with the given program name, argument reader, and ping options.
 *
 * @param progname The name of the program.
 * @param argr The argument reader structure.
 * @param ping_options The ping options structure.
 * @return A pointer to the initialized PING structure, or NULL if an error occurred.
 */
PING *ping_init(const char *progname, t_ping_options *ping_options, t_ping_stats *stats)
{
    int fd;
    struct protoent *proto;
    PING *ping;

    proto = getprotobyname("icmp");
    if (!proto)
    {
        fprintf(stderr, "%s: unknown protocol icmp.\n", progname);
        return NULL;
    }

    fd = socket(AF_INET, SOCK_RAW, proto->p_proto);
    if (fd < 0)
    {
        if (errno == EPERM || errno == EACCES)
        {
            errno = 0;
            fd = socket(AF_INET, SOCK_DGRAM, proto->p_proto);
            if (fd < 0)
            {
                if (errno == EPERM || errno == EACCES || errno == EPROTONOSUPPORT)
                    printf("%s: Lacking privilege for icmp socket.\n", progname);
                else
                    printf("%s: %s\n", progname, strerror(errno));

                return NULL;
            }
        }
        else
            return NULL;
    }

    ping = malloc(sizeof(*ping));
    if (!ping)
    {
        close(fd);
        return ping;
    }

    memset(ping, 0, sizeof(*ping));

    ping->options = ping_options;

    ping->fd = fd;
    ping->count = 0;
    ping->interval = 1;
    ping->datalen = ping->options->size;
    ping->ident = getpid() & 0xFFFF;
    ping->num_emit = 0;
    ping->num_recv = 0;
    ping->num_rept = 0;
    gettimeofday(&ping->start_time, NULL);

    stats->sum = -1;
    stats->min = -1;
    stats->max = -1;

    if (ping_options->ttl > 0)
        if (setsockopt(ping->fd, IPPROTO_IP, IP_TTL,
                       &ping_options->ttl, sizeof(ping_options->ttl)) < 0)
        {
            perror("setsockopt");
            free(ping);
            return NULL;
        }

    return ping;
}

/**
 * Sets the destination address for the PING structure.
 *
 * @param ping The PING structure to set the destination for.
 * @param host The hostname or IP address of the destination.
 * @return Returns 0 on success, or 1 if an error occurred.
 */
int set_dest(PING *ping, const char *host)
{
    struct addrinfo hints;
    struct addrinfo *res;
    struct sockaddr_in *ipv4;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, NULL, &hints, &res) != 0)
        return 1;

    ipv4 = (struct sockaddr_in *)res->ai_addr;
    ping->dest = *ipv4;

    ping->hostname = strdup(host);

    freeaddrinfo(res);

    return 0;
}

/**
 * Sends an ICMP packet to the destination address.
 *
 * @param ping The PING structure containing the socket file descriptor and destination address.
 * @param ping_args The ping options specifying the packet size.
 * @return 0 if the packet is sent successfully, other otherwise.
 */
int send_packet(PING *ping, t_ping_options ping_args)
{
    char *packet;
    struct icmphdr *icp;
    struct timeval now;
    size_t len;
    int sent;

    len = sizeof(struct icmphdr) + ping_args.size;

    packet = malloc(len);

    icp = (struct icmphdr *)packet;
    icp->type = ICMP_ECHO;
    icp->code = 0;
    icp->un.echo.id = htons(ping->ident);
    icp->un.echo.sequence = htons(ping->num_emit);
    icp->checksum = icmp_cksum((uint16_t *)icp, len);

    if (len >= sizeof(struct icmphdr) + sizeof(struct timeval))
    {
        gettimeofday(&now, NULL);
        memcpy(packet + sizeof(struct icmphdr), &now, sizeof(now));
    }

    sent = sendto(ping->fd, packet, len, 0, (struct sockaddr *)&ping->dest, sizeof(ping->dest));
    if (sent < 0)
    {
        perror("sendto");
        return 1;
    }

    ping->count++;
    ping->num_emit++;

    free(packet);

    return 0;
}

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

void ping_print(uint8_t type, uint hlen, ssize_t received, char *from, uint seq, uint ttl, struct timeval *now)
{
    char message[40];
    char time[20];

    switch (type)
    {
    case ICMP_ECHO:
    case ICMP_ECHOREPLY:
        if (received >= (ssize_t)(hlen + sizeof(struct icmphdr) + sizeof(struct timeval)))
            snprintf(time, 20, " time=%.3f ms", ((double)now->tv_sec) * 1000.0 + ((double)now->tv_usec) / 1000.0);
        else
            snprintf(time, 1, "");
        snprintf(message, 40, "icmp_seq=%d ttl=%d%s",
                 seq,
                 ttl,
                 time);
        break;
    case ICMP_DEST_UNREACH:
        snprintf(message, 30, "Destination Host Unreachable");
        break;
    case ICMP_TIME_EXCEEDED:
        snprintf(message, 23, "Time to live exceeded");
        break;
    default:
        snprintf(message, 22, "Unknown ICMP type %d", type);
        break;
    }
    printf("%ld bytes from %s: %s",
           received,
           from,
           message);

    printf("\n");
}

/**
 * Receives an ICMP packet and processes its contents.
 *
 * @param ping The PING structure containing the necessary information.
 * @return Returns 0 on success, other on failure.
 */
int recv_packet(PING *ping, t_ping_stats *stats, t_ping_options ping_options)
{
    char packet[IP_MAXPACKET];
    struct sockaddr_in from;
    socklen_t fromlen;
    ssize_t received;
    uint hlen;
    struct timeval now, sent, *tp;
    struct icmphdr *icp;

    fromlen = sizeof(from);
    received = recvfrom(ping->fd, packet, IP_MAXPACKET, 0, (struct sockaddr *)&from, &fromlen);
    if (received < 0)
    {
        perror("recvfrom");
        return 1;
    }

    struct ip *ip_packet = (struct ip *)packet;
    hlen = ip_packet->ip_hl << 2;

    if (received < hlen + ICMP_MINLEN)
        return -1;

    icp = (struct icmphdr *)(packet + hlen);

    gettimeofday(&now, NULL);
    tp = (struct timeval *)(icp + 1);
    memcpy(&sent, tp, sizeof(sent));
    tvsub(&now, &sent);

    if (!ping_options.quiet)
        ping_print(
            icp->type,
            hlen,
            received - hlen,
            inet_ntoa(*(struct in_addr *)&from.sin_addr.s_addr),
            ntohs(icp->un.echo.sequence),
            ip_packet->ip_ttl,
            &now);

    ping->num_recv++;

    calculate_stats(stats, &now);

    return 0;
}

void sig_handler(__attribute__((__unused__)) int signo)
{
    g_kill = true;
}

int ft_ping(const char *argv[])
{
    t_argr *argr;
    t_ping_options ping_options;
    t_args *args;
    t_ping_stats stats;
    PING *ping;

    signal(SIGINT, sig_handler);

    if (parse_args(&argp, argv, &args))
        return 1;

    ping_options.verbose = false;
    ping_options.count = PING_DEFAULT_COUNT;
    ping_options.size = PING_DEFAULT_PKT_S;
    ping_options.interval = PING_DEFAULT_INTERVAL;
    ping_options.ttl = PING_DEFAULT_TTL;
    ping_options.quiet = false;

    while ((argr = get_next_option(args)))
    {
        if (argr->option && argr->option->sflag == '?')
        {
            help_args(&argp, argv[0]);
            free_args(args);
            return 1;
        }
        if (argr->option && argr->option->sflag == 'v')
            ping_options.verbose = true;
        if (argr->option && argr->option->sflag == 'c')
        {
            if (parse_count_arg(&ping_options, argr, argv[0]))
            {
                free_args(args);
                return 1;
            }
        }
        if (argr->option && argr->option->sflag == 's')
        {
            if (parse_size_arg(&ping_options, argr, argv[0]))
            {
                free_args(args);
                return 1;
            }
        }
        if (argr->option && argr->option->sflag == 'i')
        {
            if (parse_interval_arg(&ping_options, argr, argv[0]))
            {
                free_args(args);
                return 1;
            }
        }
        if (argr->option && argr->option->sflag == 't')
        {
            if (parse_ttl_arg(&ping_options, argr, argv[0]))
            {
                free_args(args);
                return 1;
            }
        }
        if (argr->option && argr->option->sflag == 'q')
            ping_options.quiet = true;
    }

    argr = get_next_arg(args);

    if (!argr)
    {
        printf("%s: destination argument required\n", argv[0]);
        free_args(args);
        return 1;
    }

    ping = ping_init(argv[0], &ping_options, &stats);
    set_dest(ping, argr->values[0]);

    printf("PING %s (%s): %ld bytes of data",
           ping->hostname, inet_ntoa(ping->dest.sin_addr), ping->datalen);

    if (ping->options->verbose)
        printf(", id 0x%x = %d", ping->ident, ping->ident);
    printf("\n");

    fd_set fdset;
    struct timeval timeout, last, interval, now;

    memset(&timeout, 0, sizeof(timeout));
    memset(&interval, 0, sizeof(interval));
    memset(&now, 0, sizeof(now));

    interval.tv_sec = 1;
    interval.tv_usec = 0;

    gettimeofday(&last, NULL);
    send_packet(ping, ping_options);

    while (!g_kill || ping->options->count == 0)
    {
        FD_ZERO(&fdset);
        FD_SET(ping->fd, &fdset);

        gettimeofday(&now, NULL);
        timeout.tv_sec = last.tv_sec + interval.tv_sec - now.tv_sec;
        timeout.tv_usec = last.tv_usec + interval.tv_usec - now.tv_usec;

        while (timeout.tv_usec < 0)
        {
            timeout.tv_usec += 1000000;
            timeout.tv_sec--;
        }
        while (timeout.tv_usec >= 1000000)
        {
            timeout.tv_usec -= 1000000;
            timeout.tv_sec++;
        }

        if (timeout.tv_sec < 0)
            timeout.tv_sec = timeout.tv_usec = 0;

        int result = select(ping->fd + 1, &fdset, NULL, NULL, &timeout);
        if (result < 0 && errno != EINTR)
        {
            perror("select");
            free_args(args);
            return 1;
        }
        else if (result == 1)
        {
            recv_packet(ping, &stats, ping_options);
        }
        else if (ping->num_emit < ping->options->count && !g_kill)
        {
            send_packet(ping, ping_options);
        }

        if (ping->count == ping->options->count && ping->num_recv == ping->num_emit)
            break;
        gettimeofday(&last, NULL);
    }

    int packet_loss = (int)(100.0 - (float)ping->num_recv / (float)ping->num_emit * 100.0);
    printf("--- %s ping statistics ---\n", ping->hostname);
    printf("%ld packets transmitted, %ld packets received, %d%% packet loss\n",
           ping->num_emit, ping->num_recv,
           packet_loss);
    if (ping->num_recv > 0)
        printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n",
               stats.min,
               stats.sum / ping->num_recv,
               stats.max,
               stats.sum / ping->num_recv);

    close(ping->fd);
    free(ping->hostname);
    free(ping);

    free_args(args);

    return 0;
}
