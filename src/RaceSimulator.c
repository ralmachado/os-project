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

void sigint() {
  signal(SIGINT, SIG_IGN);
  terminate(1);
}

void test_mq() {
    msg my_msg;
    my_msg.msgtype = 1;
    msgsnd(mqid, &my_msg, msglen, 0);
}

int main(void) {
    struct sigaction interrupt, statistics;
    sigaddset(&block, SIGUSR1);
    interrupt.sa_handler = sigint;
    statistics.sa_handler = get_statistics;
    sigemptyset(&statistics.sa_mask);
    statistics.sa_flags = 0;
    sigemptyset(&interrupt.sa_mask);
    interrupt.sa_flags = 0;
    sigaction(SIGINT, &interrupt, NULL);

    if (init_sem()) terminate(1);
    init_log();
    log_message("[Race Simulator] Hello");

    if (read_conf("config.txt")) terminate(1);
    if (init_shm()) terminate(1);
    init_npipe();
    init_mq();


    // Temp fix
    signal(SIGINT, SIG_IGN);
    init_proc(race_manager, NULL);
    init_proc(breakdown_manager, NULL);
    // Temp fix
    signal(SIGINT, sigint);
    sigaction(SIGTSTP, &statistics, NULL);
    sigprocmask(SIG_BLOCK, &block, NULL);

    while(wait(NULL) != -1);

    terminate(0);
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
    while (wait(NULL) != -1);
    if (shm) {
        for (int i = configs.noTeams+1; i >= 0; i--)
            shmctl(shmids[i], IPC_RMID, NULL);
        // if (shm->teams) free(shm->teams);
        if (shmctl(shmid, IPC_RMID, NULL) == -1)
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
    shmid = shmget(IPC_PRIVATE, sizeof(shm), IPC_CREAT|0700);
    if (shmid == -1) {
        log_message("[Race Simulator] Failed to get shared memory key");
        return -1;
    } log_message("[Race Simulator] Shared memory created");

    if ((shm = shmat(shmid, NULL, 0)) == (void*) -1) {
        log_message("[Race Simulator] Failed to attach shared memory segment");
        return -1;
    } log_message("[Race Simulator] Shared memory segment attached");
    
    shmids[0] = shmget(IPC_PRIVATE, configs.noTeams*sizeof(Team),IPC_CREAT|0700);
    if (shmids[0] != -1) {
        log_message("[Race Simulator] Teams array created");
        shm->teams = shmat(shmids[0], NULL, 0);
        if (shm->teams) log_message("[Race Simulator] Teams array attached");

        for (int i = 0; i < configs.noTeams; i++) {
            Team *team = &(shm->teams[i]);
            team->init = false;
            shmids[i+1] = shmget(IPC_PRIVATE, configs.maxCars*sizeof(Car), IPC_CREAT|0700);
            team->cars = shmat(shmids[i+1], NULL, 0);
            if (team->cars == NULL) log_message("Failed to allocate space for cars");
            else {
                for (int j = 0; j < configs.maxCars; j++)
                    team->cars[i].number = -1;
            }
        }
    }

    return 0;
}

/* ----- Named Pipe ----- */

void init_npipe() {
    if (mkfifo(PIPE_NAME, O_CREAT|O_EXCL|0600)) {
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

void get_statistics() {
    // TODO Implement statistics
    // kill(0, SIGUSR1); // Sends SIGUSR1 to all processes and threads to suspend race
}