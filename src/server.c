#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

#define MAX_QUOTE_LEN 1000
#define STRING_USAGE "Usage: %s [-s service]\n"
#define STRING_QUOTE_HEADER "Quote Of The Day from"

#ifndef ARGPARSE_H
#define ARGPARSE_H

struct arguments {
    char *program_name;
    char *service;
};

void print_usage(const char *program_name);
void print_help(void);
int parse_args(int argc, char *argv[], struct arguments *args);

#endif /* ARGPARSE_H */

#ifndef CONFIG_H
#define CONFIG_H

#define MAX_IPV4_LENGTH 15
#define MAX_SERVICE_LENGTH 12
#define SERVICE_DEFAULT "qotd"

#endif /* CONFIG_H */

#ifndef QOTD_H
#define QOTD_H

int qotd_setup_socket(struct arguments *args, struct sockaddr_in *myaddr);
int qotd_server_listen(int sockfd);
int qotd_server_send_quote(int sockfd, struct sockaddr_in *client_addr, char *quote, size_t total_size);

#endif /* QOTD_H */

typedef enum {
    FLAG_UNKNOWN = -1,
    FLAG_HELP    = 0,
    FLAG_SERVICE = 1
} FlagType;

typedef enum {
    SERVICE_UNKNOWN,
    SERVICE_QOTD
} ServiceType;

FlagType get_flag(const char *arg) {
    if (!strcmp(arg, "-h")) return FLAG_HELP;
    if (!strcmp(arg, "--help")) return FLAG_HELP;
    if (!strcmp(arg, "-s")) return FLAG_SERVICE;
    if (!strcmp(arg, "--service")) return FLAG_SERVICE;
    return FLAG_UNKNOWN;
}

void init_defaults(char *prog_name, struct arguments *args) {
    args->program_name = prog_name;
    args->service = SERVICE_DEFAULT;
}

int parse_args(int argc, char *argv[], struct arguments *args) {
    int count = 1;
    init_defaults(argv[0], args);

    while (count < argc) {
        if (argv[count][0] == '-') {
            switch (get_flag(argv[count])) {
                case FLAG_HELP:
                    print_usage(args->program_name);
                    print_help();
                    return 0;
                case FLAG_SERVICE:
                    count++;
                    if (count >= argc) return -1;
                    args->service = argv[count];
                    break;
                default:
                    return -1;
            }
        }
        count++;
    }
    return 0;
}

void print_usage(const char *program_name) {
    fprintf(stdout, STRING_USAGE, program_name);
}

void print_help(void) {
    fprintf(stdout, "Options:\n");
    fprintf(stdout, "  -h, --help               Show this help message and exit\n");
    fprintf(stdout, "  -s, --service <service>  Specify the service to use (default: %s)\n", SERVICE_DEFAULT);
}

ServiceType get_service_type(const char *name) {
    if (strcmp(name, "qotd") == 0) return SERVICE_QOTD;
    return SERVICE_UNKNOWN;
}

int qotd_server_send_quote(int sockfd, struct sockaddr_in *client_addr, char *quote, size_t total_size) {
    int err;
    FILE *fd;
    int pos = 0;
    char buffer[MAX_QUOTE_LEN];

    system("/usr/games/fortune -s > /tmp/quote.txt");
    fd = fopen("/tmp/quote.txt", "r");

    do {
        buffer[pos] = fgetc(fd);
        pos++;
    } while (pos < (MAX_QUOTE_LEN - 2) && !feof(fd));

    buffer[pos - 1] = '\0';
    fclose(fd);

    strcat(quote, buffer);

    err = sendto(sockfd, quote, total_size, 0, (struct sockaddr *)client_addr, (socklen_t)sizeof(*client_addr));
    if (err == -1) {
        perror("sendto");
        return -1;
    }

    return 0;
}

int qotd_server_listen(int sockfd) {
    int err;
    char *quote = NULL;
    char hostname[256];
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    size_t total_size;

    err = gethostname(hostname, sizeof(hostname));
    if (err != 0) {
        perror("gethostname");
        return -1;
    }

    err = recvfrom(sockfd, NULL, 0, 0, (struct sockaddr *)&client_addr, &client_addr_len);
    if (err == -1) {
        perror("recvfrom");
        return -1;
    }

    total_size = strlen(STRING_QUOTE_HEADER) + strlen(hostname) + MAX_QUOTE_LEN + 3;
    quote = (char *)malloc(total_size);
    if (!quote) {
        perror("malloc");
        return -1;
    }

    memset(quote, 0, total_size);
    snprintf(quote, total_size, "%s %s:\n", STRING_QUOTE_HEADER, hostname);

    err = qotd_server_send_quote(sockfd, &client_addr, quote, total_size);
    if (err == -1) {
        free(quote);
        return -1;
    }

    free(quote);
    return 0;
}

int qotd_setup_socket(struct arguments *args, struct sockaddr_in *myaddr) {
    struct servent *qotd_servent;
    int sockfd;

    if ((qotd_servent = getservbyname(args->service, "udp")) == NULL) {
        fprintf(stderr, "Could not resolve QOTD's port number\n");
        return -1;
    }

    myaddr->sin_family = AF_INET;
    myaddr->sin_port = qotd_servent->s_port;
    myaddr->sin_addr.s_addr = INADDR_ANY;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        return -1;
    }

    if (bind(sockfd, (struct sockaddr *)myaddr, (socklen_t)sizeof(*myaddr)) == -1) {
        perror("bind");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int main(int argc, char *argv[]) {
    struct sockaddr_in myaddr = {0};
    struct arguments args = {0};
    ServiceType service;
    int sockfd;
    int err;

    err = parse_args(argc, argv, &args);
    if (err == -1) {
        perror("parse_args");
        print_usage(argv[0]);
        exit(1);
    }

    service = get_service_type(args.service);

    switch (service) {
        case SERVICE_QOTD:
            sockfd = qotd_setup_socket(&args, &myaddr);
            if (sockfd == -1) exit(1);

            while (1) {
                err = qotd_server_listen(sockfd);
                if (err == -1) exit(1);
            }
            close(sockfd);
            break;
        case SERVICE_UNKNOWN:
        default:
            fprintf(stderr, "Service not found\n");
            exit(1);
    }

    return 0;
}
