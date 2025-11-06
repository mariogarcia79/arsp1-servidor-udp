#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <arpa/inet.h>
#include <sys/socket.h>
//#include <netinet/in.h> //double import from <arpa/inet.h>, not needed
#include <netdb.h>

#define MAX_IPV4_LENGTH    15
#define MAX_SERVICE_LENGTH 12
#define MAX_QUOTE_LEN     512

#define STRING_USAGE "Usage: %s <IP address> [-s service]\n"
#define STRING_QUOTE_HEADER "Quote Of The Day from"

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
    fprintf(stdout, "  -s, --service <service>  Specify the service to use (default: qotd)\n");
}

struct arguments {
    char *program_name;
    char *service;
};

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
    args->service = "qotd";
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

int
qotd_server_listen
(
    struct arguments   *args,
    struct sockaddr_in *myaddr
){
    struct servent *qotd_servent;
    int sockfd;

    /*
     * Fill servent structure to get QOTD port number.
     * The port number is obtained from /etc/services file using getservbyname()
     */
    if ((qotd_servent = getservbyname(args->service, "tcp")) == NULL) {
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

    // Open TCP socket with SOCK_STREAM flag
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return -1;
    }
    
    // Bind the client address to the socket
    if (bind(sockfd, (struct sockaddr *) myaddr, (socklen_t) sizeof(*myaddr)) == -1) {
        perror("bind");
        close(sockfd);
        return -1;
    }

    // Listen to client requests for QOTD
    if (listen(sockfd, 100) == -1) {
        perror("listen");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

/*
 * qotd_server_send_quote
 * This functions fetches the actual quote, appends it to the header passed
 * as argument to the function (should already have memory allocated), and 
 * then sends the whole datagram.
 * Returns 0 on success and -1 on error (sets errno).
 */
int
qotd_child_send_quote
(
    int clientfd,
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
    err = send(clientfd, quote, total_size, 0);
    if (err == -1) {
        perror("send");
        return -1;
    }

    if (shutdown(clientfd, SHUT_RDWR) == -1) {
        perror("shutdown");
        return -1;
    }

    return 0;
}

int
qotd_child_answer
(
    int clientfd
)
{
    int err;
    char *quote = NULL;
    char hostname[256]; // Maximum hostname size
    size_t total_size;

    err = recv(clientfd, NULL, 0, 0);
    if (err == -1) {
        perror("recv");
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

    err = qotd_child_send_quote(clientfd, quote, total_size);
    if (err == -1)
        goto exit_error_socket;

    free(quote);

    if (shutdown(clientfd, SHUT_RDWR) == -1) {
        perror("shutdown");
        goto exit_error_socket;
    }

    close(clientfd);
    return 0;

// clean exit
exit_error_socket:
    // No more receptions or transmissions
    if (shutdown(clientfd, SHUT_RDWR) == -1)
        perror("shutdown");
    close(clientfd);
    return -1;
}

int
qotd_server_accept
(
    int sockfd
){
    int err;
    int clientfd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    
    clientfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (clientfd == -1) {
        perror("accept");
        goto exit_error_socket;
    }
    
    return clientfd;

// clean exit
exit_error_socket:
    // No more receptions or transmissions
    if (shutdown(sockfd, SHUT_RDWR) == -1)
        perror("shutdown");
    close(sockfd);
    return -1;
}

int
main (int argc, char *argv[])
{
    struct sockaddr_in myaddr = {0};
    struct arguments   args   = {0};
    int sockfd;
    int clientfd;
    int err;
    pid_t pid;

    if (parse_args(argc, argv, &args) == -1) {
        perror("parse_args");
        print_usage(argv[0]);
        exit(1);
    }

    if (strcmp(args.service, "qotd")) {
        fprintf(stderr, "Service not found\n");
        exit(1);
    }

    sockfd = qotd_server_listen(&args, &myaddr);
    if (sockfd == -1) exit(1);

    while(1) {
        clientfd = qotd_server_accept(sockfd);
        if (clientfd == -1)
            goto exit_error_socket;
        
        pid = fork();
        if (pid == -1) {
            perror("fork failed");
            goto exit_error_socket;
        } else if (pid == 0) {
            err = qotd_child_answer(clientfd);
            if (err == -1)
                goto exit_error_socket;
            return 0;
        } else {
            wait(NULL);
        }
        
    }

    return 0;

// clean exit
exit_error_socket:
    // No more receptions or transmissions
    if (shutdown(sockfd, SHUT_RDWR) == -1)
        perror("shutdown");
    close(sockfd);
    return -1;
}
