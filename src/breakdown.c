// David Valente Pereira Barros Leitão - 2019223148
// Rodrigo Alexandre da Mota Machado - 2019218299

// Breakdown Manager process functions

#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "libs/header.h"
#include "libs/message.h"

#define RAND 101

extern void log_message();

char buff[BUFFSIZE];

void breakdown_int(int signum) {
    if (signum == SIGINT) {    
        log_message("[Breakdown Manager] Process exiting");
        exit(0);
    }
} 

void create_break(){
    Car* car;
    msg breakdown;  
    char buff[BUFFSIZE];
    srand(getpid());
    log_message("[Breakdown Manager] Generating malfunctions");
    while(1){
        if (shm->race_status != ONGOING) return;
        int odds = rand() % RAND;
        for (int k = 0; k < configs.noTeams; k++) {
            for (int m = 0; m < shm->teams[k].racers; m++) {
                car = &(shm->teams[k].cars[m]);
                if (odds >= car->reliability && car->state == RACE) {
                    snprintf(buff, sizeof(buff), "[Breakdown Manager] Malfunction in car %d", shm->teams[k].cars[m].number);
                    log_message(buff);
                    breakdown.msgtype = shm->teams[k].cars[m].number;
                    msgsnd(mqid, &breakdown, msglen, 0);
                }
            }
        }
        sleep(configs.tBreakdown);
    }
}

void breakdown_manager() {
    // struct sigaction sigint;
    // sigint.sa_handler = breakdown_int;
    // sigemptyset(&sigint.sa_mask);
    // sigaddset(&sigint.sa_mask, SIGTSTP);
    // sigint.sa_flags = 0;
    // sigaction(SIGINT, &sigint, NULL);

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGTSTP);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_SETMASK, &mask, NULL);
    
    log_message("[Breakdown Manager] Process spawned");
    // Does its thing...
    while (1) {
        log_message("[Breakdown Manager] Waiting for race start");
        pthread_mutex_lock(&shm->race_mutex);
        while (shm->race_status != ONGOING) {
            if (shm->race_int) {                
                log_message("[Breakdown Manager] Process exiting");
                pthread_mutex_unlock(&(shm->race_mutex));
                exit(0);
            }
            pthread_cond_wait(&(shm->race_cv), &(shm->race_mutex));
        }
        pthread_mutex_unlock(&shm->race_mutex);
        create_break();
    }
    // while (1) pause(); // TODO Change this to the actual breakdown function
    exit(0);
}
