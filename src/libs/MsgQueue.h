/* ----- Message Queue ----- */

typedef struct {
    long msgtype;
    // TODO define message struct
    char *test;
} msg;

int mqid;
size_t msglen;