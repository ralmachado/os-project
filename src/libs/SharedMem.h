#define BUFFSIZE 256
#define NAMESIZE 16
#define true 1
#define false 0

typedef struct config_struct {
    int timeUnit;
    int lapDistance;
    int lapCount;
    int noTeams;
    int maxCars;
    int tBreakdown;
    int tBoxMin;
    int tBoxMax;
    int capacity;
} Config;

typedef struct car_struct {
    short int state; // Car state
    short int lowFuel; // Has low fuel
    short int malfunction; // Is malfunctioning
    int number; // Car number
    int speed; // Car speed
    int reliability; // Car reliability
    int position; // Car position in track
    int laps; // Completed laps
    int stops; // Total pit stops
    int finish; // Leaderboard position once finished
    double consumption; // Car consumption rate
    double fuel; // Car fuel tank
} Car;

typedef struct team_struct {
    short int init; // Team is initialized?
    short int box; // Team's box state
    int racers; // Total racers
    int id; // TODO Replace all instances of id with name
    char name[NAMESIZE];
    Car *cars; // This is yet another shared memory
} Team;

typedef struct shared_struct {
    int malfunctions;
    int topup;
    int pos;
    Team *teams; // This is another shared memory
    pthread_cond_t race_cv;
    pthread_mutex_t race_mutex;
    short int race_status;
} Sharedmem;

enum Box{FREE, RESERVED, OCCUPPIED};
enum State{RACE, SAFETY, BOX, QUIT, FINISH};

Config configs;
int shmid;
Sharedmem *shm;
