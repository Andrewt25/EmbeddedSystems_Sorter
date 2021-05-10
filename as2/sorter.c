#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include "sorter.h"

static void sortNewArray();
static void createRandomArray();
static void bubbleSort();
static void swapArrayElements(int *array, int i, int j);


int *inProgressArray;
int arraySize = 100;
int inProgressArraySize = 0;
long long sortedArrays = 0;
pthread_t sorter_thread;
pthread_mutex_t sorterLock;
int sorterShutdownFlag = 1;

void Sorter_startSorting(){

    pthread_mutex_init(&sorterLock, NULL);
    pthread_create(&sorter_thread, NULL, (void *) &sortNewArray, NULL);

}
void Sorter_stopSorting(){
    pthread_mutex_lock(&sorterLock);
    sorterShutdownFlag = 0;
    pthread_mutex_unlock(&sorterLock);

    pthread_join(sorter_thread, NULL);
    pthread_mutex_destroy(&sorterLock);
    
}

static void sortNewArray(){

    while(sorterShutdownFlag == 1){
        pthread_mutex_lock(&sorterLock);
        inProgressArray = malloc(arraySize * sizeof(int));
        inProgressArraySize = arraySize;
        createRandomArray();
        pthread_mutex_unlock(&sorterLock);

        bubbleSort();

        pthread_mutex_lock(&sorterLock);
        free(inProgressArray);
        inProgressArraySize = 0;
        sortedArrays += 1;
        pthread_mutex_unlock(&sorterLock);
    }
    pthread_exit(NULL);

}

static void createRandomArray(){
    
    for(int i = 1; i <= inProgressArraySize; i++){
        inProgressArray[i-1] = i;
    }
    srand(time(NULL));
    for(int r = 0; r < inProgressArraySize; r++){
        int i = rand() % inProgressArraySize;
        int j = rand() % inProgressArraySize;
        swapArrayElements(inProgressArray, i, j);
    }

}

static void bubbleSort(){
    pthread_mutex_lock(&sorterLock);
    int inProgressArraySizeCopy = inProgressArraySize;
    pthread_mutex_unlock(&sorterLock);

    for(int i = 1;  i <= inProgressArraySizeCopy -1; i++){
        for(int j = 0; j <= inProgressArraySizeCopy - i - 1; j++){
            pthread_mutex_lock(&sorterLock);
            if(inProgressArray[j] > inProgressArray[j+1]){
                swapArrayElements(inProgressArray, j, j+1);
            }
            pthread_mutex_unlock(&sorterLock);
        }
    }
}

void static swapArrayElements(int *array, int i, int j){
    int temp = array[i];
    array[i] = array[j];
    array[j] = temp;
}

void Sorter_setArraySize(int newSize){
    pthread_mutex_lock(&sorterLock);
    arraySize = newSize;
    pthread_mutex_unlock(&sorterLock);
}
int Sorter_getArrayLength(void){
    pthread_mutex_lock(&sorterLock);
    int arrayLength = inProgressArraySize;
    pthread_mutex_unlock(&sorterLock);

    return arrayLength;
}

int* Sorter_getArrayData(int *length){
    pthread_mutex_lock(&sorterLock);
    *length = inProgressArraySize;
    pthread_mutex_unlock(&sorterLock);

    int *inProgressCopy = malloc(*length * sizeof(int));
    memcpy(inProgressCopy, inProgressArray, *length *sizeof(int));
    
    return inProgressCopy;
}

long long Sorter_getNumberArraysSorted(){
    pthread_mutex_lock(&sorterLock);
    long long sortedArrayCopy = sortedArrays;
    pthread_mutex_unlock(&sorterLock);

    return sortedArrayCopy;
};
