#include <unistd.h>
#include <string>
#include "clkUtilities.h"
#include <fstream>

#define FILE_NAME "scheduler.log"

/* Modify this file as needed*/
int remainingtime =0;
int id = -2;
int arrivingTime,fullRunningTime,waitingTime;
int previousClk, currentClk;

void stopHandler(int signal);

int main(int agrc, char* argv[]) {

    initClk();
    signal(SIGTSTP,stopHandler);
    remainingtime = std::atoi(argv[1]);
    id = std::atoi(argv[2]);
    arrivingTime= std::atoi(argv[3]);
    fullRunningTime= std::atoi(argv[4]);
    waitingTime = std::atoi(argv[5]);
    printf("process of id %d remaining time is %d is starting\n", id, remainingtime);
    previousClk = getClk();
    currentClk = getClk();
    while(remainingtime>0) {
        while (previousClk == currentClk){
            currentClk=getClk();
        }
        previousClk=currentClk;
        //sleep(STEP_TIME);
        remainingtime--;
    }
    printf("process of id %d terminating at %d\n",id, getClk());
    destroyClk(false);
    return 0;
}

void stopHandler(int signal) {
    printf("i am process id %d stopping with remaining time %d\n", id, remainingtime);
    previousClk = currentClk =getClk();
}
