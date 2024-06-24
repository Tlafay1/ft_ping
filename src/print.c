#include "ft_ping.h"

void print_header(PING *ping)
{
    printf("PING %s (%s): %ld bytes of data",
           ping->hostname, inet_ntoa(ping->dest.sin_addr), ping->datalen);

    if (ping->options.verbose)
        printf(", id 0x%x = %d", ping->ident, ping->ident);
    printf("\n");
}

void print_stats(PING *ping)
{
    int packet_loss = (int)(100.0 - (float)ping->num_recv / (float)ping->num_emit * 100.0);
    printf("--- %s ping statistics ---\n", ping->hostname);
    printf("%ld packets transmitted, %ld packets received, %d%% packet loss\n",
           ping->num_emit, ping->num_recv,
           packet_loss);

    double avg = ping->stats.sum / ping->num_recv;
    double vari = ping->stats.sum_square / ping->num_recv - avg * avg;
    if (ping->num_recv > 0 && ping->stats.sum > 0)
        printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n",
               ping->stats.min,
               ping->stats.sum / ping->num_recv,
               ping->stats.max,
               nsqrt(vari, 0.0005));
}

void print_error_dump(struct ip *ip, struct icmphdr *icmp_packet)
{
    int hlen = ip->ip_hl << 2;
    unsigned char *cp = (unsigned char *)ip + hlen;

    printf("IP Hdr Dump:\n");
    for (u_int64_t i = 0; i < sizeof(*ip); i++)
    {
        printf("%02x%s", *((u_char *)ip + i),
               (i % 2) ? " " : "");
    }
    printf("\n");
    printf("Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src      Dst     Data\n");
    printf(" %1x  %1x %02x %04x %04x",
           ip->ip_v,
           ip->ip_hl,
           ip->ip_tos,
           ntohs(ip->ip_len),
           ntohs(ip->ip_id));
    printf("   %1x %04x",
           (ntohs(ip->ip_off) & 0xe000) >> 13,
           ntohs(ip->ip_off) & 0x1fff);
    printf("  %02x  %02x",
           ip->ip_ttl,
           ip->ip_p);
    printf(" %04x",
           ntohs(ip->ip_sum));
    printf(" %s",
           inet_ntoa(ip->ip_src));
    printf(" %s",
           inet_ntoa(ip->ip_dst));
    printf("\n");
    if (ip->ip_p == IPPROTO_ICMP)
    {
        icmp_packet = (struct icmphdr *)((u_char *)ip + (ip->ip_hl << 2));
        printf("ICMP: type %u, code %u, size %u, id 0x%x, seq %d\n", *cp, *(cp + 1), ntohs(ip->ip_len) - hlen, ntohs(icmp_packet->un.echo.id), ntohs(icmp_packet->un.echo.sequence));
        if (*cp == ICMP_ECHOREPLY || *cp == ICMP_ECHO)
            printf(", id 0x%04x, seq 0x%04x", *(cp + 4) * 256 + *(cp + 5),
                   *(cp + 6) * 256 + *(cp + 7));
        printf("\n");
    }
}

int print_recv(uint8_t type, uint hlen, ssize_t received, char *from, uint seq, uint ttl, struct timeval *now)
{
    char message[40];
    char time[20];
    int error = 0;

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
        error = 1;
        break;
    case ICMP_TIME_EXCEEDED:
        snprintf(message, 23, "Time to live exceeded");
        error = 1;
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
    return error;
}