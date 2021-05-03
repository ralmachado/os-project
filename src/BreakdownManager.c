// David Valente Pereira Barros Leitão - 2019223148
// Rodrigo Alexandre da Mota Machado - 2019218299

// Breakdown Manager process functions

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <semaphore.h>

#include "libs/SharedMem.h"
#include "libs/MsgQueue.h"

extern void log_message();

sem_t *mutex;
char buff[64];

void get_msg() {
    msg my_msg;
    msgrcv(mq_id, &my_msg, sizeof(msg)-sizeof(long), 0, 0);
    char buff[128];
    snprintf(buff, sizeof(buff), "[Breakdown Manager] Message: %s", my_msg.test);
    log_message(buff);
}

void breakdown_manager() {
    mutex = sem_open("MUTEX", 0);
    sem_wait(mutex);
    log_message("[Breakdown Manager] Process spawned");
    snprintf(buff, sizeof(buff)-1, "[Breakdown Manager] tBreakdown -> %d", configs->tBreakdown);
    log_message(buff);
    snprintf(buff, sizeof(buff)-1, "[Breakdown Manager] tBoxMin -> %d", configs->tBoxMin);
    log_message(buff);
    snprintf(buff, sizeof(buff)-1, "[Breakdown Manager] tBoxMax -> %d", configs->tBoxMax);
    log_message(buff);
    get_msg();
    sem_post(mutex);
    sleep(5);
    sem_wait(mutex);
    log_message("[Breakdown Manager] Process exiting");
    sem_post(mutex);
    exit(0);
}
