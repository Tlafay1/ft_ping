#include "ft_ping.h"

void create_packet(PING *ping, struct icmphdr *packet, size_t len)
{
    packet->type = ICMP_ECHO;
    packet->code = 0;
    packet->un.echo.id = htons(ping->ident);
    packet->un.echo.sequence = htons(ping->num_emit);
    packet->checksum = icmp_cksum((uint16_t *)packet, len);

    if (len >= sizeof(struct icmphdr) + sizeof(struct timeval))
    {
        struct timeval now;
        gettimeofday(&now, NULL);
        memcpy((char *)packet + sizeof(struct icmphdr), &now, sizeof(now));
    }
}

/**
 * Sends an ICMP packet to the destination address.
 *
 * @param ping The PING structure containing the socket file descriptor and destination address.
 * @param ping_args The ping options specifying the packet size.
 * @return 0 if the packet is sent successfully, other otherwise.
 */
int send_packet(PING *ping)
{
    size_t len;
    int sent;

    len = sizeof(struct icmphdr) + ping->options.size;

    char packet[len];

    create_packet(ping, (struct icmphdr *)packet, len);

    sent = sendto(ping->fd, packet, len, 0, (struct sockaddr *)&ping->dest, sizeof(ping->dest));
    if (sent < 0)
    {
        perror("sendto");
        return 1;
    }

    ping->count++;
    ping->num_emit++;

    return 0;
}

/**
 * Receives an ICMP packet and processes its contents.
 *
 * @param ping The PING structure containing the necessary information.
 * @param stats The ping statistics structure.
 * @param ping_options The ping options.
 * @return Returns 0 on success, other on failure.
 */
int recv_packet(PING *ping)
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

    if (!ping->options.quiet)
        print_recv(
            icp->type,
            hlen,
            received - hlen,
            inet_ntoa(*(struct in_addr *)&from.sin_addr.s_addr),
            ntohs(icp->un.echo.sequence),
            ip_packet->ip_ttl,
            &now);

    ping->num_recv++;

    if (received >= (ssize_t)(hlen + sizeof(struct icmphdr) + sizeof(struct timeval)))
        calculate_stats(&ping->stats, &now);

    return 0;
}