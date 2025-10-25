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

/*
 * qotd_server_send_quote
 * This functions fetches the actual quote, appends it to the header passed
 * as argument to the function (should already have memory allocated), and 
 * then sends the whole datagram.
 * Returns 0 on success and -1 on error (sets errno).
 */
int
qotd_server_send_quote
(
    int sockfd,
    struct sockaddr_in *client_addr,
    char *quote,
    size_t total_size
){
    int err;

    FILE *fd;
    int pos = 0;
    char buffer[MAX_QUOTE_LEN];

    // Fetch the quote
    system("/usr/games/fortune -s > /tmp/quote.txt");
    fd = fopen("/tmp/quote.txt", "r");
    
    do {
        buffer[pos] = fgetc(fd);
        pos++;
    } while( pos < (MAX_QUOTE_LEN - 2) && !feof(fd) );

    buffer[pos - 1] = '\0';
    fclose(fd);

    // Append the actual quote to the message.
    // Memory for the message was already allocated.
    strcat(quote, buffer);

    // Send quote datagram
    err = sendto(sockfd, quote, total_size, 0,
            (struct sockaddr *)client_addr, (socklen_t)sizeof(*client_addr));
    if (err == -1) {
        perror("sendto");
        return -1;
    }

    return 0;
}

/*
 * qotd_server_listen
 * Gets the hostname of the machine to craft the QOTD message header,
 * then waits for incomming requests, and upon receipt crafts the header
 * and sends it using the qotd_server_send_quote() function.
 */
int
qotd_server_listen(int sockfd)
{
    int err;
    char *quote = NULL;
    char hostname[256]; // Maximum hostname size
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    size_t total_size;

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

    /*
     * Allocate memory for the whole message, although in this function I only set
     * the quote header. The actual message is set on the qotd_server_send_quote()
     */
    total_size = strlen(STRING_QUOTE_HEADER) +
                strlen(hostname) +
                MAX_QUOTE_LEN +
                3; // Account for separators
    quote = (char *)malloc(total_size);
    if (!quote) {
        perror("malloc");
        goto exit_error_socket;
    }

    memset(quote, 0, total_size);

    // Only pass the quote header
    snprintf(quote, total_size, "%s %s:\n", STRING_QUOTE_HEADER, hostname);

    err = qotd_server_send_quote(sockfd, &client_addr, quote, total_size);
    if (err == -1)
        goto exit_error_socket;

    free(quote);
    return 0;

exit_error_socket:
    if (quote)
        free(quote);
    close(sockfd);
    return -1;
}

/*
 * qotd_setup_socket
 * This function fetches a port, creates a socket, binds the IP address
 * to the socket and returns the socket file descriptor.
 */
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