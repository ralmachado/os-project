#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "libs/SharedMem.h"

pthread_t* cars;
pthread_mutex_t t_mutex = PTHREAD_MUTEX_INITIALIZER;
int racers = 0, teamId, * box;

extern void log_message();

// Car threads live here
void* vroom(void* r_id) {
    int id = *(int*)r_id;
    free(r_id);
    char buff[128];
    snprintf(buff, sizeof(buff) - 1, "Team Manager #%d: Car thread #%d created\n", teamId, id);
    log_message(buff);
    sleep(2);
    snprintf(buff, sizeof(buff) - 1, "Team Manager #%d: Car thread #%d exiting\n", teamId, id);
    
    pthread_mutex_lock(&t_mutex);
    log_message(buff);
    pthread_mutex_unlock(&t_mutex);

    pthread_exit(0);
}

// Create a car thread if conditions match
void spawn_car() {
    if (racers < configs->maxCars) {
        int* id;
        if (!(id = malloc(sizeof(int)))) {
            perror("malloc fail");
            return;
        }
        *id = racers + 1;
        pthread_create(&cars[racers], NULL, vroom, id);
        racers++;
    }
}

// Await for all car threads to exit
void join_threads() {
    char buff[64];
    for (int i = 0; i < racers; i++) {
        pthread_join(cars[i], NULL);
        snprintf(buff, sizeof(buff) - 1, "Team Manager #%d: joined thread #%d\n", teamId, i + 1);
        log_message(buff);
    }

}

// Team Manager process lives here
void team_execute() {
    char buff[64];
    snprintf(buff, sizeof(buff) - 1, "Team Manager #%d: Box state = %d\n", teamId, *box);
    log_message(buff);
    for (int i = 0; i < 2; i++)
        spawn_car();

    join_threads();

    snprintf(buff, sizeof(buff) - 1, "Team Manager #%d: Process exiting\n", teamId);
    log_message(buff);
    exit(0);
}

// Setup Team Manager
void team_init(void* id) {
    teamId = *(int*)id;
    free(id);
    char buff[128];
    snprintf(buff, sizeof(buff) - 1, "Team Manager #%d: Process spawned\n", teamId);

    cars = malloc(sizeof(pthread_t) * configs->maxCars);
    box = boxes + (teamId - 1);

    log_message(buff);
    team_execute();
}
