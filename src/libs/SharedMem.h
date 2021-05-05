typedef struct {
    int timeUnit, lapDistance, lapCount, noTeams, maxCars, tBreakdown, tBoxMin, tBoxMax, capacity;
} Config;

typedef struct {
    int position;
    int laps;
    int fuel;
    int stops;
    int malfunctions;
    int topup;
} Car;

typedef struct {
    Car* cars;
} Team;

typedef struct {
    // Insert structs here
    int *boxes;
    Team* teams;
} Sharedmem;

enum Box{FREE, RESERVED, OCCUPPIED};

Config configs;
Sharedmem *shm;
int shmid;
