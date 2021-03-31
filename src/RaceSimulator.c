#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

#include "libs/RaceManager.h"
#include "libs/TeamManager.h"
#include "libs/BreakdownManager.h"
#include "libs/SharedMem.h"

#define DEBUG 0
#define BUFFSIZE 128

FILE* log_file;

void init_shm();
void read_conf(char* filename);
void init_proc(void (*function)(), void* arg);
void log_message(char* message);
void wait_childs(int n);
void terminate(int code);

int main(void) {
    if ((log_file = fopen("log.txt", "w+")) == NULL) {
        perror("Failed to open log file");
        exit(1);
    }

    log_message("Hello\n");

    init_shm(configs);

    read_conf("config.txt");

    init_proc(race_manager, NULL);
    init_proc(breakdown_manager, NULL);

    wait_childs(2);
    
    terminate(0);
    return 0;
}

void wait_childs(int n) {
    for (int i = 0; i < n; i++) wait(NULL);
}

void init_proc(void (*function)(), void* arg) {
    if (function == NULL) return;
    if (fork() == 0) {
        if (arg) function(arg);
        else function();
    }
}

void init_shm() {
    configs_key = shmget(IPC_PRIVATE, sizeof(sharedmem), IPC_EXCL|IPC_CREAT|0766);
    if (configs_key == -1) {
        perror("Failed to get shared memory key");
        exit(1);
    } else log_message("Created shared memory segment\n");

    configs = (sharedmem *) shmat(configs_key, NULL, 0);
    if (configs == NULL) {
        perror("Failed to attach to shared memory segment");
        exit(1);
    } else log_message("Attached to shared memory segment\n");
}

void read_conf(char* filename) {
    FILE* config;
    if ((config = fopen(filename, "r")) == NULL) {
        perror("Failed to open config file");
        terminate(1);
    }

    int in1, in2;
    
    fscanf(config, "%d\n", &in1);
    if (in1 <= 0) {
        fprintf(stderr, "timeUnit null\n");
        fclose(config);
        terminate(1);
    } else configs->timeUnit = in1;

    fscanf(config, "%d, %d\n", &in1, &in2);
    if (in1 <= 0) {
        log_message("lapDistance is not a positive integer\n");
        fclose(config);
        terminate(1);
    } else if (in2 <= 0) {
        log_message("lapCount is not a positive integer\n");
        fclose(config);
        terminate(1);
    } else {
        configs->lapDistance = in1;
        configs->lapCount = in2;
    }

    fscanf(config, "%d\n", &in1);
    if (in1 < 3) {
        log_message("Not enough teams to setup a race, minimum is 3\n");
        fclose(config);
        terminate(1);
    } else configs->noTeams = in1;

    fscanf(config, "%d\n", &in1);
    if (in1 <= 0) {
        log_message("maxCars is not a positive integer\n");
        fclose(config);
        terminate(1);
    } else configs->maxCars = in1;

    fscanf(config, "%d\n", &in1);
    if (in1 <= 0) {
        log_message("tBreakdown is not a positive integer\n");
        fclose(config);
        terminate(1);
    } else configs->tBreakdown = in1;

    fscanf(config, "%d, %d\n", &in1, &in2);
    if (in1 <= 0) {
        log_message("tBoxMin is not a positive integer\n");
        fclose(config);
        terminate(1);
    } else if (in2 < in1) { 
        log_message("tBoxMax is not greater than tBoxMin\n");
        fclose(config);
        terminate(1);
    } else  {
        configs->tBoxMin = in1;
        configs->tBoxMax = in2;
    }
    
    fscanf(config, "%d\n", &in1);
    if (in1 <= 0) {
        log_message("capacity is not a positive integer\n");
        fclose(config);
        terminate(1);
    } else configs->capacity = in1;


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
    log_message("Configuration read\n");
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

void terminate(int code) {
    if (configs) {
        shmctl(configs_key, IPC_RMID, NULL);
        log_message("Shared memory segment removed\n");
        configs = NULL;
    }

    if (log_file) {
        log_message("Goodbye\n");
        fclose(log_file);
        log_file = NULL;
    }
    exit(code);
}