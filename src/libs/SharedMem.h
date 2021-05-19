#define BUFFSIZE 256
#define NAMESIZE 16

typedef enum {false, true} bool;
typedef enum {FREE, RESERVED, OCCUPPIED} Box;
typedef enum {RACE, SAFETY, BOX, QUIT, FINISH} State;

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
    State state; // Car state
    bool lowFuel; // Has low fuel
    bool malfunction; // Is malfunctioning
    int number; // Car number
    int speed; // Car speed
    int reliability; // Car reliability
    int position; // Car position in track
    int laps; // Completed laps
    int stops; // Total pit stops
    int finish; // Leaderboard position once finished
    int finalpos; // Final leaderboard pos
    double consumption; // Car consumption rate
    double fuel; // Car fuel tank
    char *team;
} Car;

typedef struct team_struct {
    bool init; // Team is initialized?
    Box box; // Team's box state
    int racers; // Total racers
    int id; // TODO Replace all instances of id with name
    char name[NAMESIZE];
    Car *cars; // This is yet another shared memory
} Team;

typedef struct shared_struct {
    int malfunctions;
    int topup;
    int pos;
    int on_track;
    int racing;
    int init_cars;
    Team *teams; // This is another shared memory
    pthread_cond_t race_cv;
    pthread_mutex_t race_mutex, close_mutex;
    bool race_status;
    bool race_int; // SIGINT received
    bool race_usr1; // SIGUSR1 received
} Sharedmem;

Config configs;
Sharedmem *shm;
Car** stats_arr;
int statsid;