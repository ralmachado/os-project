#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

#include "team.h"

#define DEBUG 0
#define BUFFSIZE 128

typedef struct mem_struct {
    int timeUnit, lapDistance, lapCount, noTeams, tBreakdown, tBoxMin, tBoxMax, capacity;
} sharedmem;

sharedmem* configs = NULL;
int shm_key;

FILE* log_file;

int childs = 0;

void init_shm();
void read_conf(char* filename);
void log_message(char* message);
void terminate(int code);

/* 
    ---- TODO ----

    1. Criar processo "Gestor de Corrida", "Gestor de Equipa" e "Gestor de Avarias"
    2. O "Gestor de Equipa" deve criar as threads "Carro"
*/

int main(void) {
    if ((log_file = fopen("log.txt", "w+")) == NULL) {
        perror("Failed to open log file");
        exit(1);
    }

    log_message("Hello\n");

    init_shm(configs);

    read_conf("config.txt");

    if (fork() == 0) {
        team_init();
    } else childs++;

    for (int i = 0; i < childs; i++) {
        wait(NULL);
        childs--;
    }

    terminate(0);
    return 0;
}

void init_shm() {
    shm_key = shmget(IPC_PRIVATE, sizeof(sharedmem), IPC_EXCL|IPC_CREAT|0766);
    if (shm_key == -1) {
        perror("Failed to get shared memory key");
        exit(1);
    } else log_message("Created shared memory segment\n");

    configs = (sharedmem *) shmat(shm_key, NULL, 0);
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

    char buff[BUFFSIZE], *token;
    
    if (fgets(buff, sizeof(buff), config)) {
        if ((configs->timeUnit = atoi(buff)) == 0) {
            fprintf(stderr, "timeUnit null\n");
            exit(1);
        }
    } else {
        fprintf(stderr, "Missing configuration arguments\n");
        fclose(config);
        terminate(1);
    }

    if (fgets(buff, sizeof(buff), config)) {
        token = strtok(buff, ",");
        if (token == NULL || (configs->lapDistance = atoi(token)) == 0) {
            fprintf(stderr, "lapDistance null\n");
            fclose(config);
            terminate(1);
        }
        token = strtok(NULL, ",");
        if (token == NULL || (configs->lapCount = atoi(token)) == 0) {
            fprintf(stderr, "lapCount null\n");
            fclose(config);
            terminate(1);
        }
    }

    if (fgets(buff, sizeof(buff), config)) {
        if (!(configs->noTeams = atoi(buff))) {
            fprintf(stderr, "noTeams null\n");
            fclose(config);
            terminate(1);
        }
    } else {
        fprintf(stderr, "Missing configuration arguments\n");
        fclose(config);
        terminate(1);
    }

    if (fgets(buff, sizeof(buff), config)) {
        if (!(configs->tBreakdown = atoi(buff))) {
            fprintf(stderr, "tBreakdown null\n");
            fclose(config);
            terminate(1);
        }
    } else {
        fprintf(stderr, "Missing configuration arguments\n");
        fclose(config);
        terminate(1);
    }
    
    if (fgets(buff, sizeof(buff), config)) {
        token = strtok(buff, ",");
        if (token == NULL || !(configs->tBoxMin = atoi(token))) {
            fprintf(stderr, "tBoxMin null\n");
            fclose(config);
            terminate(1);
        }
        token = strtok(NULL, "\n");
        if (token == NULL || !(configs->tBoxMax = atoi(token))) {
            fprintf(stderr, "tBoxMax null\n");
            fclose(config);
            terminate(1);
        }
    }

    if (fgets(buff, sizeof(buff), config)) {
        if (!(configs->capacity = atoi(buff))) {
            fprintf(stderr, "capacity null\n");
            fclose(config);
            terminate(1);
        }
    } else {
        fprintf(stderr, "Missing configuration arguments\n");
        fclose(config);
        terminate(1);
    }

    #if DEBUG
        printf(">> timeUnit = %d\n", configs->timeUnit);
        printf(">> lapDistance = %d\n", configs->lapDistance);
        printf(">> lapCount = %d\n", configs->lapCount);
        printf(">> noTeams = %d\n", configs->noTeams);
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
    printf("%s %s", time_s, message);
}

void terminate(int code) {
    if (configs) {
        shmctl(shm_key, IPC_RMID, NULL);
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