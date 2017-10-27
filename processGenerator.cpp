#include "clkUtilities.h"
#include "queueUtilities.h"
#include <unistd.h>
#include <cstdio>
#include <fstream>
#include <queue>
#include <string>

//can't put it anywhere else :)
std::queue <processData> processes;

void ClearResources(int);
int readAlgorithmFromUser ();
pid_t createClk ();
pid_t createScheduler(int algorithm);
void readProcessFile (char* fileName);

int main() {

    initQueue(true);
    // 1-Ask the user about the chosen scheduling Algorithm and its parameters if exists.
    int algorithm = readAlgorithmFromUser();
    // 2-Initiate and create Scheduler and Clock processes.
    pid_t clkPid = createClk();
    pid_t schedulerPid = createScheduler(algorithm);
    // 3-use this function AFTER creating clock process to initialize clock, and initialize MsgQueue
    initClk();

    //4-Creating a data structure for process  and  provide it with its parameters
    readProcessFile("processes.txt");
    //5-Send & Notify the information to  the scheduler at the appropriate time 
    //(only when a process arrives) so that it will be put it in its turn.
    int schedulerStatus;
    while (! processes.empty()){
        int x = getClk();
        while (!processes.empty()) {
            if (x == processes.front().arrivingTime) {
                //TODO change the behaviour according to the algorithm
                Sendmsg(processes.front());
                processes.pop();
                kill(schedulerPid, SIGUSR1);
            } else {
                break;
            }
        }
    }

    //no more processes, send end of transmission message
    lastSend();
    waitpid(schedulerPid, &schedulerStatus, WNOHANG);
    pause();
    //To clear all resources
    ClearResources(0);
    return 0;
}

void ClearResources(int)
{
    msgctl(qid, IPC_RMID, (struct msqid_ds*)0);
    destroyClk(true); 
    exit(0);
}

int readAlgorithmFromUser (){
    printf("Please choose an algorithm\n 1- HPF\n 2- Shortest Remaining Time First\n 3- Round Robin\n");
    int algorithm;
    do {
        scanf("%d", &algorithm);
    }while (algorithm <1 || algorithm >3);
}

pid_t createClk (){
    pid_t clkPid = fork();
    if (clkPid == -1) {
        perror("failed to create clk fork");
    }
    else if (clkPid == 0) {
        printf("creating clk process\n");
        char *params[] = {"clk.out\0", NULL};
        int result = 0;
        result = execvp("./clk.out", NULL);
        if (result == -1) {
            printf("error can't create clk process\n");
        } else if (result == 0) {
            printf("sucess creating clk process\n");
        }
    }
    return clkPid;
}

pid_t createScheduler(int algorithm){
    pid_t schedulerPid = fork();
    if (schedulerPid == -1) {
        perror("failed to create scheduler fork");
    } else if (schedulerPid == 0) {
        char algoChar = algorithm + '0';
        char algorithmParam [] = {algoChar,NULL};
        char *parms[] = {"scheduler.out\0",algorithmParam,NULL};
        printf("creating scheduler process\n");
        int result = 0;
        result = execvp("./scheduler.out", parms);
        if (result == -1) {
            printf("error can't create scheduler process\n");
        } else if (result == 0) {
            printf("sucess creating scheduler process\n");
        }
    }
}

void readProcessFile (char* fileName){
    std::ifstream input;
    input.open(fileName);
    if (! input.is_open()){
        printf("can't open the file\n");
        exit(1);
    }
    int id, arrivalTime, priority, fullRunTime;
    while (input >> id){
        if (id == (int)'#'){
            std::string comment;
            getline(input,comment);
            continue;
        }
        input >> arrivalTime >> fullRunTime>>priority;
        processes.push(processData(id,priority,arrivalTime,fullRunTime, fullRunTime,-1,-1));
    }
}


