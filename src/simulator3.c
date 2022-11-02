#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "mem_init.h"

// Global variables for the carpark simulator 
// ---------------------------------------------
// Definitions 
#define MS_IN_MICROSECONDS 1000
#define QueueSize 100

// variables for car queue
pthread_mutex_t queueEntry;
pthread_mutex_t queueExit;
int carLevelCount[LEVELS] = {0};


// ---------------------------------------------
// Methods in charge of random generation 
// ---------------------------------------------

// The random function needs to be thread safe when generating random numbers
// ---------------------------------------------
pthread_mutex_t rand_mutex;;
int randThread(void){
    int randomNumber;
    pthread_mutex_lock(&rand_mutex);
    randomNumber = rand();
    pthread_mutex_unlock(&rand_mutex);
    return randomNumber;
}

// Random generator for picking a carpark level
// ---------------------------------------------
int randLevel(void){
    int randomLevel;
    randomLevel = randThread() % LEVELS;
    if (carLevelCount[randomLevel] < QueueSize){
        return randomLevel;
    }
    else{
        //Loop through the levels to find a level with space
        for (int i = 0; i < LEVELS; i++){
            if (carLevelCount[i] < QueueSize){
                return i;
            }
        }
    }
    return -1;
}

//---------------------------------------------
// Methods in charge of the car queue
//---------------------------------------------
// Method to add a car to the queue


//---------------------------------------------
// Methods for the LPR sensor
//---------------------------------------------
// Initialise the LPR sensor
void *initLPR(void *arg)
{
	lpr_sensor_t *lpr = arg;

    // check if failed 
    if (pthread_mutex_init(&lpr->mutex, NULL) != 0) {
        printf("LPR mutex init failed");
    }
    return NULL;
}

//---------------------------------------------
// Methods for the information display
//---------------------------------------------
// Initialise the information display
void *initDisplay(void *arg)
{
    information_sign_t *display = arg;

    // check if failed 
    if (pthread_mutex_init(&display->mutex, NULL) != 0) {
        printf("Display mutex init failed");
    }
    return NULL;
}


// ---------------------------------------------
// Functions relating to the boomgate          |
// ---------------------------------------------

// Function to initalise the boom gate
// ---------------------------------------------
void *boomgateInit(void *arg){
    boom_gate_t *boom_gate = arg;
    
    // Error check the mutex and condition variable
    if (pthread_mutex_init(&boom_gate->mutex, NULL) != 0) {
        printf("Boom gate mutex init failed");
    }

    // Lock the mutex 
    pthread_mutex_lock(&boom_gate->mutex);
    // Set the boom gate to closed
    boom_gate->status = 'C';
    //Broadcast the condition variable
    pthread_cond_broadcast(&boom_gate->cond);
    // Unlock the mutex
    pthread_mutex_unlock(&boom_gate->mutex);
    return NULL;

}

// Function that will be called by the boom gate thread
// ---------------------------------------------
void boomGateSimualtor(void *arg){
    // Get the boom gate thread
    boom_gate_t *boom_gate = arg;
    // Lock the boomgate mutex for use
    pthread_mutex_lock(&boom_gate->mutex);
    // Logic for the boom gate
    //------------------------
    // If the boomgate has been set to raise
    if (boom_gate->status == 'R'){
        // Raise the boom gate after 10ms
        usleep(10 * MS_IN_MICROSECONDS);
        boom_gate->status = 'O';
        // Broadcast the change to the boom gate
        pthread_cond_broadcast(&boom_gate->cond);
        //Print the boom gate status
        printf("Boom gate opening");
    }
    // If the boom gate has been set to open
    else if (boom_gate->status == 'O'){
        //Print the boom gate status
        printf("Boom gate is open");
    }
    // If the boom gate is set to lowering 
    else if (boom_gate->status == 'L'){
        // Lower the boom gate after 10ms
        usleep(10 * MS_IN_MICROSECONDS);
        boom_gate->status = 'C';
        // Broadcast the change to the boom gate
        pthread_cond_broadcast(&boom_gate->cond);
        //Print the boom gate status
        printf("Boom gate is closing");
    }
    // If the boom gate is set to closed
    else if (boom_gate->status == 'C'){
        //Print the boom gate status
        printf("Boom gate is closed");
    }
    // If the boom gate is set to an illegal status
    else{
        //Print the boom gate status
        printf("Boom gate is in an illegal state");
    }
    // Unlock the boom gate mutex
    printf("\n");
    pthread_mutex_unlock(&boom_gate->mutex);

}


// ---------------------------------------------
// Functions relating to the car               |
// ---------------------------------------------

// Function to get a plate from plates.txt
// ---------------------------------------------
char *getPlate(){
    // Allocate memory for the plate
    char *plate = malloc(6);
    FILE *fp = fopen("plates.txt", "r");
    // Error check the file
    if (fp == NULL){
        printf("Error opening file");
    }
    // Pick a random line 
    int randomPlateLine = randThread()%100 + 1;
    // printf("Line: %d\n", randomPlateLine);
    for(int i = 0; i < randomPlateLine; i++){
        fgets(plate, sizeof(plate)+1, fp);
    }
    fclose(fp);
    return plate;
}

// TERRIBLE CAR GENERATOR FOR NOW!
// ---------------------------------------------
void *carThread(void *shmCar){
    // Grab shared memory
    shared_memory_data_t *shm = shmCar;
    // Grab a random license plate from the text file
    //!TODO: Make this generate a random license plate
    char *plate = getPlate();
    char LicensePlate[6];
    strcpy(LicensePlate, plate);
    free(plate);
    // Grab a random level 
    int level = randLevel();
    // Grab a random exit level 
    //int exitLevel = randLevel();
    // Grab a random time to wait
    //!TODO TIMINGS
    //int waitTime = randThread() % 1000;


    // print car details
    printf("%s has arrived at entrance %d\n", LicensePlate, level+1);


    // Access the levels LPR sensor
    lpr_sensor_t *lpr = &shm->entrance[level].lpr_sensor;

    // Lock the LPR mutex
    //pthread_mutex_lock(&lpr->mutex);

    memcpy(lpr->plate, LicensePlate, sizeof(LicensePlate));

    // Broadcast the condition variable
    //pthread_cond_broadcast(&lpr->cond);
    // Signal the LPR sensor

    // Lock the LPR mutex
    pthread_mutex_lock(&lpr->mutex);
    pthread_cond_signal(&lpr->cond);
    pthread_mutex_unlock(&lpr->mutex);

    //pthread_mutex_unlock(&lpr->mutex);

    // TO THIS POINT

    // // Wait for the info sign to say the car can enter
    // information_sign_t *info = &shm->entrance[level-1].information_sign;
    // pthread_cond_wait(&info->cond, &info->mutex);


    // // If the car is rejected by the car park or full 
    // if (info->display == 'F' || info->display == 'X'){
    //     // Print the rejection message
    //     printf("%s has been rejected\n", LicensePlate);
    //     // Unlock the mutex
    //     pthread_mutex_unlock(&info->mutex);
    //     // Exit the thread
    //     pthread_exit(NULL);
    // }

    // // Check if the information sign gave an integer
    // if (info->display == '1' || info->display == '2' || info->display == '3' || info->display == '4' || info->display == '5'){
    //     // Print the acceptance message
    //     printf("%s heading to level ", LicensePlate);
    //     // Trigger level LPR 
    //     //lpr_sensor_t *levelLpr = &shm->level[info->display - '0' - 1].lpr_sensor;
    //     //int level = atoi(info->display);
    //     // Unlock the mutex
    //     pthread_mutex_unlock(&info->mutex);

    //     usleep(waitTime * MS_IN_MICROSECONDS);
    //     printf("Car is leaving the car park\n");
    //     //Trigger exit LPR
    //     lpr_sensor_t *exitLPR = &shm->exit[exitLevel].lpr_sensor;
    //     memcpy(exitLPR->plate, LicensePlate, sizeof(LicensePlate));

    //     // Exit the thread
    //     pthread_exit(NULL);
    // }

    // else {
    //     // Print the rejection message
    //     printf("NOT A VALID CHARACTER");
    //     // Unlock the mutex
    //     pthread_mutex_unlock(&info->mutex);
    //     // Exit the thread
    //     pthread_exit(NULL);
    // }



    return NULL;

    
}

// Generate a car every 1-100ms 
void *generateCar(void *shm ){
    printf("Generating cars\n");
    // Setting the maxmium amount of spawned cars before queue implementation!!!
    //!TODO: Implement queue
    int maximumCars = 100;
    pthread_t cars[maximumCars];
    for (int i = 0; i < maximumCars; i++){
        pthread_create(&cars[i], NULL, carThread, shm);
        int time = (randThread() % (100 - 1 + 1)) + 1; // create random wait time between 1-100ms
        
        int TestingMultiplier = 50;
        
        usleep(time*MS_IN_MICROSECONDS * TestingMultiplier);
    }

    return NULL;
    
}



// ---------------------------------------------
// Set defaults 
// ---------------------------------------------
//create a function that sets the default value for boom gate
void setBoomGateStatus(boom_gate_t *boom_gate, char status)
{
    boom_gate->status = status;
}

void setDefaults(shared_memory_t shm) {
    
    pthread_attr_t pthreadAttr;
    pthread_mutexattr_t mutexAttr;
    pthread_condattr_t condAttr;
    pthread_attr_init(&pthreadAttr);
    pthread_mutexattr_init(&mutexAttr);
    pthread_condattr_init(&condAttr);
    pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);
    pthread_condattr_setpshared(&condAttr, PTHREAD_PROCESS_SHARED);
    
    pthread_t *boomgatethreads = malloc(sizeof(pthread_t) * (ENTRANCES + EXITS));
    
    for (int i = 0; i < ENTRANCES; i++) {
		boom_gate_t *bg = &shm.data->entrance[i].boom_gate;
		pthread_create(boomgatethreads + i, NULL, boomgateInit,(void *) bg);
	}

    for (int i = 0; i < EXITS; i++) {
        boom_gate_t *bg = &shm.data->exit[i].boom_gate;
        pthread_create(boomgatethreads + i, NULL, boomgateInit,(void *) bg);
    }

    // Create threads for the LPR sensors
    pthread_t *lprthreads = malloc(sizeof(pthread_t) * (ENTRANCES + EXITS));
    // For LPR entrance
    for (int i = 0; i < ENTRANCES; i++) {
        lpr_sensor_t *lpr = &shm.data->entrance[i].lpr_sensor;
        pthread_create(lprthreads + i, NULL, initLPR,(void *) lpr);
    }
    // For LPR exit
    for (int i = 0; i < EXITS; i++) {
        lpr_sensor_t *lpr = &shm.data->exit[i].lpr_sensor;
        pthread_create(lprthreads + i, NULL, initLPR,(void *) lpr);
    }
    // For LPR on each level 
    for (int i = 0; i < LEVELS; i++) {
        lpr_sensor_t *lpr = &shm.data->level[i].lpr_sensor;
        pthread_create(lprthreads + i, NULL, initLPR,(void *) lpr);
    }

    // Create threads for the info signs
    pthread_t *infothreads = malloc(sizeof(pthread_t) * (ENTRANCES));
    // For info signs entrance
    for (int i = 0; i < ENTRANCES; i++) {
        information_sign_t *info = &shm.data->entrance[i].information_sign;
        pthread_create(infothreads + i, NULL, initDisplay,(void *) info);
    }


}

// main function 
int main(void)
{
    printf("Welcome to the simulator\n");
     //Set the seed of the random number generator
    srand(time(0));

    shared_memory_t shm;
    create_shared_object(&shm, "PARKING");
    setDefaults(shm);

    // create a thread to generate cars
    pthread_t carGen;
    // copy shm to a new pointer
    //shared_memory_t *shmCar = &shm;

    shared_memory_data_t *shmCar = shm.data;
    

    //lpr_sensor_t *lpr2 = &shm.data->entrance->lpr_sensor;
    pthread_create(&carGen, NULL, generateCar, (void *) shmCar);

    // Change level 4 LPR plate 

    // Put string into LPR sensor plate
    // char *plate = getPlate();
    // lpr_sensor_t *lpr = &shm.data->entrance[0].lpr_sensor;
    // memcpy(lpr->plate, plate, sizeof(char)*6);
    // printf("%s read into level LPR\n", lpr->plate);
    // free(plate);

    for (;;) {

        // // Close the boom gates for testing
        // for (int i = 0; i < ENTRANCES; i++) {
        //     shm.data->entrance[i].boom_gate.status = 'L';
        // }
        
        for(int i = 0; i < ENTRANCES; i++) {
            boom_gate_t *bg = &shm.data->entrance[i].boom_gate;

            if (bg->status == 'R'){
                pthread_t boomgatethread;
                pthread_create(&boomgatethread, NULL,(void *(*)(void *))boomGateSimualtor, (void *) bg);
                //pthread_join(boomgatethread, NULL);
            }
            else if (bg->status == 'L'){
                pthread_t boomgatethread;
                pthread_create(&boomgatethread, NULL,(void *(*)(void *))boomGateSimualtor, (void *) bg);
                //pthread_join(boomgatethread, NULL);
            }
        }


        usleep(5 * MS_IN_MICROSECONDS);

    }


}