// David Valente Pereira Barros Leitão - 2019223148
// Rodrigo Alexandre da Mota Machado - 2019218299

// Configuration file parsing

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libs/header.h"

extern void log_message(char* msg);
extern void terminate(int code);

/* 
 * TODO If time is not of the essence, switch atoi and atof to strtol and strtod 
 * to check for conversion errors
 */
int read_conf(char* filename) {
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
            log_message("[Race Simulator] Bad config: timeUnit null");
            fclose(config);
            return 1;
        } else configs.timeUnit = in1;
        in1 = in2 = 0;
    } else {
        log_message("[Race Simulator] Bad config: check file structure");
        fclose(config);
        return 1;
    }

    if (fgets(buff, sizeof(buff) - 1, config)) {
        token = strtok(buff, ", ");
        in1 = atoi(token);
        token = strtok(NULL, "\n");
        in2 = atoi(token);
        if (in1 <= 0) {
            log_message("[Race Simulator] Bad config: lapDistance is not a positive integer\n");
            fclose(config);
            return 1;
        } else if (in2 <= 0) {
            log_message("[Race Simulator] Bad config: lapCount is not a positive integer\n");
            fclose(config);
            return 1;
        } else {
            configs.lapDistance = in1;
            configs.lapCount = in2;
        }
        in1 = in2 = 0;
    } else {
        log_message("[Race Simulator] Bad config: check file structure");
        fclose(config);
        return 1;
    }


    if (fgets(buff, sizeof(buff) - 1, config)) {
        in1 = atoi(buff);
        if (in1 < 3) {
            log_message("[Race Simulator] Bad config: Not enough teams to setup a race, minimum is 3\n");
            fclose(config);
            return 1;
        } else configs.noTeams = in1;
        in1 = in2 = 0;
    } else {
        log_message("[Race Simulator] Bad config: check file structure");
        fclose(config);
        return 1;
    }

    if (fgets(buff, sizeof(buff) - 1, config)) {
        in1 = atoi(buff);
        if (in1 <= 0) {
            log_message("[Race Simulator] Bad config: maxCars is not a positive integer\n");
            fclose(config);
            return 1;
        } else configs.maxCars = in1;
        in1 = in2 = 0;
    } else {
        log_message("[Race Simulator] Bad config: check file structure");
        fclose(config);
        return 1;
    }

    if (fgets(buff, sizeof(buff) - 1, config)) {
        in1 = atoi(buff);
        if (in1 <= 0) {
            log_message("[Race Simulator] Bad config: tBreakdown is not a positive integer\n");
            fclose(config);
            return 1;
        } else configs.tBreakdown = in1;
        in1 = in2 = 0;
    } else {
        log_message("[Race Simulator] Bad config: check file structure");
        fclose(config);
        return 1;
    }

    if (fgets(buff, sizeof(buff) - 1, config)) {
        token = strtok(buff, ", ");
        in1 = atoi(token);
        token = strtok(NULL, "\n");
        in2 = atoi(token);
        if (in1 <= 0) {
            log_message("[Race Simulator] Bad config: tBoxMin is not a positive integer\n");
            fclose(config);
            return 1;
        } else if (in2 < in1) {
            log_message("[Race Simulator] Bad config: tBoxMax is not greater than tBoxMin\n");
            fclose(config);
            return 1;
        } else {
            configs.tBoxMin = in1;
            configs.tBoxMax = in2;
        }
        in1 = in2 = 0;
    } else {
        log_message("[Race Simulator] Bad config: check file structure");
        fclose(config);
        return 1;
    }

    if (fgets(buff, sizeof(buff) - 1, config)) {
        in1 = atoi(buff);
        if (in1 <= 0) {
            log_message("[Race Simulator] Bad config: capacity is not a positive integer\n");
            fclose(config);
            return 1;
        } else configs.capacity = in1;
        in1 = in2 = 0;
    } else {
        log_message("[Race Simulator] Bad config: check file structure");
        fclose(config);
        return 1;
    }

    fclose(config);
    log_message("[Race Simulator] Configuration read");
    return 0;
}