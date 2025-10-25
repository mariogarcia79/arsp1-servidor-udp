#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <netinet/in.h>

#include "argparse.h"
#include "qotd.h"

typedef enum {
    SERVICE_UNKNOWN,
    SERVICE_QOTD
} ServiceType;

ServiceType
get_service_type(const char *name)
{
    if (strcmp(name, "qotd") == 0) return SERVICE_QOTD;
    return SERVICE_UNKNOWN;
}

int
main (int argc, char *argv[])
{
    struct sockaddr_in myaddr = {0};
    struct arguments   args   = {0};
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
    
    switch(service) {
        case SERVICE_QOTD:
            sockfd = qotd_setup_socket(&args, &myaddr);
            if (sockfd == -1) exit(1);
            while(1) {
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