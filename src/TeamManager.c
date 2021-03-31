#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "libs/SharedMem.h"

pthread_t cars[10];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int racers = 0, teamId;

extern void log_message();

void* vroom(void* r_id) {
    int id = *(int*)r_id;
    free(r_id);
    char buff[128];
    snprintf(buff, sizeof(buff) - 1, "Team Manager #%d: Car thread #%d created\n", teamId, id);
    log_message(buff);
    sleep(2);
    snprintf(buff, sizeof(buff) - 1, "Team Manager #%d: Car thread #%d exiting\n", teamId, id);
    pthread_mutex_lock(&mutex);
    log_message(buff);
    pthread_mutex_unlock(&mutex);
    pthread_exit(0);
}

void spawn_car() {
    if (racers < 10) {
        int* id;
        if (!(id = malloc(sizeof(int)))) {
            perror("malloc fail\n");
            return;
        }
        *id = racers + 1;
        pthread_create(&cars[racers], NULL, vroom, id);
        racers++;
    }
}

void join_threads() {
    char buff[64];
    for (int i = 0; i < racers; i++) {
        pthread_join(cars[i], NULL);
        snprintf(buff, sizeof(buff) - 1, "Team Manager #%d: joined thread #%d\n", teamId, i + 1);
        log_message(buff);
    }

}

void team_execute() {
    for (int i = 0; i < 2; i++)
        spawn_car();

    join_threads();

    char buff[64];
    snprintf(buff, sizeof(buff) - 1, "Team Manager #%d: Exiting\n", teamId);
    log_message(buff);
    exit(0);
}

void team_init(void* id) {
    teamId = *(int*)id;
    free(id);
    char buff[128];
    snprintf(buff, sizeof(buff) - 1, "Team Manager #%d: Process spawned\n", teamId);
    log_message(buff);
    team_execute();
}
