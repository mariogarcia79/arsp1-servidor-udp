#ifndef QOTD_H
#define QOTD_H

#define STRING_QUOTE_HEADER "Quote Of The Day from"

int qotd_setup_socket
(
    struct arguments   *args,
    struct sockaddr_in *myaddr
);

int qotd_server_listen
(
    int sockfd
);

#endif /* QOTD_H */