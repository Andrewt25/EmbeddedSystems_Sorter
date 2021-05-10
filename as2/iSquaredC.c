#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <fcntl.h>
#include <unistd.h>
#include "iSquaredC.h"
#include "sorter.h"

#define I2C_DEVICE_ADDRESS 0x20
#define REG_OUT_TOP 0x15
#define REG_OUT_BOTTOM 0x14
#define LEFT_DIGIT "/sys/class/gpio/gpio61/value"
#define RIGHT_DIGIT "/sys/class/gpio/gpio44/value"

static void displayArrayCount();
static void configDisplayPins();
static void executeCommand(char *command);
static void displayDigit(int number);
static int initBus(int address);
static void writeToReg(unsigned char out, unsigned char value);
static void setDisplayValues(int arraysSorterdPerSec);
static void toggleDigit(int state, char* digit);

struct numberSegment{
    unsigned char bottom;
    unsigned char top;
};
//TODO FLIP AROUND
const struct numberSegment numbers[] = {
    {0xA1, 0x86}, //0
    {0x80, 0x02}, //1
    {0x31, 0x0F}, //2
    {0xB0, 0x0F}, //3
    {0x90, 0x8B}, //4
    {0xB0, 0x8C}, //5
    {0xB1, 0x8C}, //6
    {0x80, 0x06}, //7
    {0xB1, 0x8E}, //8
    {0xB0, 0x8E}, //9
};

pthread_t iSquaredC_thread;
pthread_mutex_t iSquaredCLock;
int iSquaredCShutdownFlag = 1;
int leftDigit = 0;
int rightDigit = 0;
int arraysSortedTotal = 0;



void ISquaredC_StartDisplay(){
    
    pthread_mutex_init(&iSquaredCLock, NULL);
    pthread_create(&iSquaredC_thread, NULL, (void *) &displayArrayCount, NULL);

}

void ISquaredC_StopDisplay(){
    pthread_mutex_lock(&iSquaredCLock);
    iSquaredCShutdownFlag = 0;
    pthread_mutex_unlock(&iSquaredCLock);

    pthread_join(iSquaredC_thread, NULL);
    pthread_mutex_destroy(&iSquaredCLock);
    toggleDigit(0, LEFT_DIGIT);
    toggleDigit(0, RIGHT_DIGIT);
}


void ISquaredC_UpdateArraysSortedPerSec(){
    int total = Sorter_getNumberArraysSorted();
    pthread_mutex_lock(&iSquaredCLock);
    setDisplayValues(total - arraysSortedTotal);
    arraysSortedTotal = total;
    pthread_mutex_unlock(&iSquaredCLock);
}

static void setDisplayValues(int arraysSortedPerSec){
    if(arraysSortedPerSec > 99){
        leftDigit = 9;
        rightDigit = 9;
    } else if(arraysSortedPerSec == 0){
        leftDigit = 0;
        rightDigit = 0;
        int total = Sorter_getNumberArraysSorted(); 
        while(total == arraysSortedTotal){
            total = Sorter_getNumberArraysSorted();
        }
        leftDigit = 0;
        rightDigit = 1;
    }else {
        leftDigit = arraysSortedPerSec/10;
        rightDigit = arraysSortedPerSec % 10;
    }
}

static int initBus( int address){
    int fileDesc = open("/dev/i2c-1", O_RDWR);

    int result = ioctl(fileDesc, I2C_SLAVE, address);
    if(result < 0){
        perror("I2C: Unable to set I2C device to slave address.");
        exit(1);
    }
    return fileDesc;
} 

static void displayArrayCount(){
    configDisplayPins();
    int seconds = 0;
    long nanoseconds = 5000000;
    struct timespec delay = {seconds, nanoseconds};
    toggleDigit(0, LEFT_DIGIT);
    toggleDigit(0, RIGHT_DIGIT);
    
    while(iSquaredCShutdownFlag == 1){

        pthread_mutex_lock(&iSquaredCLock);
        displayDigit(leftDigit);
        pthread_mutex_unlock(&iSquaredCLock);

        toggleDigit(1, LEFT_DIGIT);
        nanosleep(&delay, (struct timespec *) NULL);
        toggleDigit(0, LEFT_DIGIT);

        pthread_mutex_lock(&iSquaredCLock);
        displayDigit(rightDigit);
        pthread_mutex_unlock(&iSquaredCLock);
        toggleDigit(1, RIGHT_DIGIT);
       
        nanosleep(&delay, (struct timespec *) NULL);
        toggleDigit(0, RIGHT_DIGIT);
       
    }
} 


static void displayDigit(int number){

    writeToReg(REG_OUT_TOP, numbers[number].top);
    writeToReg(REG_OUT_BOTTOM, numbers[number].bottom);
    
}

static void writeToReg(unsigned char out, unsigned char value){

    int fileDesc = initBus(I2C_DEVICE_ADDRESS);
    unsigned char buff[2];

    buff[0] = out;
    buff[1] = value;
    
    int result = write(fileDesc, buff, 2);
    if(result != 2){
        perror("I2C: Unable to write i2c register.");
        exit(1);
    }
    close(fileDesc);
}



static void configDisplayPins(){
    executeCommand("config-pin P9_18 i2c");
    executeCommand("config-pin P9_17 i2c");
    executeCommand("echo 61 > /sys/class/gpio/export");
    executeCommand("echo 44 > /sys/class/gpio/export");
    executeCommand("echo out > /sys/class/gpio/gpio61/direction");
    executeCommand("echo out > /sys/class/gpio/gpio44/direction");
    writeToReg(0x00, 0x00);
    writeToReg(0x01, 0x00);

}

static void executeCommand(char* command){
    FILE *pipe = popen(command, "r");
    fflush(pipe);
    
    if(!pipe){
        printf("Unable to execute Command: %s\n", command);
        exit(1);
    }

    int exitCode = WEXITSTATUS(pclose(pipe));
    if(exitCode != 0){
        printf("Command: %s failed with exit code %d\n", command, exitCode);
    }
}

static void toggleDigit(int state, char* digit){
    FILE *pFile = fopen(digit, "w");
    if(pFile == NULL){
        printf("ERROR OPENING %s\n", digit);
        exit(-1);
    }

    int charWriter = fprintf(pFile, "%d", (int)state);
    fflush(pFile);
    
    if(charWriter <= 0){
        printf("ERROR WRITING DATA\n");
        exit(-1);
    }
    fclose(pFile);
}

