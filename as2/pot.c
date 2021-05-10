#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include "pot.h"
#include "iSquaredC.h"
#include "sorter.h"

static void readPOT();
static int calculateArraySize();
static void setArraySize();
static int getLowerBoundDatapoint();

pthread_t pot_thread;
pthread_mutex_t potLock;
int potShutdownFlag = 1;
int potReading = 0;

struct ReadingArrayRelation{
    const int reading;
    const int arraySize;
};

const struct ReadingArrayRelation dataPoints[] = {
    {0, 1},
    {500, 20},
    {1000, 60},
    {1500, 120},
    {2000, 250},
    {2500, 300},
    {3000, 500},
    {3500, 800},
    {4000, 1200},
    {4100, 2100},
};


void Pot_StartReader(){

    pthread_mutex_init(&potLock, NULL);
    pthread_create(&pot_thread, NULL, (void *) &readPOT, NULL);
    ISquaredC_StartDisplay();
    

}
void Pot_StopReader(){
    pthread_mutex_lock(&potLock);
    potShutdownFlag = 0;
    pthread_mutex_unlock(&potLock);

    pthread_join(pot_thread, NULL);
    pthread_mutex_destroy(&potLock);

    ISquaredC_StopDisplay();
    
}

static void readPOT(){
    int seconds = 1;
    long nanoseconds = 0;
    struct timespec delay = {seconds, nanoseconds};
    int a2dReading = 0;

    while(potShutdownFlag == 1){
        nanosleep(&delay, (struct timespec *) NULL);
        FILE *potFile = fopen("/sys/bus/iio/devices/iio:device0/in_voltage0_raw", "r");
        if(!potFile){
            printf("ERROR: Unable to open voltage input file");
            fflush(stdout);
        }
        int itemsRead = fscanf(potFile, "%d", &a2dReading);
        if(itemsRead <= 0){
            printf("ERROR: Unable to read values form voltage input file");
            fflush(stdout);
        }
        fclose(potFile);
        if(potReading != a2dReading){
            potReading = a2dReading;
            setArraySize();
        }

        ISquaredC_UpdateArraysSortedPerSec();

    }
    pthread_exit(NULL);
    
}

static void setArraySize(){
    pthread_mutex_lock(&potLock);
    int newSize = calculateArraySize(potReading);
    pthread_mutex_unlock(&potLock);

    Sorter_setArraySize(newSize);
    pthread_mutex_lock(&potLock);
    printf("Pot Value: %d, Array size: %d\n", potReading, newSize);
    pthread_mutex_unlock(&potLock);
    fflush(stdout);
}

static int calculateArraySize(){
    int lowerIndex = getLowerBoundDatapoint(potReading);

    int upperIndex = lowerIndex + 1;

    // (input - pointA)/(pointB - pointA)
    double slope = ((double)potReading - dataPoints[lowerIndex].reading)/
    (dataPoints[upperIndex].reading- dataPoints[lowerIndex].reading); 

    // slope*(n-m) + m
    double arraySize = slope * (dataPoints[upperIndex].arraySize - dataPoints[lowerIndex].arraySize)
    + dataPoints[lowerIndex].arraySize;

    return (int)round(arraySize);
}

static int getLowerBoundDatapoint(){
    double lowerBound = (double)potReading/500;

    int dataPointIndex = floor(lowerBound);
    return dataPointIndex;
}