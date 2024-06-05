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
 * Sends an ICMP packet to the destination address.
 *
 * @param ping The PING structure containing the socket file descriptor and destination address.
 * @param ping_args The ping options specifying the packet size.
 * @return 0 if the packet is sent successfully, other otherwise.
 */
int send_packet(PING *ping)
{
    struct icmphdr *icp;
    struct timeval now;
    size_t len;
    int sent;

    len = sizeof(struct icmphdr) + ping->options.size;

    char packet[len];

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

void sig_handler(__attribute__((__unused__)) int signo)
{
    g_kill = true;
}

int ft_ping(const char *argv[])
{
    PING ping;

    if (ping_parse_args(&ping, argv))
        return 1;

    print_header(&ping);

    fd_set fdset;
    struct timeval timeout, last, interval, now;

    memset(&timeout, 0, sizeof(timeout));
    memset(&interval, 0, sizeof(interval));
    memset(&now, 0, sizeof(now));

    interval.tv_sec = 1;
    interval.tv_usec = 0;

    gettimeofday(&last, NULL);
    send_packet(&ping);

    while (!g_kill || ping.options.count == 0)
    {
        FD_ZERO(&fdset);
        FD_SET(ping.fd, &fdset);

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

        int result = select(ping.fd + 1, &fdset, NULL, NULL, &timeout);
        if (result < 0 && errno != EINTR)
        {
            perror("select");
            return 1;
        }
        else if (result == 1)
        {
            recv_packet(&ping);
        }
        else if (ping.num_emit < ping.options.count && !g_kill)
        {
            send_packet(&ping);
        }

        if (ping.count == ping.options.count && ping.num_recv == ping.num_emit)
            break;
        gettimeofday(&last, NULL);
    }

    print_stats(&ping);

    close(ping.fd);
    free(ping.hostname);

    return 0;
}
