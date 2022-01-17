#ifndef SOCKETQ_H_INCLUDED
#define SOCKETQ_H_INCLUDED


#include <stdlib.h>
#include <stdint.h>


typedef struct {
    char  *path;
    int    server;
    int    client;
    size_t msg_size;
} socketq_t;


int  socketq_init(socketq_t *socketq, char *path, size_t msg_size);
void socketq_send(socketq_t *socketq, uint8_t *message);
int  socketq_receive(socketq_t *socketq, uint8_t *message);
int  socketq_receive_nonblock(socketq_t *socketq, uint8_t *message, int timeout);


#endif