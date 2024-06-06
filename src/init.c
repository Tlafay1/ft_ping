#include "ft_ping.h"

/**
 * Opens a socket for ICMP communication.
 *
 * This function creates a socket for sending and receiving ICMP packets.
 * It first retrieves the protocol information for ICMP using `getprotobyname()`.
 * If the protocol information is not found, an error message is printed and -1 is returned.
 * Otherwise, it creates a raw socket using `socket()` with the retrieved protocol.
 * If the raw socket creation fails due to lack of privilege, it falls back to creating a datagram socket.
 * If the socket creation fails for any other reason, an error message is printed and -1 is returned.
 *
 * @param progname The name of the program.
 * @return The file descriptor of the opened socket, or -1 if an error occurred.
 */
static int ping_open_socket(const char *progname)
{
    int fd;
    struct protoent *proto;

    proto = getprotobyname("icmp");
    if (!proto)
    {
        fprintf(stderr, "%s: unknown protocol icmp.\n", progname);
        return -1;
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

                return -1;
            }
        }
        else
            return -1;
    }

    return fd;
}

/**
 * Initializes a PING structure with the given program name, argument reader, and ping options.
 *
 * @param progname The name of the program.
 * @param argr The argument reader structure.
 * @param ping_options The ping options structure.
 * @return A pointer to the initialized PING structure, or NULL if an error occurred.
 */
static PING *ping_init(PING *ping, const char *progname)
{
    ping->fd = ping_open_socket(progname);
    ping->count = 0;
    ping->interval = 1;
    ping->datalen = ping->options.size;
    ping->ident = getpid() & 0xFFFF;
    ping->num_emit = 0;
    ping->num_recv = 0;
    ping->num_rept = 0;
    gettimeofday(&ping->start_time, NULL);

    ping->stats.sum = -1;
    ping->stats.min = -1;
    ping->stats.max = -1;

    if (ping->options.ttl > 0)
        if (setsockopt(ping->fd, IPPROTO_IP, IP_TTL,
                       &ping->options.ttl, sizeof(ping->options.ttl)) < 0)
        {
            perror("setsockopt");
            free(ping);
            return NULL;
        }

    return ping;
}

/**
 * Sets the destination address for the PING structure.
 *
 * @param ping The PING structure to set the destination for.
 * @param host The hostname or IP address of the destination.
 * @return Returns 0 on success, or 1 if an error occurred.
 */
static int set_dest(PING *ping, const char *host)
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

/**
 * Parses the ping options from the command line arguments.
 *
 * @param ping_options The structure to store the parsed ping options.
 * @param args The command line arguments.
 * @param progname The name of the program.
 * @return Returns 0 on success, or 1 if there was an error.
 */
int parse_ping_options(t_ping_options *ping_options, t_args *args, const char *progname)
{
    t_argr *argr;

    ping_options->verbose = false;
    ping_options->count = PING_DEFAULT_COUNT;
    ping_options->size = PING_DEFAULT_PKT_S;
    ping_options->interval = PING_DEFAULT_INTERVAL;
    ping_options->ttl = PING_DEFAULT_TTL;
    ping_options->quiet = false;

    while ((argr = get_next_option(args)))
    {
        switch (argr->option->sflag)
        {
        case '?':
            help_args(&argp, progname);
            free_args(args);
            return 1;
        case 'v':
            ping_options->verbose = true;
            break;
        case 'c':
            if (parse_count_arg(ping_options, argr, progname))
            {
                free_args(args);
                return 1;
            }
            break;
        case 's':
            if (parse_size_arg(ping_options, argr, progname))
            {
                free_args(args);
                return 1;
            }
            break;
        case 'i':
            if (parse_interval_arg(ping_options, argr, progname))
            {
                free_args(args);
                return 1;
            }
            break;
        case 't':
            if (parse_ttl_arg(ping_options, argr, progname))
            {
                free_args(args);
                return 1;
            }
            break;
        case 'q':
            ping_options->quiet = true;
            break;
        }
    }
    return 0;
}

/**
 * Parses the command line arguments and initializes the PING structure.
 *
 * @param ping The PING structure to be initialized.
 * @param argv The command line arguments.
 * @return Returns 0 on success, or 1 if there was an error.
 */
int ping_parse_args(PING *ping, const char *argv[])
{
    t_args *args;
    int ret;

    if (parse_args(&argp, argv, &args))
    {
        free_args(args);
        return 1;
    }
    t_argr *argr = get_next_arg(args);

    if (!argr)
    {
        printf("%s: destination argument required\n", argv[0]);
        free_args(args);
        return 1;
    }
    ret = parse_ping_options(&ping->options, args, argv[0]);
    ping = ping_init(ping, argv[0]);
    set_dest(ping, argr->values[0]);
    free_args(args);
    return ret;
}
