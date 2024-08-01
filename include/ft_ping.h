#ifndef PING_H
#define PING_H

#include <ctype.h>
#include <stdbool.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <signal.h>
#include <math.h>

#include <sys/time.h>

#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/in_systm.h>

#include "libft.h"
#include "argparse.h"

static t_argo options[] = {
    {'c', "count", "count", "stop after <count> replies", ONE_ARG},
    {'i', "interval", "interval", "wait <number> seconds between sending each packet", ONE_ARG},
    {'n', "numeric", "numeric", "do not resolve host addresses.\n\t\t\t Here for swag purposes", NO_ARG},
    {'q', "quiet", "quiet", "quiet output", NO_ARG},
    {'s', "size", "data size", "use <size> as number of data bytes to be sent", ONE_ARG},
    {'t', "ttl", "time to live", "define time to live", ONE_ARG},
    {'v', "verbose", "verbose", "verbose output", NO_ARG},
    {'?', "help", "help", "print help and exit", NO_ARG},
    {0, NULL, NULL, NULL, NO_ARG}};

static t_argp argp __attribute__((unused)) = {
    .options = options,
    .args_doc = "[options] <destination>",
    .doc = ""};

#define MAXIPLEN 60
#define MAXICMPLEN 76

/**
 * @brief The size of the ping packet in bytes.
 */
#define PING_DEFAULT_PKT_S (64 - ICMP_MINLEN)

/**
 * @brief The rate at which the ping loop sleeps in microseconds.
 */
#define PING_DEFAULT_INTERVAL 1000000

/**
 * @brief The timeout delay for receiving packets in seconds.
 */
#define PING_DEFAULT_RECV_TIMEOUT 1

/**
 * @brief The maximum size of the packet data in bytes.
 */
#define PING_MAX_DATALEN (IP_MAXPACKET - MAXIPLEN - MAXICMPLEN)

/**
 * @brief The default number of packets to send, 0 means infinite.
 */
#define PING_DEFAULT_COUNT 0

/**
 * @brief The default time to live value.
 */
#define PING_DEFAULT_TTL 64

/**
 * @brief The options for the ping program.
 */
typedef struct s_ping_options
{
    bool verbose;
    size_t count;
    uint16_t size;
    float interval;
    int ttl;
    bool quiet;
} t_ping_options;

typedef struct s_ping_stats
{
    double min;
    double max;
    double sum;
    double sum_square;
} t_ping_stats;

/**
 * @brief The data for the ping program.
 */
typedef struct ping_data PING;
struct ping_data
{
    int fd;                       /* Socket file descriptor */
    uint16_t ident;               /* Process ID */
    size_t count;                 /* Number of packets to send */
    struct timeval start_time;    /* Time when the ping loop starts */
    size_t interval;              /* Interval between packets */
    struct sockaddr_in dest;      /* Destination address */
    struct sockaddr_in from;      /* Source address */
    char hostname[HOST_NAME_MAX]; /* Hostname */
    size_t datalen;               /* Data byte count */
    struct icmphdr hdr;           /* ICMP header */
    size_t num_emit;              /* Number of packets transmitted */
    size_t num_recv;              /* Number of packets received */
    size_t num_rept;              /* Number of duplicates received */
    size_t num_err;               /* Number of errors */
    t_ping_options options;       /* Ping options */
    t_ping_stats stats;           /* Ping statistics */
};

/* ft_ping.c */
int ft_ping(const char *argv[]);

/* init.c */
int parse_ping_options(t_ping_options *ping_options, t_args *args, const char *progname);
int ping_parse_args(PING *ping, const char *argv[]);
int ping_open_socket(const char *progname);
int ping_init(PING *ping, const char *progname);

/* print.c */
void print_stats(PING *ping);
void print_header(PING *ping);
void print_error_dump(struct icmphdr *icmp_packet, ssize_t received);
int print_recv(uint8_t type, uint hlen, ssize_t received, char *from, uint seq, uint ttl, struct timeval *now);

/* stats.c */
void calculate_stats(t_ping_stats *stats, struct timeval *sent);

/* icmp.c */
int send_packet(PING *ping);
int recv_packet(PING *ping);
void create_packet(PING *ping, struct icmphdr *packet, size_t len);

/* utils.c */
double nsqrt(double a, double prec);
uint16_t icmp_cksum(uint16_t *icmph, int len);
void tvsub(struct timeval *out, struct timeval *in);
int parse_count_arg(t_ping_options *ping_args, t_argr *argr, const char *progname);
int parse_size_arg(t_ping_options *ping_args, t_argr *argr, const char *progname);
int parse_interval_arg(t_ping_options *ping_args, t_argr *argr, const char *progname);
int parse_ttl_arg(t_ping_options *ping_args, t_argr *argr, const char *progname);
void calculate_timeout(struct timeval *timeout, struct timeval *last, struct timeval *interval);

#endif