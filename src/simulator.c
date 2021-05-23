// David Valente Pereira Barros Leitão - 2019223148
// Rodrigo Alexandre da Mota Machado - 2019218299

// Race Simulator process functions

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "libs/header.h"
#include "libs/breakdown.h"
#include "libs/message.h"
#include "libs/race.h"
#include "libs/team.h"

#define DEBUG 0
#define PIPE_NAME "manager"

int *shmids;
FILE* log_file;
sem_t* mutex;
sigset_t block;
pthread_condattr_t shared_cv;
pthread_mutexattr_t shared_mutex;

void get_statistics();
void init_log();
void init_mq();
void init_npipe();
void init_proc(void (*function)(), void* arg);
void init_psync();
int init_sem();
int init_shm();
void psync_destroy();
void log_message(char* message);
extern int read_conf(char* filename);
void terminate(int code);
void wait_childs();

void sigint(int signo) {
    if (signo == SIGINT) terminate(1);
}

void test_mq() {
    msg my_msg;
    my_msg.msgtype = 1;
    msgsnd(mqid, &my_msg, msglen, 0);
}

int main(void) {
    struct sigaction interrupt, statistics;
    
    statistics.sa_handler = get_statistics;
    statistics.sa_flags = SA_NODEFER;
    sigemptyset(&statistics.sa_mask);
    sigaddset(&statistics.sa_mask, SIGINT);
    
    interrupt.sa_handler = sigint;
    interrupt.sa_flags = 0;
    sigemptyset(&interrupt.sa_mask);
    sigaction(SIGINT, &interrupt, NULL);
    
    // Block SIGUSR1 and SIGTSTP
    sigaddset(&block, SIGUSR1);
    sigaddset(&block, SIGTSTP);
    sigprocmask(SIG_BLOCK, &block, NULL);

    if (init_sem()) terminate(1);
    init_log();
    log_message("[Race Simulator] Hello");

    if (read_conf("config.txt")) terminate(1);
    if (init_shm()) terminate(1);
    init_npipe();
    init_mq();
    init_psync();

    sigaddset(&block, SIGINT);
    sigprocmask(SIG_BLOCK, &block, NULL);
    init_proc(race_manager, NULL);
    init_proc(breakdown_manager, NULL);

    sigdelset(&block, SIGINT);
    sigdelset(&block, SIGTSTP);
    sigaction(SIGINT, &interrupt, NULL);
    sigprocmask(SIG_SETMASK, &block, NULL);
    sigaction(SIGTSTP, &statistics, NULL);

    while (1) pause();

    return 0;
}

/* ----- Process Managing ----- */

// Create a process, and execute function on the new process
void init_proc(void (*function)(), void* arg) {
    if (function == NULL) return;
    if (fork() == 0) {
        if (arg) function(arg);
        else function();
    }
}

// Await for n child termination
void wait_childs() {
    while (wait(NULL) != -1);
}

// Cleanup shared memory segments and close opened streams before exiting
void terminate(int code) {
    shm->race_int = true;
    if (shm->race_status == ONGOING) {
        pthread_mutex_lock(&shm->race_mutex);
        while (shm->race_status == ONGOING)
            pthread_cond_wait(&shm->race_cv, &shm->race_mutex);
        pthread_mutex_unlock(&shm->race_mutex);
    }

    pthread_cond_broadcast(&(shm->race_cv));
    kill(0, SIGINT);
    while (wait(NULL) != -1);

    psync_destroy();
    
    if (stats_arr) shmctl(statsid, IPC_RMID, NULL);

    if (shm) {
        for (int i = configs.noTeams+1; i >= 1; i--)
            if (shmctl(shmids[i], IPC_RMID, NULL)) perror("Failed to detroy shared mem segment");
        if (shmctl(shmids[0], IPC_RMID, NULL) == -1)
            log_message("[Race Simulator] Shared memory couldn't be destroyed");
        else log_message("[Race Simulator] Shared memory destroyed");
        shm = NULL;
    }

    if (unlink(PIPE_NAME) == -1 && errno != ENOENT) {
      log_message("[Race Simulator] Couldn't close the named pipe");
    } else log_message("[Race Simulator] Named pipe destroyed");

    if (mqid != -1) {
        if (msgctl(mqid, IPC_RMID, NULL) == -1) 
            log_message("[Race Simulator] Couldn't close the message queue");
        else log_message("[Race Simulator] Message queue destroyed");
    }

    if (log_file) {
        log_message("[Race Simulator] Goodbye");
        fclose(log_file);
        log_file = NULL;
    }

    sem_close(mutex);
    sem_unlink("MUTEX");

    exit(code);
}

int init_sem() {
    sem_unlink("MUTEX");
    mutex = sem_open("MUTEX", O_CREAT|O_EXCL, 0766, 1);
    if (mutex == SEM_FAILED) return 1;
    return 0;
}

/* ----- Shared Memory Initialization (Configs) ----- */

int init_shm() {
    shmids = calloc(2+configs.noTeams, sizeof(int));
    shmids[0] = shmget(IPC_PRIVATE, sizeof(shm), IPC_CREAT|0700);
    if (shmids[0] == 0) {
        log_message("[Race Simulator] Failed to get shared memory key");
        return -1;
    } log_message("[Race Simulator] Shared memory created");

    if ((shm = shmat(shmids[0], NULL, 0)) == (void*) -1) {
        log_message("[Race Simulator] Failed to attach shared memory segment");
        return -1;
    } log_message("[Race Simulator] Shared memory segment attached");
    
    shmids[1] = shmget(IPC_PRIVATE, configs.noTeams*sizeof(Team),IPC_CREAT|0700);
    if (shmids[0] != -1) {
        log_message("[Race Simulator] Teams array created");
        shm->teams = shmat(shmids[1], NULL, 0);
        if (shm->teams) log_message("[Race Simulator] Teams array attached");

        for (int i = 0; i < configs.noTeams; i++) {
            Team *team = &(shm->teams[i]);
            team->init = false;
            shmids[i+2] = shmget(IPC_PRIVATE, configs.maxCars*sizeof(Car), IPC_CREAT|0700);
            team->cars = shmat(shmids[i+2], NULL, 0);
            if (team->cars == NULL) log_message("Failed to allocate space for cars");
            else {
                for (int j = 0; j < configs.maxCars; j++)
                    team->cars[j].number = -1;
            }
        }
    }

    statsid = shmget(IPC_PRIVATE, configs.noTeams*configs.maxCars*sizeof(Car*), IPC_CREAT|0700);
    stats_arr = shmat(statsid, NULL, 0);

    return 0;
}

/* ----- Named Pipe ----- */

void init_npipe() {
    if (mkfifo(PIPE_NAME, O_CREAT|O_EXCL|0700)) {
        log_message("[Race Simulator] Failed to create named pipe");
        terminate(1);
    }

    log_message("[Race Simulator] Created named pipe");
}

/* ----- Message Queue ----- */

void init_mq() {
    if ((mqid = msgget(IPC_PRIVATE, IPC_CREAT|0600)) == -1) {
        log_message("[Race Simulator] Failed to create the message queue");
        terminate(1);
    }

    log_message("[Race Simulator] Created message queue");
    msglen = sizeof(msg)-sizeof(long);
}

/* ----- Pthread Mutexes and Condition Variables ----- */

void init_psync() {
    pthread_condattr_init(&shared_cv);
    pthread_mutexattr_init(&shared_mutex);
    pthread_condattr_setpshared(&shared_cv, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_setpshared(&shared_mutex, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_setrobust(&shared_mutex, PTHREAD_MUTEX_ROBUST);

    if (pthread_cond_init(&shm->race_cv, &shared_cv))
        log_message("[Race Simulator] Failed to initialize 'race_cv'");
    else log_message("[Race Simulator] Initialized 'race_cv'");

    if (pthread_mutex_init(&shm->race_mutex, &shared_mutex))
        log_message("[Race Simulator] Failed to initialize 'race_mutex'");
    else log_message("[Race Simulator] Initialized 'race_mutex'");

    if (pthread_cond_init(&shm->stats_cv, &shared_cv))
        log_message("[Race Simulator] Failed to initialize 'stats_cv'");
    else log_message("[Race Simulator] Initialized 'stats_cv'");

    if (pthread_mutex_init(&shm->stats_mutex, &shared_mutex))
        log_message("[Race Simulator] Failed to initialize 'stats_mutex'");
    else log_message("[Race Simulator] Initialized 'stats_mutex'");
}

void psync_destroy() {
    if (pthread_mutex_destroy(&shm->race_mutex))
        log_message("[Race Simulator] Failed to destroy 'race_mutex'");
    else log_message("[Race Simulator] Destroyed 'race_mutex'");

    if (pthread_mutex_destroy(&shm->stats_mutex))
        log_message("[Race Simulator] Failed to destroy 'stats_mutex'");
    else log_message("[Race Simulator] Destroyed 'stats_mutex'");

    if (pthread_cond_destroy(&shm->stats_cv))
        log_message("[Race Simulator] Failed to destroy 'stats_cv'");
    else log_message("[Race Simulator] Destroyed 'stats_cv'");

    if (pthread_cond_destroy(&shm->race_cv))
        log_message("[Race Simulator] Failed to destroy 'race_cv'");
    else log_message("[Race Simulator] Destroyed 'race_cv'");

    if (pthread_condattr_destroy(&shared_cv))
        log_message("[Race Simulator] Failed to destroy 'shared_cv' condattr");
    else log_message("[Race Simulator] Destroyed 'shared_cv' condattr");

    if (pthread_mutexattr_destroy(&shared_mutex))
        log_message("[Race Simulator] Failed to destroy 'shared_mutex' mutexattr");
    else log_message("[Race Simulator] Destroyed 'shared_mutex' mutexattr");
}

/* ----- Logging Functions -----*/

void init_log() {
    if ((log_file = fopen("log.txt", "w+")) == NULL) {
        puts("[Race Simulator] Failed to open log file");
        exit(1);
    }
}

void log_message(char* message) {
    // Get current time
    char time_s[10];
    time_t time_1 = time(NULL);
    struct tm *time_2 = localtime(&time_1);
    strftime(time_s, sizeof(time_s), "%H:%M:%S ", time_2);

    // Print and write to log file
    sem_wait(mutex);
    fprintf(log_file, "%s %s\n", time_s, message);
    printf("%s %s\n", time_s, message);
    fflush(log_file);
    fflush(stdout);
    sem_post(mutex);
}

/* ----- Race Statistics ----- */

int compare(const void* a, const void* b) {
    Car* carA = (Car *) a;
    Car* carB = (Car *) b;

    if (carA->laps < carB->laps) return 1;
    else if (carA->laps > carB->laps) return -1;
    else {
        if (carA->state == FINISH && carB->state == FINISH) return (carA->finish - carB->finish);
        else if (carA->position < carB->position) return 1;
        else if (carA->position > carB->position) return -1;
        else return 0;
    }
}

void leaderboard() {
    char buff[BUFFSIZE];
    int min;
    if (shm->init_cars > 5) min = 5;
    else min = shm->init_cars;
    Car* curr;
    for (int i = 0; i < min; i++) {
        curr = stats_arr[i];
        if (curr->state != FINISH) {
            snprintf(buff, sizeof(buff), "[Stats] #%d: Car %d | Team: %s | Laps completed: %d | Box stops: %d", 
                i+1, curr->number, curr->team, curr->laps, curr->stops);
        } else {
            snprintf(buff, sizeof(buff), "[Stats] #%d: Car %d | Team: %s | Finished | Box stops: %d", 
                i+1, curr->number, curr->team, curr->stops);
        }
        log_message(buff);
    }

    curr = stats_arr[shm->init_cars-1];
    if (curr->state != FINISH) {
        snprintf(buff, sizeof(buff), "[Stats] Last place: Car %d | Team: %s | Laps completed: %d | Box stops: %d",
            curr->number, curr->team, curr->laps, curr->stops);
    } else {
        snprintf(buff, sizeof(buff), "[Stats] Last place: Car %d | Team: %s | Finished | Box stops: %d", 
            curr->number, curr->team, curr->stops);
    }
    log_message(buff);
}

void get_statistics() {
    shm->show_stats = true;
    pthread_mutex_lock(&shm->stats_mutex);
    while (shm->stats.racing != 0)
        pthread_cond_wait(&shm->stats_cv, &shm->stats_mutex);
    pthread_mutex_unlock(&shm->stats_mutex);
    char buff[BUFFSIZE];
    qsort(stats_arr, shm->init_cars, sizeof(Car*), compare);
    leaderboard();
    snprintf(buff, sizeof(buff), "[Stats] Total Malfunctions: %d", shm->stats.malfunctions);
    log_message(buff);
    snprintf(buff, sizeof(buff), "[Stats] Total Fuel-Ups: %d", shm->stats.topup);
    log_message(buff);
    snprintf(buff, sizeof(buff), "[Stats] Cars on track: %d", shm->stats.on_track);
    log_message(buff);
    shm->show_stats = false;
    pthread_cond_broadcast(&shm->stats_cv);
}
