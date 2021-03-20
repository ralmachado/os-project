#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEBUG 1
#define BUFFSIZE 128

int timeUnit, lapDistance, lapCount, noTeams, tBreakdown, tBoxMin, tBoxMax, capacity;
FILE* log_file;

void read_conf(char*);
void log_message(char*);

/* 
    ---- TODO ----
    1. Criar memória partilhada
    void init_shm(void);
    
    2. Criar processo "Gestor de Corrida", "Gestor de Equipa" e "Gestor de Avarias"
    3. O "Gestor de Equipa" deve criar as threads "Carro"
*/

int main(void) {
    if ((log_file = fopen("log.txt", "w+")) == NULL) {
        perror("Failed to open log file");
        exit(1);
    }

    read_conf("config.txt");
    log_message("I'm so tired, please\n");

    fclose(log_file);
    return 0;
}

void read_conf(char* filename) {
    FILE* config;
    if ((config = fopen(filename, "r")) == NULL) {
        perror("Failed to open config file");
        exit(1);
    }

    char buff[BUFFSIZE], *token;
    
    if (fgets(buff, sizeof(buff), config)) {
        if (!(timeUnit = atoi(buff))) {
            fprintf(stderr, "timeUnit null\n");
            exit(1);
        }
    } else {
        fprintf(stderr, "Missing configuration arguments\n");
        exit(1);
    }

    if (fgets(buff, sizeof(buff), config)) {
        token = strtok(buff, ",");
        if (token == NULL || !(lapDistance = atoi(token))) {
            fprintf(stderr, "lapDistance null\n");
            exit(1);
        }
        token = strtok(NULL, ",");
        if (token == NULL || !(lapCount = atoi(token))) {
            fprintf(stderr, "lapCount null\n");
            exit(1);
        }
    }

    if (fgets(buff, sizeof(buff), config)) {
        if (!(noTeams = atoi(buff))) {
            fprintf(stderr, "noTeams null\n");
            exit(1);
        }
    } else {
        fprintf(stderr, "Missing configuration arguments\n");
        exit(1);
    }

    if (fgets(buff, sizeof(buff), config)) {
        if (!(tBreakdown = atoi(buff))) {
            fprintf(stderr, "tBreakdown null\n");
            exit(1);
        }
    } else {
        fprintf(stderr, "Missing configuration arguments\n");
        exit(1);
    }
    
    if (fgets(buff, sizeof(buff), config)) {
        token = strtok(buff, ",");
        if (token == NULL || !(tBoxMin = atoi(token))) {
            fprintf(stderr, "tBoxMin null\n");
            exit(1);
        }
        token = strtok(NULL, "\n");
        if (token == NULL || !(tBoxMax = atoi(token))) {
            fprintf(stderr, "tBoxMax null\n");
            exit(1);
        }
    }

    if (fgets(buff, sizeof(buff), config)) {
        if (!(capacity = atoi(buff))) {
            fprintf(stderr, "capacity null\n");
            exit(1);
        }
    } else {
        fprintf(stderr, "Missing configuration arguments\n");
        exit(1);
    }

    #if DEBUG
        printf(">> timeUnit = %d\n", timeUnit);
        printf(">> lapDistance = %d\n", lapDistance);
        printf(">> lapCount = %d\n", lapCount);
        printf(">> noTeams = %d\n", noTeams);
        printf(">> tBreakdown = %d\n", tBreakdown);
        printf(">> tBoxMin = %d\n", tBoxMin);
        printf(">> tBoxMax = %d\n", tBoxMax);
        printf(">> capacity = %d\n", capacity);
    #endif

    fclose(config);
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