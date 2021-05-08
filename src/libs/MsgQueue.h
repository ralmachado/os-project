/* ----- Message Queue ----- */

#define MSGSIZE 64

typedef struct {
    long msgtype;
    // TODO define message struct
    char message[MSGSIZE];
} msg;

int mqid;
size_t msglen;