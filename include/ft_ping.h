#ifndef PING_H
#define PING_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <ctype.h>

#include "libft.h"
#include "argparse.h"

/**
 * @brief The size of the ping packet.
 *
 * This constant defines the size of the ping packet in bytes.
 * The default value is 64 bytes.
 */
#define PING_PKT_S 64

/**
 * @brief The rate at which the ping loop sleeps.
 *
 * This constant defines the rate at which the ping loop sleeps in microseconds.
 * The default value is 1 second.
 */
#define PING_SLEEP_RATE 1000000

/**
 * @brief The timeout delay for receiving packets.
 *
 * This constant defines the timeout delay for receiving packets in seconds.
 * The default value is 1 second.
 */
#define RECV_TIMEOUT 1

/**
 * @brief The ping packet structure.
 */
typedef struct s_ping_pkt
{
    struct icmphdr hdr;
    char msg[PING_PKT_S - sizeof(struct icmphdr)];
} __attribute__((packed)) t_ping_pkt;

/**
 * @brief Structure representing the arguments for the ping program.
 */
typedef struct s_ping_args
{
    int verbose;
    long count;
    char *host;
    char *ip;
    char *reverse_dns;
} t_ping_args;

int ft_ping(const char *argv[]);
char *dns_lookup(const char *host);
char *reverse_dns_lookup(const char *ip);
int parse_count_arg(t_ping_args *ping_args, t_argr *argr, const char *progname);

#endif