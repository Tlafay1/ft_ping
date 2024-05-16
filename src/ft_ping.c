#include "ft_ping.h"

unsigned short icmp_cksum(unsigned char *addr, int len)
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
    gettimeofday(&ping->start_time, NULL);

    return ping;
}

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

int send_packet(PING *ping, t_ping_options ping_args)
{
    char packet[ping_args.size + sizeof(struct icmphdr)];
    struct icmphdr *icmphdr;
    struct timeval *tv;
    int len;
    int sent;

    icmphdr = (struct icmphdr *)packet;
    icmphdr->type = ICMP_ECHO;
    icmphdr->code = 0;
    icmphdr->checksum = 0;
    icmphdr->un.echo.id = getpid() & 0xFFFF;
    icmphdr->un.echo.sequence = 0;

    tv = (struct timeval *)(packet + sizeof(struct icmphdr));
    gettimeofday(tv, NULL);

    len = sizeof(packet);
    icmphdr->checksum = icmp_cksum((unsigned char *)icmphdr, len);

    printf("id sent: %d\n", icmphdr->un.echo.id);

    sent = sendto(ping->fd, packet, len, 0, (struct sockaddr *)&ping->dest, sizeof(ping->dest));
    if (sent < 0)
    {
        perror("sendto");
        return 1;
    }

    ping->count++;

    return 0;
}

int recv_packet(PING *ping)
{
    char packet[IP_MAXPACKET];
    struct sockaddr_in from;
    socklen_t fromlen;
    int len;
    int received;

    fromlen = sizeof(from);
    received = recvfrom(ping->fd, packet, IP_MAXPACKET, 0, (struct sockaddr *)&from, &fromlen);
    if (received < 0)
    {
        perror("recvfrom");
        return 1;
    }

    struct icmphdr *icmphdr;

    /* ICMP header */
    icmphdr = (struct icmphdr *)packet;

    uint16_t id = icmphdr->un.echo.id;

    printf("id: %d\n", id);

    printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.2f ms\n",
           received, ping->hostname, icmphdr->un.echo.sequence, 0, 0.0);

    return 0;
}

int ft_ping(const char *argv[])
{
    t_argr *argr;
    t_ping_options ping_options;
    t_args *args;
    PING *ping;

    if (parse_args(&argp, argv, &args))
        return 1;

    ping_options.verbose = false;
    ping_options.count = -1;
    ping_options.size = PING_DEFAULT_PKT_S;
    ping_options.interval = PING_DEFAULT_INTERVAL;

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

    gettimeofday(&last, NULL);
    send_packet(ping, ping_options);

    while (true || ping->options->count == -1)
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

        if (timeout.tv_sec < 0)
            timeout.tv_sec = timeout.tv_usec = 0;

        int result = select(ping->fd + 1, &fdset, NULL, NULL, &timeout);
        printf("result: %d\n", result);
        if (result < 0)
        {
            perror("select");
            free_args(args);
            return 1;
        }
        else if (result == 1)
            recv_packet(ping);
        else
            send_packet(ping, ping_options);

        if (ping->count == ping->options->count)
        {
            printf("--- %s ping statistics ---\n", ping->hostname);
            break;
        }
    }

    free_args(args);

    return 0;
}
