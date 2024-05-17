#ifndef PING_H
#define PING_H

#include <ctype.h>
#include <stdbool.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>

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
    {'i', "interval", "interval", "wait NUMBER seconds between sending each packet", ONE_ARG},
    {'s', "size", "data size", "use <size> as number of data bytes to be sent", ONE_ARG},
    {'t', "ttl", "time to live", "define time to live", ONE_ARG},
    {'v', "verbose", "verbose", "verbose output", NO_ARG},
    {'?', "help", "help", "print help and exit", NO_ARG},
    {0}};

static t_argp argp = {
    .options = options,
    .args_doc = "[options] <destination>",
    .doc = ""};

#define MAXIPLEN 60
#define MAXICMPLEN 76

/**
 * @brief The size of the ping packet in bytes.
 */
#define PING_DEFAULT_PKT_S 64 - sizeof(struct icmphdr)

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
#define PING_MAX_DATALEN IP_MAXPACKET - MAXIPLEN - MAXICMPLEN

/**
 * @brief The default number of packets to send, 0 means infinite.
 */
#define PING_DEFAULT_COUNT 0

/**
 * @brief The options for the ping program.
 */
typedef struct s_ping_options
{
    bool verbose;
    size_t count;
    uint16_t size;
    float interval;
} t_ping_options;

typedef struct ping_data PING;

/**
 * @brief The data for the ping program.
 */
struct ping_data
{
    int fd;                       /* Socket file descriptor */
    uint16_t ident;               /* Process ID */
    size_t count;                 /* Number of packets to send */
    struct timeval start_time;    /* Time when the ping loop starts */
    size_t interval;              /* Interval between packets */
    struct sockaddr_in dest;      /* Destination address */
    struct sockaddr_in from;      /* Source address */
    char *hostname;               /* Hostname */
    size_t datalen;               /* Data byte count */
    struct icmphdr hdr;           /* ICMP header */
    unsigned char *packet_buffer; /* Packet buffer */
    size_t num_emit;              /* Number of packets transmitted */
    size_t num_recv;              /* Number of packets received */
    size_t num_rept;              /* Number of duplicates received */
    t_ping_options *options;      /* Ping options */
};

int ft_ping(const char *argv[]);
char *dns_lookup(const char *host);
char *reverse_dns_lookup(const char *ip);
int parse_count_arg(t_ping_options *ping_args, t_argr *argr, const char *progname);
int parse_size_arg(t_ping_options *ping_args, t_argr *argr, const char *progname);
int parse_interval_arg(t_ping_options *ping_args, t_argr *argr, const char *progname);

#endif