#include "clkUtilities.h"
#include "queueUtilities.h"
#include <unistd.h>
#include <cstdio>
#include <fstream>
#include <string>
#include <queue>

//can't put it anywhere else :)
std::queue <processData> processes;

void ClearResources(int);

int main() {

    initQueue(true);
    // 1-Ask the user about the chosen scheduling Algorithm and its parameters if exists.
    printf("Please choose an algorithm\n 1- HPF\n 2- Shortest Remaining Time First\n 3- Round Robin\n");
    int algorithm;
    do {
        scanf("%d", &algorithm);
    }while (algorithm <1 || algorithm >3);

    // 2-Initiate and create Scheduler and Clock processes.
    pid_t schedulerPid;
    pid_t clkPid = fork();
    if (clkPid == -1) {
        perror("failed to create clk");
    } else if (clkPid == 0) {
        printf("creating clk process\n");
        execl("/home/mk18/Desktop/Ass1OS/clk.out","clk", NULL);
    }else {
        schedulerPid = fork();
        if (schedulerPid == -1) {
            perror("failed to create scheduler");
        } else if (schedulerPid == 0) {
                char algorithmChar = algorithm + '0';
                printf("creating scheduler process\n");
                int result = 0;
                result = execl("/home/mk18/Desktop/Ass1OS/scheduler.out", &algorithmChar, NULL);
                if (result == -1) {
                    printf("error sch\n");
                } else if (result == 0) {
                    printf("sucess\n");
                }
            }
        }

    /*
    else {
        clkPid = fork();
        if (clkPid == -1) {
            perror("failed to create clk");
        } else if (clkPid == 0) {
            printf("creating clk process\n");
            execl("./clk","clk", NULL);
        }
    }
*/
    // 3-use this function AFTER creating clock process to initialize clock, and initialize MsgQueue
    printf("i am here\n");
    sleep(5);
    initClk();

    //4-Creating a data structure for process  and  provide it with its parameters
    std::ifstream input;
    input.open("processes.txt");
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
    //5-Send & Notify the information to  the scheduler at the appropriate time 
    //(only when a process arrives) so that it will be put it in its turn.

    int schedulerStatus;
    while (! processes.empty()){
        int x = getClk();
        while (!processes.empty()) {
            if (x == processes.front().arrivingTime) {
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





