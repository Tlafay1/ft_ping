#include "ft_ping.h"

static t_argo options[] = {
    {'v', NULL, "verbose", "verbose output", NO_ARG},
    // Currently bugged due to the lib
    {'?', NULL, "help", "print help and exit", NO_ARG},
    {0}
};

static t_argp argp = {
    .options = options,
    .args_doc = "[options] <destination>",
    .doc = ""
};

char    *dns_lookup(const char *host)
{
    struct addrinfo hints;
    struct addrinfo *res;
    char *ip = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, NULL, &hints, &res) != 0)
        return NULL;

    struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
    ip = ft_strdup(inet_ntoa(ipv4->sin_addr));
    freeaddrinfo(res);
    return ip;
}

char   *reverse_dns_lookup(const char *ip)
{
    struct sockaddr_in sa;
    char host[1024];
    char service[20];

    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr(ip);
    getnameinfo((struct sockaddr*)&sa, sizeof(sa), host, 1024, service, 20, 0);
    return ft_strdup(host);
}

int ft_ping(int argc, const char **argv)
{
	t_argr      *argr;
    t_ping_args args;

    t_list *head = parse_args(&argp, argc, argv);
    
    if (head == NULL)
        return 1;

    args.verbose = 0;

    while ((argr = get_next_option(head)))
	{
		if (argr->option && argr->option->sflag == '?')
        {
            help_args(&argp, argv[0]);
            free_args(head);
            return 1;
        }
        if (argr->option && argr->option->sflag == 'v')
            args.verbose = 1;
	}

    argr = get_next_arg(head);

    if (!argr)
    {
        printf("%s: destination argument required\n", argv[0]);
        free_args(head);
        return 1;
    }

    args.host = argr->values[0];
    args.ip = dns_lookup(args.host);
    args.reverse_dns = reverse_dns_lookup(args.ip);
    printf("PING %s (%s) 56(84) bytes of data.\n", args.host, args.ip);
    printf("64 bytes from %s: icmp_seq=1 ttl=64 time=0.000 ms\n", args.reverse_dns);

    free_args(head);
    return 0;
}
