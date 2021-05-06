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
#define RAND 100


extern void log_message();

sem_t *mutex;
char buff[64];

void get_msg() {
    msg my_msg;
    msgrcv(mqid, &my_msg, msglen, 0, 0);
    char buff[128];
    snprintf(buff, sizeof(buff), "[Breakdown Manager] Message: %s", my_msg.test);
    log_message(buff);
}

void create_break(){
    while(1){
        msg breakdown;
        srand(getpid());
        int odds = rand([0,RAND]);
        for (int k = 0; k<configs.noTeams){
            int rl = shm->teams[k]->cars.reliability;
            if (odds >= rl){
                msgsnd(mq_id,&breakdown, sizeof(msg)-sizeof(long),0);
            }
        }
    }
    sleep(configs->tBreakdown);
}

void breakdown_manager() {
    mutex = sem_open("MUTEX", 0);
    log_message("[Breakdown Manager] Process spawned");
    // Does its thing...
    sleep(5);
    log_message("[Breakdown Manager] Process exiting");
    sem_close(mutex);
    exit(0);
}
