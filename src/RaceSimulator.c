// David Valente Pereira Barros Leitão - 2019223148
// Rodrigo Alexandre da Mota Machado - 2019218299

// Race Simulator process functions

#include <errno.h>
#include <fcntl.h>
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

#include "libs/BreakdownManager.h"
#include "libs/MsgQueue.h"
#include "libs/RaceManager.h"
#include "libs/SharedMem.h"
#include "libs/TeamManager.h"

#define DEBUG 0
#define PIPE_NAME "manager"

int *shmids;
FILE* log_file;
sem_t* mutex;
sigset_t block;

void get_statistics();
void init_log();
void init_mq();
void init_npipe();
void init_proc(void (*function)(), void* arg);
int init_sem();
int init_shm();
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
    // FIXME SIGTSTP Is calling SIGINT?
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
    kill(0, SIGINT);
    if (shm->race_status == RACE) shm->race_int = true;
    while (wait(NULL) != -1);

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

/* ----- Logging Functions -----*/

void init_log() {
    if ((log_file = fopen("log.txt", "w+")) == NULL) {
        puts("[Race Simulator] Failed to open log file");
        exit(1);
    }
}

void log_message(char* message) {
    sem_wait(mutex);
    // Get current time
    char time_s[10];
    time_t time_1 = time(NULL);
    struct tm *time_2 = localtime(&time_1);
    strftime(time_s, sizeof(time_s), "%H:%M:%S ", time_2);

    // Print and write to log file
    fprintf(log_file, "%s %s\n", time_s, message);
    fflush(log_file);
    printf("%s %s\n", time_s, message);
    fflush(stdout);
    sem_post(mutex);
}

// TODO think about adding a finish time to compare two finished cars
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
    int size = min(5, shm->init_cars);
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
    // TODO Implement statistics
    qsort(stats_arr, shm->init_cars, sizeof(Car*), compare);
    char buff[BUFFSIZE];
    leaderboard();
    snprintf(buff, sizeof(buff), "[Stats] Total Malfunctions: %d", shm->malfunctions);
    log_message(buff);
    snprintf(buff, sizeof(buff), "[Stats] Total Fuel-Ups: %d", shm->topup);
    log_message(buff);
    snprintf(buff, sizeof(buff), "[Stats] Cars on track: %d", shm->on_track);
    log_message(buff);
}
