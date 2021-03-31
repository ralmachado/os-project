#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "libs/SharedMem.h"

pthread_t cars[10];
int racers = 0;

extern void log_message();

void* vroom() {
    log_message("Car thread created\n");
    sleep(2);
    log_message("Car thread exiting\n");
    pthread_exit(0);
}

void spawn_car() {
    if (racers < 10) {
        pthread_create(&cars[racers], NULL, vroom, NULL);
        racers++;
    }
}

void join_threads() {
    for (int i = 0; i < racers; i++) {
        pthread_join(cars[i], NULL);
        log_message("Team Manager joined threads\n");
    }

}

void team_execute() {
    for (int i = 0; i < 2; i++)
        spawn_car();

    join_threads();

    log_message("Team Manager exiting\n");
    exit(0);
}

void team_init() {
    log_message("Team Manager process spawned\n");
    team_execute();
}
