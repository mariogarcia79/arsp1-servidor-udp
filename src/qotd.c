#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>
//#include <netinet/in.h> //double import from <arpa/inet.h>, not needed
#include <netdb.h>

#include "argparse.h"
#include "config.h"
#include "qotd.h"

#define MAX_QUOTE_LEN 1000

int
qotd_server_send_quote
(
    int sockfd,
    struct sockaddr_in *client_addr,
    char *quote
){
    int err;

    FILE *fd;
    int pos = 0;
    char buffer[MAX_QUOTE_LEN];

    system("/usr/games/fortune -s > /tmp/quote.txt");
    fd = fopen("/tmp/quote.txt", "r");
    
    do {
        buffer[pos] = fgetc(fd);
        pos++;
    } while( pos < (MAX_QUOTE_LEN - 2) && !feof(fd) );

    buffer[pos - 1] = '\0';
    fclose(fd);

    strcat(quote, buffer);

    // Send quote message
    err = sendto(sockfd, quote, sizeof(quote), 0,
            (struct sockaddr *)client_addr, (socklen_t)sizeof(*client_addr));
    if (err == -1) {
        perror("sendto");
        close(sockfd);
        return -1;
    }

    return 0;
}

int
qotd_server_listen(int sockfd)
{
    int err;
    char *quote = NULL;
    char hostname[256]; // Maximum hostname size
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    err = gethostname(hostname, sizeof(hostname));
    if (err != 0) {
        perror("gethostname");
        goto exit_error_socket;
    }

    err = recvfrom(sockfd, NULL, 0, 0, (struct sockaddr *) &client_addr, &client_addr_len);
    if (err == -1) {
        perror("recvfrom");
        goto exit_error_socket;
    }

    size_t total_size = strlen(STRING_QUOTE_HEADER) +
                        strlen(hostname) +
                        MAX_QUOTE_LEN +
                        5; // Account for separators
    quote = (char *)malloc(total_size);
    if (!quote) {
        perror("malloc");
        goto exit_error_socket;
    }

    memset(quote, 0, total_size);

    // Only pass the quote header
    snprintf(quote, total_size, "%s %s:\n", STRING_QUOTE_HEADER, hostname);

    qotd_server_send_quote(sockfd, &client_addr, quote);

    free(quote);
    return 0;

exit_error_socket:
    if (quote)
        free(quote);
    close(sockfd);
    return -1;
}

int
qotd_setup_socket(
    struct arguments   *args,
    struct sockaddr_in *myaddr
){
    struct servent *qotd_servent;
    int sockfd;

    /*
     * Fill servent structure to get QOTD port number.
     * The port number is obtained from /etc/services file using getservbyname()
     */
    if ((qotd_servent = getservbyname(args->service, "udp")) == NULL) {
        fprintf(stderr, "Could not resolve QOTD's port number\n");
        return -1;
    }

    /*
     * Fill sockaddr_in structure with the client address (used for binding).
     * INADDR_ANY is used to bind to all interfaces.
     */
    myaddr->sin_family = AF_INET;
    myaddr->sin_port = qotd_servent->s_port;
    myaddr->sin_addr.s_addr = INADDR_ANY;

    // Open UDP socket with SOCK_DGRAM flag
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        return -1;
    }

    // Bind the client address to the socket
    if (bind(sockfd, (struct sockaddr *) myaddr, (socklen_t) sizeof(*myaddr)) == -1) {
        perror("bind");
        close(sockfd);
        return -1;
    }

    return sockfd;
}