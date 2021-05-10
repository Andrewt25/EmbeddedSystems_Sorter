#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include "sorter.h"
#include "udpHandler.h"

int main() {
UDPHandler_StartListening();
UDPHandler_WaitForExit();
}
