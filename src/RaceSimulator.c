// David Valente Pereira Barros Leitão - 2019223148
// Rodrigo Alexandre da Mota Machado - 2019218299

// Race Simulator process functions

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "libs/RaceManager.h"
#include "libs/TeamManager.h"
#include "libs/BreakdownManager.h"
#include "libs/SharedMem.h"

#define DEBUG 0
#define BUFFSIZE 128
#define PIPE_NAME "manager"

FILE* log_file;
sem_t* mutex;

void init_log();
void init_npipe();
void init_proc(void (*function)(), void* arg);
void init_sem();
void init_shm();
void log_message(char* message);
void read_conf(char* filename);
void terminate(int code);
void wait_childs(int n);

void sigint() {
  signal(SIGINT, SIG_IGN);
  puts("");
  terminate(1);
}

int main(void) {
    signal(SIGINT, sigint);
    init_sem();
    init_log();
    log_message("[Race Simulator] Hello\n");

    init_shm(configs);
    init_npipe();

    read_conf("config.txt");

    // Temp fix
    signal(SIGINT, SIG_IGN);
    init_proc(race_manager, NULL);
    init_proc(breakdown_manager, NULL);
    // Temp fix
    signal(SIGINT, sigint);

    wait_childs(2);

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
void wait_childs(int n) {
    for (int i = 0; i < n; i++) wait(NULL);
}

// Cleanup shared memory segments and close opened streams before exiting
void terminate(int code) {
    if (configs) {
        shmctl(configs_key, IPC_RMID, NULL);
        log_message("[Race Simulator] Configs shared memory destroyed\n");
        configs = NULL;
    }

    if (unlink(PIPE_NAME) && errno != ENOENT) {
      log_message("[Race Simulator] Couldn't close the named pipe");
    }

    if (log_file) {
        log_message("[Race Simulator] Goodbye\n");
        fclose(log_file);
        log_file = NULL;
    }

    sem_close(mutex);
    sem_unlink("MUTEX");

    exit(code);
}

void init_sem() {
    sem_unlink("MUTEX");
    mutex = sem_open("MUTEX", O_CREAT|O_EXCL, 0766, 1);
}

/* ----- Shared Memory Initialization (Configs) ----- */

void init_shm() {
    configs_key = shmget(IPC_PRIVATE, sizeof(sharedmem), IPC_EXCL|IPC_CREAT|0766);
    if (configs_key == -1) {
        log_message("[Race Simulator] Failed to get configs shared memory key\n");
        exit(1);
    } else log_message("[Race Simulator] Configs shared memory created\n");

    configs = (sharedmem *) shmat(configs_key, NULL, 0);
    if (configs == NULL) {
        log_message("[Race Simulator] Failed to attach to configs shared memory\n");
        exit(1);
    } else log_message("[Race Simulator] Configs shared memory attached\n");
}

// Parse config file
void read_conf(char* filename) {
    FILE* config;
    if ((config = fopen(filename, "r")) == NULL) {
        log_message("[Race Simulator] Failed to open config file\n");
        terminate(1);
    }

        int in1, in2;
    char buff[64], * token;

    if (fgets(buff, sizeof(buff) - 1, config)) {
        in1 = atoi(buff);
        if (in1 <= 0) {
            fprintf(stderr, "timeUnit null\n");
            fclose(config);
            return;
        } else configs->timeUnit = in1;
        in1 = in2 = 0;
    } else {
        fprintf(stderr, "Invalid configuration file");
        fclose(config);
        return;
    }

    if (fgets(buff, sizeof(buff) - 1, config)) {
        token = strtok(buff, ", ");
        in1 = atoi(token);
        token = strtok(NULL, "\n");
        in2 = atoi(token);
        if (in1 <= 0) {
            printf("lapDistance is not a positive integer\n");
            fclose(config);
            return;
        } else if (in2 <= 0) {
            printf("lapCount is not a positive integer\n");
            fclose(config);
            return;
        } else {
            configs->lapDistance = in1;
            configs->lapCount = in2;
        }
        in1 = in2 = 0;
    } else {
        fprintf(stderr, "Invalid configuration file");
        fclose(config);
        return;
    }


    if (fgets(buff, sizeof(buff) - 1, config)) {
        in1 = atoi(buff);
        if (in1 < 3) {
            printf("Not enough teams to setup a race, minimum is 3\n");
            fclose(config);
            return;
        } else configs->noTeams = in1;
        in1 = in2 = 0;
    } else {
        fprintf(stderr, "Invalid configuration file");
        fclose(config);
        return;
    }

    if (fgets(buff, sizeof(buff) - 1, config)) {
        in1 = atoi(buff);
        if (in1 <= 0) {
            printf("maxCars is not a positive integer\n");
            fclose(config);
            return;
        } else configs->maxCars = in1;
        in1 = in2 = 0;
    } else {
        fprintf(stderr, "Invalid configuration file");
        fclose(config);
        return;
    }

    if (fgets(buff, sizeof(buff) - 1, config)) {
        in1 = atoi(buff);
        if (in1 <= 0) {
            printf("tBreakdown is not a positive integer\n");
            fclose(config);
            return;
        } else configs->tBreakdown = in1;
        in1 = in2 = 0;
    } else {
        fprintf(stderr, "Invalid configuration file");
        fclose(config);
        return;
    }

    if (fgets(buff, sizeof(buff) - 1, config)) {
        token = strtok(buff, ", ");
        in1 = atoi(token);
        token = strtok(NULL, "\n");
        in2 = atoi(token);
        if (in1 <= 0) {
            printf("tBoxMin is not a positive integer\n");
            fclose(config);
            return;
        } else if (in2 < in1) {
            printf("tBoxMax is not greater than tBoxMin\n");
            fclose(config);
            return;
        } else {
            configs->tBoxMin = in1;
            configs->tBoxMax = in2;
        }
        in1 = in2 = 0;
    } else {
        fprintf(stderr, "Invalid configuration file");
        fclose(config);
        return;
    }

    if (fgets(buff, sizeof(buff) - 1, config)) {
        in1 = atoi(buff);
        if (in1 <= 0) {
            printf("capacity is not a positive integer\n");
            fclose(config);
            return;
        } else configs->capacity = in1;
        in1 = in2 = 0;
    } else {
        fprintf(stderr, "Invalid configuration file\n");
        fclose(config);
        return;
    }

    #if DEBUG
        printf(">> timeUnit = %d\n", configs->timeUnit);
        printf(">> lapDistance = %d\n", configs->lapDistance);
        printf(">> lapCount = %d\n", configs->lapCount);
        printf(">> noTeams = %d\n", configs->noTeams);
        printf(">> maxCars = %d\n", configs->maxCars);
        printf(">> tBreakdown = %d\n", configs->tBreakdown);
        printf(">> tBoxMin = %d\n", configs->tBoxMin);
        printf(">> tBoxMax = %d\n", configs->tBoxMax);
        printf(">> capacity = %d\n", configs->capacity);
    #endif

    fclose(config);
    log_message("[Race Simulator] Configuration read\n");
}

/* ----- Named Pipe ----- */

void init_npipe() {
  if (mkfifo(PIPE_NAME, O_CREAT|O_EXCL|0600)) {
    log_message("[Race Simulator] Failed to create named pipe");
    terminate(1);
  }
}

/* ----- Logging Functions -----*/

void init_log() {
    if ((log_file = fopen("log.txt", "w+")) == NULL) {
        log_message("[Race Simulator] Failed to open log file\n");
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
    fprintf(log_file, "%s %s", time_s, message);
    fflush(log_file);
    printf("%s %s", time_s, message);
}
