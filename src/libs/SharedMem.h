typedef struct {
    int timeUnit, lapDistance, lapCount, noTeams, maxCars, tBreakdown, tBoxMin, tBoxMax, capacity;
} Config;

typedef struct {
    short int state;
    int position;
    int laps;
    int fuel;
    int stops;
    int malfunctions;
    int topup;
} Car;

typedef struct {
    int id;
    Car* cars;
} Team;

typedef struct {
    int *boxes;
    Team **teams;
} Sharedmem;

enum Box{FREE, RESERVED, OCCUPPIED};
enum State{RACE, SAFETY, BOX, QUIT, FINISH};

Config configs;
Sharedmem *shm;
int shmid;
