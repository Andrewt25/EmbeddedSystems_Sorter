#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ctype.h>
#include <string.h>
#include <pthread.h>

#include "udpHandler.h"
#include "sorter.h"
#include "pot.h"

#define MAX_LEN 1024
#define MAX_ACTION_LEN 6
#define PORT 12345

static void listenForCommand();
static void sendResponse(char* msg);
static void handleRequest(char* command);
static void messageToLower(char* msg);
static void displayArray(int *arr, int length);
static void grabSubstringWord(char* msg, int wordPosition, char* substring);

struct sockaddr_in sinLocal;
struct sockaddr_in sinRemote;
int socketDescriptor;

pthread_t listener_thread;
pthread_mutex_t udpLock;
int listenerShutdownFlag = 1;



void UDPHandler_StartListening(){

   memset(&sinLocal, 0, sizeof(sinLocal));
   sinLocal.sin_family = AF_INET;
   sinLocal.sin_addr.s_addr = htonl(INADDR_ANY);
   sinLocal.sin_port = htons(PORT);

   socketDescriptor = socket(PF_INET, SOCK_DGRAM, 0);
   bind(socketDescriptor, (struct sockaddr*) &sinLocal, sizeof(sinLocal));

   Sorter_startSorting();
   Pot_StartReader();

   pthread_mutex_init(&udpLock, NULL); 
   pthread_create(&listener_thread, NULL, (void *) &listenForCommand, NULL);

}


void UDPHandler_WaitForExit(){
    pthread_join(listener_thread, NULL);
    pthread_mutex_destroy(&udpLock);
    close(socketDescriptor);
    Sorter_stopSorting();
    Pot_StopReader();
}

static void listenForCommand(){

    while(listenerShutdownFlag == 1){
        unsigned int sin_len = sizeof(sinRemote);
        char messageRx[MAX_LEN];
        
        int bytesRx = recvfrom(socketDescriptor, messageRx, MAX_LEN, 0, (struct sockaddr *) &sinRemote, &sin_len);
        int terminateIdx = (bytesRx < MAX_LEN) ? bytesRx : MAX_LEN -1;
        messageRx[terminateIdx] = 0;
        handleRequest(messageRx);
    }
    pthread_exit(NULL);

}

static void handleRequest(char* command){
    char responseMsg[MAX_LEN];
    messageToLower(command);
    char action[MAX_ACTION_LEN];
    grabSubstringWord(command, 1, action);
    if(!strncmp(action, "stop", 4)){
        sprintf(responseMsg, "Shutting Down\n");
        sendResponse(responseMsg);
        listenerShutdownFlag = 0;

    } else if(!strncmp(action, "count", 5)){
        long long arraysSorted = Sorter_getNumberArraysSorted();
        sprintf(responseMsg, "%lld\n", arraysSorted);
        sendResponse(responseMsg);

    } else if(!strncmp(action, "help", 4)){
        sprintf(responseMsg, "Accepted command Examples:\n");
        sendResponse(responseMsg);
        
        sprintf(responseMsg, "count       -- display number arrays sorted.\n");
        sendResponse(responseMsg);

        sprintf(responseMsg, "get length  -- display length of array currently being sorted.\n");
        sendResponse(responseMsg);

        sprintf(responseMsg, "get array   -- display the full array being sorted.\n");
        sendResponse(responseMsg);

        sprintf(responseMsg, "get 10      -- display the tenth element of array currently being sorted.\n");
        sendResponse(responseMsg);

        sprintf(responseMsg, "stop        -- cause the server program to end.\n");
        sendResponse(responseMsg);

    } else if(!strncmp(action, "get", 3)){
        int length = 0;
        int* arr = Sorter_getArrayData(&length);
        char actionParam[MAX_ACTION_LEN];
        grabSubstringWord(command, 2, actionParam);
        if(strlen(actionParam) != 0){

            if(!strncmp(actionParam, "array", 5)){
                displayArray(arr, length);

            } else if(!strncmp(actionParam, "length", 6)){
                sprintf(responseMsg, "length: %d\n", length);
                sendResponse(responseMsg);

            } else {
                int position;
                int convert = sscanf(actionParam, "%d", &position);

                if(convert == 1){
    
                    if(position <= length && position > 0){
                        sprintf(responseMsg, "Value %d = %d\n", position, arr[position-1]);
                        sendResponse(responseMsg);
                    } else {
                        sprintf(responseMsg, "Invalid argument. Must be between 1 and %d (array length).\n", length);
                        sendResponse(responseMsg);
                    }

                } else {
                    sprintf(responseMsg, "Invalid Parameter \"%s\" for Command \"get\"\n", actionParam);
                    sendResponse(responseMsg);
                }
            }

        } else{
            sprintf(responseMsg, "Command \"get\" requires parameter see \"help\" for more details\n");
            sendResponse(responseMsg);
        }
        free(arr);
        
    } else {
        sprintf(responseMsg, "Invalid Command see \"help\" for details\n");
        sendResponse(responseMsg);
    }
    memset(responseMsg, 0, MAX_LEN);
      
}

static void sendResponse(char* msg){
    int sin_len = sizeof(sinRemote);
    sendto(socketDescriptor, msg, strlen(msg), 0, (struct sockaddr *) &sinRemote, sin_len);
}

static void grabSubstringWord(char* msg, int wordPosition, char* substring){
    int wordStart = 0;
    int wordEnd = 0;

    for(int i = 0; i < wordPosition; i++){
        wordStart += wordEnd;
        wordEnd = 0;
        while(msg[wordStart] != '\0' && isspace(msg[wordStart])){
            wordStart++;
        }
        while(msg[wordEnd + wordStart] != '\0' && !isspace(msg[wordEnd + wordStart])){
            wordEnd++;
        }
    }
    memset(substring, 0, MAX_ACTION_LEN);
    strncpy(substring, msg + wordStart, wordEnd);

}

/* array value = 2bytes, ',' = 1byte and ' ' = 1 byte = 4bytes per number;
4 bytes * 10 numbers per line + \n 1byte = 41bytes perline 
128 bytes per packet 128/41 = ~3 send 3 lines per packet*/

static void displayArray(int *arr, int length){
    int elementsPerLine = 0;
    int linesForMessage = 0;
    char responseMsg[MAX_LEN];
    char *responsePointer = responseMsg;
    for(int i = 0; i < length; i++){
        responsePointer += sprintf(responsePointer, "%d, ", arr[i]);
        elementsPerLine++;
        if(elementsPerLine == 10){
            responsePointer += sprintf(responsePointer, "\n");
            elementsPerLine = 0;
            linesForMessage++;
        }
        if(linesForMessage == 3){
            sendResponse(responseMsg);
            memset(responseMsg, 0, MAX_LEN);
            responsePointer = responseMsg;
            linesForMessage = 0;
        }
    }
    if(elementsPerLine > 0 && elementsPerLine < 10) {
        responsePointer += sprintf(responsePointer, "\n");
    }
    sendResponse(responseMsg);
    memset(responseMsg, 0, MAX_LEN);
}

static void messageToLower(char* msg){
    int i = 0;
    while(msg[i]){
        char temp = tolower(msg[i]);
        msg[i] = temp;
        i++;
    }
}


