#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "argparse.h"
#include "config.h"

void print_help(void);
void init_defaults(char *prog_name, struct arguments *args);

typedef enum {
    FLAG_UNKNOWN = -1,
    FLAG_HELP    = 0,
    FLAG_SERVICE = 1
} FlagType;

FlagType
get_flag(const char *arg)
{
    if (!strcmp(arg, "-h"))        return FLAG_HELP;
    if (!strcmp(arg, "--help"))    return FLAG_HELP;
    if (!strcmp(arg, "-s"))        return FLAG_SERVICE;
    if (!strcmp(arg, "--service")) return FLAG_SERVICE;
    return FLAG_UNKNOWN;
}

void
init_defaults(char *prog_name, struct arguments *args)
{
    args->program_name = prog_name;
    args->service = SERVICE_DEFAULT;
}

/* 
 * parse_args()
 * Parses command line arguments and fills the arguments struct.
 * Returns 0 on success, -1 on failure setting errno.
 */
int
parse_args(int argc, char *argv[], struct arguments *args)
{
    int count = 1;
    
    init_defaults(argv[0], args);
    
    while (count < argc) {
        if (argv[count][0] == '-') {
            // It's a flag, handle accordingly
            switch (get_flag(argv[count])) {
                case FLAG_HELP:
                    print_usage(args->program_name);
                    print_help();
                    goto exit_success;
                case FLAG_SERVICE:
                    count++;
                    if (count >= argc)
                        goto exit_error_invalid;
                    else
                        args->service = argv[count];
                    break;
                default:
                    goto exit_error_invalid;
            }
        }
        count++;
    }

exit_success:
    return 0;
exit_error_invalid:
    errno = EINVAL;
    return -1;
}

void
print_usage(const char *program_name)
{
    fprintf(stdout, STRING_USAGE, program_name);
}

void
print_help(void)
{
    fprintf(stdout, "Options:\n");
    fprintf(stdout, "  -h, --help               Show this help message and exit\n");
    fprintf(stdout, "  -s, --service <service>  Specify the service to use (default: %s)\n", SERVICE_DEFAULT);
}