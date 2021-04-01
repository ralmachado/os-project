#include <semaphore.h>

typedef struct mem_struct {
    int timeUnit, lapDistance, lapCount, noTeams, maxCars, tBreakdown, tBoxMin, tBoxMax, capacity;
    int* boxes;
} sharedmem;

sharedmem* configs;
int* boxes;
int configs_key, boxes_key;
sem_t *mutex;