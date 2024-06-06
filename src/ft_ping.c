#include "ft_ping.h"

bool g_kill = false;

void sig_handler(__attribute__((__unused__)) int signo)
{
    g_kill = true;
}

int ping_loop(PING *ping)
{
    fd_set fdset;
    struct timeval timeout, last, interval;

    FD_ZERO(&fdset);
    FD_SET(ping->fd, &fdset);

    interval.tv_sec = 1;
    interval.tv_usec = 0;

    gettimeofday(&last, NULL);
    send_packet(ping);

    while (!g_kill || ping->options.count == 0)
    {
        FD_ZERO(&fdset);
        FD_SET(ping->fd, &fdset);

        calculate_timeout(&timeout, &last, &interval);

        int result = select(ping->fd + 1, &fdset, NULL, NULL, &timeout);
        if (result < 0 && errno != EINTR)
        {
            perror("select");
            return 1;
        }
        else if (result == 1)
            recv_packet(ping);
        else if (ping->num_emit < ping->options.count && !g_kill)
            send_packet(ping);

        if (ping->count == ping->options.count && ping->num_recv == ping->num_emit)
            break;

        gettimeofday(&last, NULL);
    }

    return 0;
}

int ft_ping(const char *argv[])
{
    PING ping;
    int result;

    if (ping_parse_args(&ping, argv))
        return 1;

    signal(SIGINT, sig_handler);

    print_header(&ping);

    result = ping_loop(&ping);

    print_stats(&ping);

    close(ping.fd);
    return result;
}