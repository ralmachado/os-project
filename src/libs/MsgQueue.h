/* ----- Message Queue ----- */

#ifndef BUFFSIZE
#define BUFFSIZE 256
#endif

typedef struct {
    long msgtype;
    // TODO define message struct
    char message[BUFFSIZE];
} msg;

int mqid;
size_t msglen;