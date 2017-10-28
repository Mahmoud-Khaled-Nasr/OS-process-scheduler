
#include <unistd.h>
#include <string>
#include "clkUtilities.h"

/* Modify this file as needed*/
int remainingtime =0;
int id = -2;
int arrivingTime,fullRunningTime,waitingTime;
void stopHandler(int signal);

int main(int agrc, char* argv[]) {

    //if you need to use the emulated clock uncomment the following line
    initClk();
    signal(SIGSTOP,stopHandler);
    if (agrc < 3){
        printf("process with wrong number of parameters\n");
        exit(1);
    }
    remainingtime = std::atoi(argv[1]);
    id = std::atoi(argv[2]);
    printf("process of id %d remaining time is %d\n", id, remainingtime);
    arrivingTime= std::atoi(argv[3]);
    fullRunningTime= std::atoi(argv[4]);
    waitingTime = std::atoi(argv[5]);
    while(remainingtime>0) {
       sleep(STEP_TIME);
       remainingtime--;
    }
    printf("process terminating \n");
    //if you need to use the emulated clock uncomment the following line
    destroyClk(false);
    return 0;
}

void stopHandler(int signal){
    logFile << "At time "<< getClk()<< " process " << id << " stopped arr "
            << arrivingTime << " total "<< fullRunningTime <<" remain "
            << remainingtime <<" wait " << waitingTime << std::endl;
}
