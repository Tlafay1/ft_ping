#include "ft_ping.h"

bool g_kill = false;

/**
 * Calculates the Internet Checksum (ICMP checksum) for the given data.
 *
 * @param addr The address of the data.
 * @param len The length of the data in bytes.
 * @return The calculated ICMP checksum.
 */
unsigned short
icmp_cksum(unsigned char *addr, int len)
{
    register int sum = 0;
    unsigned short answer = 0;
    unsigned short *wp;

    for (wp = (unsigned short *)addr; len > 1; wp++, len -= 2)
        sum += *wp;

    /* Take in an odd byte if present */
    if (len == 1)
    {
        *(unsigned char *)&answer = *(unsigned char *)wp;
        sum += answer;
    }

    sum = (sum >> 16) + (sum & 0xffff); /* add high 16 to low 16 */
    sum += (sum >> 16);                 /* add carry */
    answer = ~sum;                      /* truncate to 16 bits */
    return answer;
}

/**
 * Initializes a PING structure with the given program name, argument reader, and ping options.
 *
 * @param progname The name of the program.
 * @param argr The argument reader structure.
 * @param ping_options The ping options structure.
 * @return A pointer to the initialized PING structure, or NULL if an error occurred.
 */
PING *ping_init(const char *progname, t_argr *argr, t_ping_options *ping_options)
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
    struct icmp *icp;
    struct timeval now;
    int len;
    int sent;

    packet = malloc(sizeof(struct icmphdr) + ping_args.size + 8);

    icp = (struct icmp *)packet;
    icp->icmp_type = ICMP_ECHO;
    icp->icmp_code = 0;
    icp->icmp_id = htons(ping->ident);
    icp->icmp_seq = htons(ping->num_emit);
    icp->icmp_cksum = icmp_cksum((unsigned char *)icp, sizeof(packet));

    gettimeofday(&now, NULL);
    unsigned long v = htonl((now.tv_sec % 86400) * 1000 + now.tv_usec / 1000);
    icp->icmp_otime = v;
    icp->icmp_rtime = v;
    icp->icmp_ttime = v;

    sent = sendto(ping->fd, packet, sizeof(packet), 0, (struct sockaddr *)&ping->dest, sizeof(ping->dest));
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

/**
 * Receives an ICMP packet and processes its contents.
 *
 * @param ping The PING structure containing the necessary information.
 * @return Returns 0 on success, other on failure.
 */
int recv_packet(PING *ping)
{
    char packet[IP_MAXPACKET];
    struct sockaddr_in from;
    socklen_t fromlen;
    int received;
    size_t hlen;
    struct timeval now, sent, *tp;

    fromlen = sizeof(from);
    received = recvfrom(ping->fd, packet, IP_MAXPACKET, 0, (struct sockaddr *)&from, &fromlen);
    if (received < 0)
    {
        perror("recvfrom");
        return 1;
    }

    struct icmphdr *icmphdr;

    struct ip *ip_packet = (struct ip *)packet;
    hlen = ip_packet->ip_hl << 2;
    if (sizeof(packet) < hlen + ICMP_MINLEN)
        return -1;

    /* ICMP header */
    icmphdr = (struct icmphdr *)(packet + hlen);

    gettimeofday(&now, NULL);
    tp = (struct timeval *)(icmphdr);
    memcpy(&sent, tp, sizeof(sent));

    printf("now seconds: %ld, sent seconds: %ld\n", now.tv_sec, sent.tv_sec);
    printf("now useconds: %ld, sent useconds: %ld\n", now.tv_usec, sent.tv_usec);

    tvsub(&now, &sent);

    printf("%d bytes from %s: icmp_seq=%d ttl=%d, time=%.3f\n",
           received,
           inet_ntoa(*(struct in_addr *)&from.sin_addr.s_addr),
           ntohs(icmphdr->un.echo.sequence),
           ip_packet->ip_ttl,
           ((double)now.tv_sec) * 1000.0 + ((double)now.tv_usec) / 1000.0);

    ping->num_recv++;

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
    PING *ping;

    signal(SIGINT, sig_handler);

    if (parse_args(&argp, argv, &args))
        return 1;

    ping_options.verbose = false;
    ping_options.count = PING_DEFAULT_COUNT;
    ping_options.size = PING_DEFAULT_PKT_S;
    ping_options.interval = PING_DEFAULT_INTERVAL;
    ping_options.ttl = PING_DEFAULT_TTL;

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
    }

    argr = get_next_arg(args);

    if (!argr)
    {
        printf("%s: destination argument required\n", argv[0]);
        free_args(args);
        return 1;
    }

    ping = ping_init(argv[0], argr, &ping_options);
    set_dest(ping, argr->values[0]);

    printf("PING %s (%s) %ld bytes of data.\n",
           ping->hostname, inet_ntoa(ping->dest.sin_addr), ping->datalen);

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
            recv_packet(ping);
        else
        {
            send_packet(ping, ping_options);
        }

        if (ping->count == ping->options->count && ping->num_recv == ping->num_emit)
            break;
        gettimeofday(&last, NULL);
    }

    printf("--- %s ping statistics ---\n", ping->hostname);
    printf("%ld packets transmitted, %ld packets received, %d%% packet loss\n",
           ping->num_emit, ping->num_recv,
           (int)(100 - (float)ping->num_recv / (float)ping->num_emit * 100));

    close(ping->fd);
    free(ping->hostname);
    free(ping);

    free_args(args);

    return 0;
}
