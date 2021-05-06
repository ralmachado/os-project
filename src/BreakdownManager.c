// David Valente Pereira Barros Leitão - 2019223148
// Rodrigo Alexandre da Mota Machado - 2019218299

// Breakdown Manager process functions

#include <signal.h>
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

char buff[BUFFSIZE];

void breakdown_int(int signum) {
    if (signum == SIGINT) exit(0);
} 

void get_msg() {
    msg my_msg;
    msgrcv(mqid, &my_msg, msglen, 0, 0);
    char buff[128];
    snprintf(buff, sizeof(buff), "[Breakdown Manager] Message: %s", my_msg.test);
    log_message(buff);
}

void create_break(){
    // TODO Update when know how to id each car, yes?
    while(1){
        msg breakdown;
        // breakdown.msgtype = ?;
        srand(getpid());
        int odds = rand() % RAND;
        for (int k = 0; k < configs.noTeams; k++) {
            for (int m = 0; m < shm->teams[k]->racers; m++) {
                int rl = shm->teams[k]->cars[m].reliability;
                if (rl != -1 && odds >= rl) {
                    msgsnd(mqid, &breakdown, msglen, 0);
                }
            }
        }
    }

    sleep(configs.tBreakdown);
}

void breakdown_manager() {
    struct sigaction sigint;
    sigint.sa_handler = breakdown_int;
    sigemptyset(&sigint.sa_mask);
    sigaddset(&sigint.sa_mask, SIGTSTP);
    sigint.sa_flags = 0;
    sigaction(SIGINT, &sigint, NULL);
    
    log_message("[Breakdown Manager] Process spawned");
    // Does its thing...
    sleep(5);
    log_message("[Breakdown Manager] Process exiting");
    exit(0);
}
