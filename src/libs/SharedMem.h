typedef struct mem_struct {
    int timeUnit, lapDistance, lapCount, noTeams, maxCars, tBreakdown, tBoxMin, tBoxMax, capacity;
} sharedmem;

sharedmem* configs;
int* boxes;
int configs_key, boxes_key;
