#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdint.h>

#define MAX_LENGTH_TEXT 128
#define CKSUM 8 /* checksum as 8 hex chars, so CHAN can corrupt it like any other char */
#define CAP 8
#define SHM_NAME "/os_cw1"

typedef enum{ MSG_DATA, MSG_ACK, MSG_NACK, MSG_TERM, MSG_FAIL} msg_type;

typedef struct{
    msg_type type;
    char text[MAX_LENGTH_TEXT + CKSUM + 1];
}msg_t;

typedef struct{
    msg_t buf[CAP];
    int in, out;
    sem_t empty;
    sem_t full;
    sem_t mutex;
}channel_t;


typedef struct{
    channel_t p_to_enc[2];     /* P1→ENC1,  P2→ENC2 */
    channel_t enc_to_chan[2];  /* ENC1→CHAN, ENC2→CHAN */
    channel_t chan_to_enc[2];  /* CHAN→ENC1, CHAN→ENC2 (data + ACK/NACK) */
    channel_t enc_to_p[2];     /* ENC1→P1,  ENC2→P2: verified text, or a "delivery failed" notice */
    double p;
    int n;
}shared_t;

/*Ipc.c functions*/
shared_t* ipc_create(double p, int n);
shared_t* ipc_attach(void);
void ipc_detach(shared_t* s);
void ipc_destroy(shared_t* s);
void channel_put(channel_t* channel, msg_t* m);
void channel_get(channel_t* channel, msg_t* m);



void enc_run(int id, shared_t* s);


void chan_run(shared_t* s);










#endif