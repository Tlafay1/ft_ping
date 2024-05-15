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
    ping->datalen = ping->options->size - sizeof(struct icmphdr);
    ping->ident = getpid() & 0xFFFF;
    ping->hostname = argr->values[0];
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

int send_packet(t_ping_options ping_args)
{
    // struct sockaddr_in dest_addr;

    // struct icmp_packet packet;
    // struct timeval tv_out;
    // struct timeval tv_in;
    // int sockfd;
    // int packet_size;
    // int bytes_sent;
    // int bytes_received;
    // int ttl;
    // int seq;
    // int pid;
    // int i;

    // ttl = 64;
    // seq = 0;
    // pid = getpid();

    // sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    // if (sockfd < 0)
    // {
    //     perror("socket");
    //     return 1;
    // }

    // if (setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0)
    // {
    //     perror("setsockopt");
    //     return 1;
    // }

    // dest_addr.sin_family = AF_INET;
    // dest_addr.sin_port = 0;
    // dest_addr.sin_addr.s_addr = inet_addr(ping_args.ip);

    // packet_size = sizeof(struct icmp_packet);
    // packet.icmp_type = ICMP_ECHO;
    // packet.icmp_code = 0;
    // packet.icmp_id = pid;
    // packet.icmp_seq = seq;
    // packet.icmp_checksum = 0;
    // packet.icmp_checksum = checksum((unsigned short *)&packet, packet_size);

    // bytes_sent = sendto(sockfd, &packet, packet_size, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    // if (bytes_sent < 0)
    // {
    //     perror("sendto");
    //     return 1;
    // }

    // tv_out.tv_usec = 0;

    // if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out)) < 0)
    // {
    //     perror("setsockopt");
    //     return 1;
    // }

    // bytes_received = recv(sockfd, &packet, packet_size, 0);
    // if (bytes_received < 0)
    // {
    //     perror("recv");
    //     return 1;
    // }

    // gettimeofday(&tv_in, NULL);

    // printf("%d bytes from %s (%s): icmp_seq=%d ttl=%d time=%.2f ms\n",
    //        bytes_received,
    //        ping_args.reverse_dns,
    //        ping_args.ip,
    //        seq,
    //        ttl,
    //        (double)(tv_in.tv_usec - tv_out.tv_usec) / 1000);

    // close(sockfd);

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
    }

    argr = get_next_arg(args);

    if (!argr)
    {
        printf("%s: destination argument required\n", argv[0]);
        free_args(args);
        return 1;
    }

    ping = ping_init(argv[0], argr, &ping_options);

    for (; ping->count < ping->options->count || ping->options->count == -1; ping->count++)
    {
        send_packet(ping_options);
        sleep(1);
    }

    free_args(args);

    return 0;
}
