// David Valente Pereira Barros Leitão - 2019223148
// Rodrigo Alexandre da Mota Machado - 2019218299

// Team Manager process functions

#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/msg.h>

#include "libs/header.h"
#include "libs/message.h"

pthread_t* cars, box_t;
// TODO Consider using robust mutexes for box_state and repair_mutex
pthread_mutex_t box_state = PTHREAD_MUTEX_INITIALIZER, *race_mutex;
pthread_cond_t repair_cv = PTHREAD_COND_INITIALIZER, *race_cv;
pthread_mutex_t repair_mutex= PTHREAD_MUTEX_INITIALIZER;
sem_t box_worker;
int racers = 0, * box, *ids;
int pipe_fd;
int in_box = -1;
Team *team;
char magic[BUFFSIZE] = "RACE FINISH";

extern void log_message();

void notify_end() {
    write(pipe_fd, magic, sizeof(magic));
    shm->race_status = END;
}

// I have no idea if any of the code I am writing here will even work...
void race(Car *me, int id) {
    // Receive breakdown messages
    char buff[BUFFSIZE];
    msg ohno;
    while (1) {
        if (shm->show_stats) {
            --(shm->stats.racing);
            // If last car to stop for stats, signal stat function to work
            if (shm->stats.racing == 0) pthread_cond_signal(&shm->stats_cv);
            pthread_mutex_lock(&shm->stats_mutex);
            // Wait for stat function end
            while (shm->show_stats) pthread_cond_wait(&shm->stats_cv, &shm->stats_mutex);
            ++(shm->stats.racing);
            pthread_mutex_unlock(&shm->stats_mutex);
        }
        size_t msgsize = msgrcv(mqid, &ohno, msglen, me->number, IPC_NOWAIT);
        if (msgsize == msglen) {
            snprintf(buff, sizeof(buff), "[Car %d] Malfunctioning, entered safety mode\n", me->number);
            write(pipe_fd, buff, sizeof(buff));
            me->state = SAFETY;
            me->malfunction = true;
            pthread_mutex_lock(&box_state);
            if (team->box == FREE) team->box = RESERVED;
            pthread_mutex_unlock(&box_state);
        }

        if (me->fuel == 0 && !(me->state == QUIT)) {
            snprintf(buff, sizeof(buff), "[Car %d] Ran out of fuel, quit from race\n", me->number);
            log_message(buff);
            me->state = QUIT;
            return;
        }

        if (me->state == RACE && (me->fuel) < 2*(me->consumption)*(configs.lapDistance)/(me->speed)) {
            snprintf(buff, sizeof(buff), "[Car %d] Extremely low on fuel, entered safety mode\n", me->number);
            write(pipe_fd, buff, sizeof(buff));
            me->state = SAFETY; 
        }
        
        if (!(me->lowFuel) && me->state == RACE && (me->fuel) < 4*(me->consumption)*(configs.lapDistance)/(me->speed)) {
            snprintf(buff, sizeof(buff), "[Car %d] Low on fuel, attempting to get box\n", me->number);
            write(pipe_fd, buff, sizeof(buff));
            me->lowFuel = true;
        }

        pthread_mutex_lock(&box_state);
        if (me->state == SAFETY && team->box == FREE) team->box = RESERVED;
        pthread_mutex_unlock(&box_state);

        switch (me->state) {
            case RACE:
                me->fuel -= me->consumption;
                if (me->position + me->speed >= configs.lapDistance) {
                    me->laps++;
                    if (me->laps < configs.lapCount) {
                        snprintf(buff, sizeof(buff), "[Car %d] Crossed the finish line, laps left: %d\n"
                                , me->number, configs.lapCount - me->laps);
                        write(pipe_fd, buff, sizeof(buff));
                        if (shm->race_int || shm->race_usr1) {
                            if (--(shm->stats.racing) == 0) notify_end();
                            return;
                        }
                    } else {
                        me->state = FINISH;    
                        me->finish = shm->pos++;        
                        snprintf(buff, sizeof(buff), "[Car %d] Finished race\n", me->number);
                        write(pipe_fd, buff, sizeof(buff));
                        if (--(shm->stats.racing) == 0) notify_end();
                        return;
                    }
                    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL); // IDEA Use this to stop race in the finishing line
                    if (me->lowFuel && team->box == FREE) { 
                        pthread_mutex_lock(&box_state);
                        if (team->box == FREE) {
                            snprintf(buff, sizeof(buff), "[Car %d] Entering the box\n", me->number);
                            write(pipe_fd, buff, sizeof(buff)); 
                            me->position = 0;
                            me->state = BOX;
                            team->box = OCCUPPIED;
                            in_box = id;
                            sem_post(&box_worker);
                            pthread_mutex_unlock(&box_state);
                            pthread_mutex_lock(&repair_mutex);
                            while (in_box != -1)
                                pthread_cond_wait(&repair_cv, &repair_mutex);
                            pthread_mutex_unlock(&repair_mutex);
                            me->state = RACE;
                            snprintf(buff, sizeof(buff), "[Car %d] Left the box\n", me->number);
                            write(pipe_fd, buff, sizeof(buff)); 
                        }
                        pthread_mutex_unlock(&box_state);
                    }
                    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL); // IDEA Use this to stop race in the finishing line
                }
                me->position += me->speed;
                me->position %= configs.lapDistance;
                // snprintf(buff, sizeof(buff), "[Car %d] Position: %d | Lap: %d\n", me->number, me->position, me->laps);
                // write(pipe_fd, buff, sizeof(buff));
                break;
            case SAFETY:
                me->fuel -= 0.4*(me->consumption);
                // If crossing starting point and still racing
                if (me->position + 0.3 * (me->speed) >= configs.lapDistance) {
                    me->laps++;
                    if (me->laps < configs.lapCount) {
                        snprintf(buff, sizeof(buff), "[Car %d] Crossed the finish line, laps left: %d\n"
                                , me->number, configs.lapCount - me->laps);
                        write(pipe_fd, buff, sizeof(buff));
                        if (shm->race_int || shm->race_usr1) {
                            if (--(shm->stats.racing) == 0) notify_end();
                            return;
                        }
                    } else {
                        me->state = FINISH;
                        me->finish = shm->pos++;
                        shm->stats.racing--;
                        snprintf(buff, sizeof(buff), "[Car %d] Finished race\n", me->number);
                        write(pipe_fd, buff, sizeof(buff));
                        if (shm->stats.racing == 0) notify_end();
                        return;
                    }
                    pthread_mutex_lock(&box_state);
                    if (team->box == RESERVED) {
                        snprintf(buff, sizeof(buff), "[Car %d] Entering the box\n", me->number);
                        write(pipe_fd, buff, sizeof(buff)); 
                        me->position = 0;
                        me->state = BOX;
                        team->box = OCCUPPIED;
                        in_box = id;
                        sem_post(&box_worker);
                        pthread_mutex_unlock(&box_state);
                        pthread_mutex_lock(&repair_mutex);
                        while (in_box != -1)
                            pthread_cond_wait(&repair_cv, &repair_mutex);
                        pthread_mutex_unlock(&repair_mutex);
                        me->state = RACE;
                        snprintf(buff, sizeof(buff), "[Car %d] left the box\n", me->number);
                        write(pipe_fd, buff, sizeof(buff));
                    }
                    pthread_mutex_unlock(&box_state);
                }
                me->position += 0.3 * me->speed;
                me->position %= configs.lapDistance;
                // snprintf(buff, sizeof(buff), "[Car %d] Position: %d | Lap: %d\n", me->number, me->position, me->laps);
                // write(pipe_fd, buff, sizeof(buff));
                break;
        }

        sleep(configs.timeUnit);
    }
}

// Car threads live here
void* vroom(void* t_id) {
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    int id = *((int*)t_id);
    char buff[64];
    snprintf(buff, sizeof(buff) - 1, "[Team Manager #%d] Car thread #%d created", team->id, id);
    log_message(buff);
    Car *me = &(team->cars[id]);
    
    while (1) {
        // Wait for race start
        pthread_mutex_lock(race_mutex);
        while (me->number == -1 || shm->race_status != ONGOING)
            pthread_cond_wait(race_cv, race_mutex);
        pthread_mutex_unlock(race_mutex);
        
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        me->fuel = configs.capacity;
        me->stops = 0;
        me->laps = 0;
        me->position = 0;
        me->lowFuel = false;
        me->malfunction = false;
        me->state = RACE;
        shm->stats.racing++;
        snprintf(buff, sizeof(buff), "[Car %d] Started racing", me->number);
        write(pipe_fd, buff, sizeof(buff));
        race(me, id);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_mutex_lock(race_mutex);
        while (shm->race_status == ONGOING)
            pthread_cond_wait(race_cv, race_mutex);
        pthread_mutex_unlock(race_mutex);
    }
    
    // snprintf(buff, sizeof(buff) - 1, "[Team Manager #%d] Car thread #%d exiting", team->id, id);
    // log_message(buff);
    
    // pthread_exit(0);
}

void* car_box() {
    char buff[BUFFSIZE];
    snprintf(buff, sizeof(buff), "[Team Manager #%d] Box is open", team->id);
    log_message(buff);
    
    srand(pthread_self());
    while (1) {
        sem_wait(&box_worker);
        Car *boxed = &(team->cars[in_box]);
        boxed->stops++;
        log_message(buff);

        if (boxed->lowFuel) {
            sleep(2*configs.timeUnit);
            boxed->fuel = configs.capacity; 
            boxed->lowFuel = false;
            shm->stats.topup++;
        }

        if (boxed->malfunction) {
            sleep(rand() % (configs.tBoxMax-configs.tBoxMin+1) + configs.tBoxMin);
            boxed->malfunction = false;
            shm->stats.malfunctions++;
        }

        in_box = -1;
        pthread_cond_signal(&repair_cv);
    }
}

// Create a car thread if conditions match
void spawn_car() {
    if (racers < configs.maxCars) {
        pthread_create(&cars[racers], NULL, vroom, ids+racers);
        racers++;
    }
}

// Await for all car threads to exit
void join_threads() {
    pthread_mutex_lock(race_mutex);
    while (shm->race_status == ONGOING)
        pthread_cond_wait(race_cv, race_mutex);
    pthread_mutex_unlock(race_mutex);
    char buff[64];
    if (pthread_cancel(box_t))
        snprintf(buff, sizeof(buff), "[Team Manager #%d] Failed to cancel Box thread", team->id);
    else snprintf(buff, sizeof(buff), "[Team Manager #%d] Cancelled Box thread", team->id);
    log_message(buff);
    if (pthread_join(box_t, NULL))
        snprintf(buff, sizeof(buff), "[Team Manager #%d Failed to join Box thread", team->id);
    else snprintf(buff, sizeof(buff), "[Team Manager #%d] Joined Box thread", team->id);
    log_message(buff);
    for (int i = 0; i < racers; i++) {
        if (pthread_cancel(cars[i]))
            snprintf(buff, sizeof(buff), "[Team Manager #%d] Failed to cancel thread #%d", team->id, i + 1);
        else snprintf(buff, sizeof(buff), "[Team Manager #%d] Cancelled thread #%d", team->id, i + 1);
        log_message(buff);
        if (pthread_join(cars[i], NULL))
            snprintf(buff, sizeof(buff), "[Team Manager #%d] Failed to join thread #%d", team->id, i + 1);
        else snprintf(buff, sizeof(buff), "[Team Manager #%d] Joined thread #%d", team->id, i + 1);
        log_message(buff);
    }
}

void team_destroy() {
    char buff[BUFFSIZE];

    if (cars) free(cars);

    if (close(pipe_fd)) {
        snprintf(buff, sizeof(buff), "[Team Manager #%d] Failed to close unnamed pipe", team->id);
        log_message(buff);
    } else {
        snprintf(buff, sizeof(buff), "[Team Manager #%d] Closed unnamed pipe", team->id);
        log_message(buff);
    }

    if (pthread_mutex_destroy(&box_state)) {
        snprintf(buff, sizeof(buff), 
            "[Team Manager #%d] Failed to destroy mutex 'box_state'", team->id);
        log_message(buff);
    } else {
        snprintf(buff, sizeof(buff), 
            "[Team Manager #%d] Destroyed mutex 'box_state'", team->id);
        log_message(buff);
    }
    
    if (pthread_mutex_destroy(&repair_mutex)) {
        snprintf(buff, sizeof(buff), 
            "[Team Manager #%d] Failed to destroy mutex 'repair_mutex'", team->id);
        log_message(buff);
    } else {
        snprintf(buff, sizeof(buff), 
            "[Team Manager #%d] Destroyed mutex 'repair_mutex'", team->id);
        log_message(buff);
    }
    
    if (pthread_cond_destroy(&repair_cv)) {
        snprintf(buff, sizeof(buff), 
            "[Team Manager #%d] Failed to destroy condition variable 'repair_cv'", team->id);
        log_message(buff);
    } else {
        snprintf(buff, sizeof(buff), 
            "[Team Manager #%d] Destroyed condition variable 'repair_cv'", team->id);
        log_message(buff);
    }

    if (sem_destroy(&box_worker)) {
        snprintf(buff, sizeof(buff), "[Team Manager #%d] Failed to destroy 'box_worker' semaphore", team->id);
        log_message(buff);
    }
}

void team_exit(int signo) {
    char buff[BUFFSIZE];
    if (signo == SIGINT) {
        snprintf(buff, sizeof(buff), "[Team Manager #%d] Received SIGINT", team->id);
        log_message(buff);
        join_threads();
        team_destroy();
        snprintf(buff, sizeof(buff), "[Team Manager #%d] Process exiting", team->id);
        log_message(buff);
    }
    exit(0);
}

// Team Manager process lives here
void team_execute() {
    ids = calloc(configs.maxCars, sizeof(int));
        
    // Criar maxCars threads -> Se carro não inicializado, thread idles
    for (int i = 0; i < configs.maxCars; i++) {
        ids[i] = i;
        spawn_car();
    }
    

    pthread_create(&box_t, NULL, car_box, NULL);
    
    pause();
}

// Setup Team Manager
void team_init(int id, int pipe) {
    // printf("Team #%d | pid: %d\n", id, getpid());
    struct sigaction sigint;    
    sigint.sa_handler = team_exit;
    sigemptyset(&sigint.sa_mask);
    sigint.sa_flags = 0;
    sigaction(SIGINT, &sigint, NULL);

    if (pipe != -1) pipe_fd = pipe;
    team = &(shm->teams[id-1]);
    team->id = id;
    team->box = FREE;
    race_mutex = &(shm->race_mutex);
    race_cv = &(shm->race_cv);
    char buff[128];
    snprintf(buff, sizeof(buff) - 1, "[Team Manager #%d] Process spawned", team->id);
    log_message(buff);
    
    cars = malloc(sizeof(pthread_t) * configs.maxCars);

    sem_init(&box_worker, 0, 0);
    
    team_execute();
}
