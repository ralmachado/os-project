// David Valente Pereira Barros Leitão - 2019223148
// Rodrigo Alexandre da Mota Machado - 2019218299

// Team Manager process functions

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/msg.h>
#include "libs/SharedMem.h"
#include "libs/MsgQueue.h"

pthread_t* cars;
pthread_mutex_t fastmutex = PTHREAD_MUTEX_INITIALIZER;
sem_t* mutex;
int racers = 0, * box;
int pipe_fd;
Team *team;

extern void log_message();

// I have no idea if any of the code I am writing here will even work...
void race(Car *me, int id) {
    // TODO Receive breakdown messages
    // msg ohno;
    // msgrcv(mqid, &ohno, msglen, *FIXME*, IPC_NOWAIT); // TODO Find way to uniquely identify a car across all teams 

    if (me->fuel == 0 && !(me->state == QUIT)) {
        me->state = QUIT;
        // TODO Update team and make car stop race
        pause(); // Maybe just pause the thread?
    }

    if (me->state == RACE && (me->fuel) < 4*(me->consumption)*(configs.lapDistance)/(me->speed)) {
        // TODO Try to get in box
        if (team->box == FREE) team->box == RESERVED;
    }

    if (me->state == RACE && (me->fuel) < 2*(me->consumption)*(configs.lapDistance)/(me->speed)) {
        me->state = SAFETY;
    }

    switch (me->state) {
        case RACE:
            me->fuel -= me->consumption;
            if (me->position + me->speed) me->laps += 1;
            me->position += me->speed;
            me->position %= configs.lapDistance;
            break;
        case SAFETY:
            me->fuel -= 0.4*(me->consumption);
            if (me->position + 0.3 * (me->speed) > configs.lapDistance) {
                me->laps += 1;
                // TODO think about creating mutex to check box state here
                pthread_mutex_lock(&fastmutex);
                if (team->box == RESERVED) {
                    me->position = 0;
                    me->state = BOX;
                    team->box == OCCUPPIED;
                }
                pthread_mutex_unlock(&fastmutex);
            }
            me->position += 0.3 * me->speed;
            me->position %= configs.lapDistance;
            break;
    }
    sleep(configs.timeUnit);
}

// Car threads live here
void* vroom(void* r_id) {
    int id = *(int*)r_id;
    free(r_id);
    char buff[64];
    snprintf(buff, sizeof(buff) - 1, "[Team Manager #%d] Car thread #%d created", team->id, id);
    log_message(buff);
    Car *me = &(shm->teams[team->id-1]->cars[id-1]);
    me->fuel = configs.capacity;
    me->position = 0;
    snprintf(buff, sizeof(buff) - 1, "[Team Manager #%d] Car #%d topped up", team->id, id);
    log_message(buff);
    sleep(2); // TODO Don't forget to get rid of this sleep
    // Race function (race())
    snprintf(buff, sizeof(buff) - 1, "[Team Manager #%d] Car thread #%d exiting", team->id, id);
    
    log_message(buff);

    pthread_exit(0);
}

// Create a car thread if conditions match
void spawn_car() {
    if (racers < configs.maxCars) {
        int* id;
        if (!(id = malloc(sizeof(int)))) {
            log_message("malloc fail");
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
        snprintf(buff, sizeof(buff) - 1, "[Team Manager #%d] Joined thread #%d", team->id, i + 1);
        log_message(buff);
    }

}

// TODO create signal handler

// Team Manager process lives here
void team_execute() {
    char buff[64];
    for (int i = 0; i < 2; i++)
        spawn_car();

    join_threads();

    snprintf(buff, sizeof(buff) - 1, "[Team Manager #%d] Process exiting", team->id);
    log_message(buff);
    close(pipe_fd);
    pthread_mutex_destroy(&fastmutex);
    exit(0);
}

// Setup Team Manager
void team_init(int id, int pipe) {
    pipe_fd = pipe;
    team = shm->teams[id-1];
    team->id = id;
    team->box = FREE;
    write(pipe_fd, "FUCK", strlen("FUCK")+1);
    char buff[128];
    snprintf(buff, sizeof(buff) - 1, "[Team Manager #%d] Process spawned", team->id);
    log_message(buff);
    
    mutex = sem_open("MUTEX", 0);
    cars = malloc(sizeof(pthread_t) * configs.maxCars);
    // box = shm->boxes + (team->id - 1);

    team_execute();
}
