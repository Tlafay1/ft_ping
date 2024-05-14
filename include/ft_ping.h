#ifndef PING_H
#define PING_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "libft.h"
#include "argparse.h"

typedef struct s_ping_args
{
    int verbose;
    char *host;
    char *ip;
    char *reverse_dns;
} t_ping_args;

int ft_ping(int argc, const char **argv);

#endif