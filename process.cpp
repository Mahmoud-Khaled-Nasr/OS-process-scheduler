#include <unistd.h>
#include <string>
#include "clkUtilities.h"
#include <fstream>

#define FILE_NAME "scheduler.log"

/* Modify this file as needed*/
int remainingtime =0;
int id = -2;
int arrivingTime,fullRunningTime,waitingTime;
void stopHandler(int signal);

int main(int agrc, char* argv[]) {

    //if you need to use the emulated clock uncomment the following line
    initClk();
    signal(SIGTSTP,stopHandler);
    if (agrc < 3){
        printf("process with wrong number of parameters\n");
        exit(1);
    }
    remainingtime = std::atoi(argv[1]);
    id = std::atoi(argv[2]);
    arrivingTime= std::atoi(argv[3]);
    fullRunningTime= std::atoi(argv[4]);
    waitingTime = std::atoi(argv[5]);
    printf("process of id %d remaining time is %d is starting\n", id, remainingtime);
    while(remainingtime>0) {
       sleep(STEP_TIME);
       remainingtime--;
    }
    printf("process of id %d terminating at %d\n",id, getClk());
    //if you need to use the emulated clock uncomment the following line
    destroyClk(false);
    return 0;
}

void stopHandler(int signal) {
    printf("i am %d  with remaining time %d\n", id, remainingtime);
}
