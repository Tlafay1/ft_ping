#include "ft_ping.h"

bool g_kill = false;

void sig_handler(__attribute__((__unused__)) int signo)
{
    g_kill = true;
}

int ping_loop(PING *ping)
{
    fd_set fdset;
    struct timeval timeout, last, interval, now;

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
            return 1;
        }
        else if (result == 1)
        {
            recv_packet(ping);
        }
        else if (ping->num_emit < ping->options.count && !g_kill)
        {
            send_packet(ping);
        }

        if (ping->count == ping->options.count && ping->num_recv == ping->num_emit)
        {
            break;
        }

        gettimeofday(&last, NULL);
    }

    return 0;
}

int ft_ping(const char *argv[])
{
    PING ping;
    int result;

    if (ping_parse_args(&ping, argv))
    {
        free(ping.hostname);
        return 1;
    }

    signal(SIGINT, sig_handler);

    print_header(&ping);

    result = ping_loop(&ping);

    print_stats(&ping);

    close(ping.fd);
    free(ping.hostname);

    return result;
}