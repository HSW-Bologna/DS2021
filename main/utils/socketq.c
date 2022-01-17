#include <assert.h>
#include <stdlib.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include "socketq.h"
#include "log.h"


int socketq_init(socketq_t *socketq, char *path, size_t msg_size) {
    assert(socketq != NULL);
    struct sockaddr_un local;
    int                len;
    int                fd;

    if ((fd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
        log_error("Error creating server socket: %s", strerror(errno));
        return -1;
    }

    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, path);
    unlink(local.sun_path);
    len = strlen(local.sun_path) + sizeof(local.sun_family);
    if (bind(fd, (struct sockaddr *)&local, len) == -1) {
        log_error("Error binding server socket: %s", strerror(errno));
        return -1;
    }

    socketq->server = fd;

    fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd <= 0) {
        log_error("Error creating client socket: %s", strerror(errno));
        return -1;
    } else {
        socketq->client = fd;
    }

    socketq->path     = path;
    socketq->msg_size = msg_size;

    return 0;
}


void socketq_send(socketq_t *socketq, uint8_t *message) {
    assert(socketq != NULL);
    struct sockaddr_un remote;
    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, socketq->path);
    int res = sendto(socketq->client, message, socketq->msg_size, 0, (struct sockaddr *)&remote, sizeof(remote));
    if (res != (int)socketq->msg_size) {
        log_error("Error while sending message: %s", strerror(errno));
    }
}


int socketq_receive(socketq_t *socketq, uint8_t *message) {
    assert(socketq != NULL);
    struct sockaddr_un peer_socket;
    socklen_t          peer_socket_len = sizeof(peer_socket);

    return recvfrom(socketq->server, message, socketq->msg_size, 0, (struct sockaddr *)&peer_socket,
                    &peer_socket_len) == (int)socketq->msg_size;
}


int socketq_receive_nonblock(socketq_t *socketq, uint8_t *message, int timeout) {
    struct sockaddr_un peer_socket;
    socklen_t          peer_socket_len = sizeof(peer_socket);

    struct pollfd fds[1] = {{.fd = socketq->server, .events = POLLIN}};
    if (poll(fds, 1, timeout)) {
        return recvfrom(socketq->server, message, socketq->msg_size, 0, (struct sockaddr *)&peer_socket,
                        &peer_socket_len) == (int)socketq->msg_size;
    } else {
        return 0;
    }
}