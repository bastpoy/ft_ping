#include "header.h"
#define OPT_TTL 256

static const struct option long_opts[] = {
    {"ttl", required_argument, NULL, OPT_TTL},
    {NULL,  0,                 NULL, 0}        // terminateur obligatoire
};

static int parse_int(const char *str, int min, int max, int *out)
{
    char *end;
    long val;

    errno = 0;
    val = strtol(str, &end, 10);
    if (errno != 0 || end == str || *end != '\0')
        return (-1);                 // pas un nombre valide
    if (val < min || val > max)
        return (-1);                 // hors limites
    *out = (int)val;
    return (0);
}

static int init_socket(struct ping_ctx *ctx)
{
    struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };

    ctx->socket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (ctx->socket < 0) {
        printf("error creating socket: %s\n", strerror(errno));
        return (1);
    }
    if (setsockopt(ctx->socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        printf("setsockopt: %s\n", strerror(errno));
        close(ctx->socket);
        return (1);
    }
    if (ctx->args.ttl > 0
    && setsockopt(ctx->socket, IPPROTO_IP, IP_TTL,
                  &ctx->args.ttl, sizeof(ctx->args.ttl)) < 0) {
    printf("setsockopt: %s\n", strerror(errno));
    close(ctx->socket);
    return (1);
}
    return (0);
}

int init_ctx(struct ping_ctx *ctx, int argc, char **argv)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->stats.sequence = 0;

    int ret = check_args(argc, argv, ctx);
    if (ret != 0)
        return (ret);

    return (init_socket(ctx));
}

int check_args(int argc, char *argv[], struct ping_ctx *ctx){
    int opt;

    ctx->args.verbose = false;
    ctx->args.ttl = 0;
    opterr = 0;

    while ((opt = getopt_long(argc, argv, "vh?", long_opts, NULL)) != -1){
        switch (opt){
        case 'v':
            ctx->args.verbose = true;
            break;
        case 'h':
            print_help();
            return 3;
        case OPT_TTL:
            if (parse_int(optarg, 1, 255, &ctx->args.ttl) != 0){
                printf("ping: invalid value (`%s' near `%s')\n", optarg, optarg);
                return 1;
            }
            break;
        case '?':
            if (optopt == OPT_TTL)
                printf("ping: option '--ttl' requires an argument\n");
            else if (optopt == 0)
                printf("ping: unrecognized option '%s'\n", argv[optind - 1]);
            else if (optopt != '?')
                printf("ping: invalid option -- '%c'\n", optopt);
            print_help();
            return 2;
        }
    }

    if (optind >= argc){
        printf("ping: missing host operand\n");
        return 2;
    }
    if (optind + 1 < argc){
        print_help();
        return 2;
    }
    ctx->index = optind;
    return 0;
}