// David Valente Pereira Barros Leitão - 2019223148
// Rodrigo Alexandre da Mota Machado - 2019218299

// Breakdown Manager process functions

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

#include "libs/SharedMem.h"

extern void log_message();

sem_t *mutex;
char buff[64];

void breakdown_manager() {
    mutex = sem_open("MUTEX", 0);
    sem_wait(mutex);
    log_message("[Breakdown Manager] Process spawned\n");
    snprintf(buff, sizeof(buff)-1, "[Breakdown Manager] tBreakdown -> %d\n", configs->tBreakdown);
    log_message(buff);
    snprintf(buff, sizeof(buff)-1, "[Breakdown Manager] tBoxMin -> %d\n", configs->tBoxMin);
    log_message(buff);
    snprintf(buff, sizeof(buff)-1, "[Breakdown Manager] tBoxMax -> %d\n", configs->tBoxMax);
    log_message(buff);
    sem_post(mutex);
    sleep(5);
    sem_wait(mutex);
    log_message("[Breakdown Manager] Process exiting\n");
    sem_post(mutex);
    exit(0);
}
