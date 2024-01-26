#include "ft_ping.h"

static t_argo options[] = {
    {'v', "verbose", "verbose", "verbose output", NO_ARG},
    // Currently bugged due to the lib
    {'?', "help", "help", "print help and exit", NO_ARG},
    {0}
};

static t_argp argp = {
    .options = options,
    .args_doc = "[options] <destination>",
    .doc = ""
};

int ping(int argc, const char **argv)
{
    t_list *head = parse_args(&argp, argc, argv);
	t_argr *argr;

    if (head == NULL)
        return 1;
    while ((argr = get_next_option(head)))
	{
		if (argr->option && argr->option->sflag == '?')
        {
            help_args(&argp, argv[0]);
            return 0;
        }
        if (argr->option && argr->option->sflag == 'v')
            printf("Verbose mode activated\n");

        if (argr->option)
            printf("sflag: %c, lflag: %s, name: %s\n", argr->option->sflag,
                   argr->option->lflag, argr->option->name);
	}

    while ((argr = get_next_arg(head)))
	{
		// Be careful, the option can be NULL.
		if (argr->option)
		{
			printf("sflag: %c, lflag: %s, name: %s\n", argr->option->sflag,
				   argr->option->lflag, argr->option->name);
		}
		// Be careful, the values can be NULL.
		if (argr->values)
			for (int i = 0; argr->values[i]; i++)
				printf("%s\n", argr->values[i]);
	}

    free_args(head);
    return 0;
}

int main(int argc, const char **argv)
{
    return ping(argc, argv);
}