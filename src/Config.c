#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libs/SharedMem.h"

extern void log_message(char* msg);
extern void terminate(int code);

void read_conf(char* filename) {
    FILE* config;
    if ((config = fopen(filename, "r")) == NULL) {
        log_message("[Race Simulator] Failed to open config file");
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
        } else configs.timeUnit = in1;
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
            configs.lapDistance = in1;
            configs.lapCount = in2;
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
        } else configs.noTeams = in1;
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
        } else configs.maxCars = in1;
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
        } else configs.tBreakdown = in1;
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
            configs.tBoxMin = in1;
            configs.tBoxMax = in2;
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
        } else configs.capacity = in1;
        in1 = in2 = 0;
    } else {
        fprintf(stderr, "Invalid configuration file\n");
        fclose(config);
        return;
    }

    fclose(config);
    log_message("[Race Simulator] Configuration read");
}