#include "ft_ping.h"

static t_argo options[] = {
    {'v', "verbose", "verbose", "verbose output", NO_ARG},
    {'c', "count", "count", "stop after <count> replies", ONE_ARG},
    {'?', "help", "help", "print help and exit", NO_ARG},
    {0}};

static t_argp argp = {
    .options = options,
    .args_doc = "[options] <destination>",
    .doc = ""};

int ft_ping(const char *argv[])
{
    t_argr *argr;
    t_ping_args ping_args;
    t_args *args;

    if (parse_args(&argp, argv, &args))
        return 1;

    ping_args.verbose = 0;
    ping_args.count = -1;

    while ((argr = get_next_option(args)))
    {
        if (argr->option && argr->option->sflag == '?')
        {
            help_args(&argp, argv[0]);
            free_args(args);
            return 1;
        }
        if (argr->option && argr->option->sflag == 'v')
            ping_args.verbose = 1;
        if (argr->option && argr->option->sflag == 'c')
        {
            if (parse_count_arg(&ping_args, argr, argv[0]))
            {
                free_args(args);
                return 1;
            }
        }
    }

    argr = get_next_arg(args);

    if (!argr)
    {
        printf("%s: destination argument required\n", argv[0]);
        free_args(args);
        return 1;
    }

    ping_args.host = argr->values[0];
    ping_args.ip = dns_lookup(ping_args.host);
    ping_args.reverse_dns = reverse_dns_lookup(ping_args.ip);

    for (int i = 0; i < ping_args.count || ping_args.count == -1; i++)
    {
        sleep(1);
    }

    free_args(args);

    return 0;
}
