#include "clkUtilities.h"
#include "queueUtilities.h"
#include <unistd.h>
#include <cstdio>
#include <fstream>
#include <queue>
#include <string>
#include <cstring>

//can't put it anywhere else :)
std::queue <processData> processes;

void ClearResources(int);
int readAlgorithmFromUser ();
int readQuanta();
pid_t createClk ();
pid_t createScheduler(int algorithm, int quanta);
void readProcessFile (char* fileName, int algorithm);

int main() {

    initQueue(true);
    // 1-Ask the user about the chosen scheduling Algorithm and its parameters if exists.
    int algorithm = readAlgorithmFromUser();
    int quanta;
    if (algorithm==3){
        quanta = readQuanta();
    }
    // 2-Initiate and create Scheduler and Clock processes.
    pid_t clkPid = createClk();
    pid_t schedulerPid = createScheduler(algorithm, quanta);
    // 3-use this function AFTER creating clock process to initialize clock, and initialize MsgQueue
    initClk();
    //4-Creating a data structure for process  and  provide it with its parameters
    readProcessFile("processes.txt",algorithm);
    //5-Send & Notify the information to  the scheduler at the appropriate time 
    //(only when a process arrives) so that it will be put it in its turn.
    int schedulerStatus;
    bool sendMassage = false;
    while (! processes.empty()){
        sleep(1);
        int x = getClk();
        while (!processes.empty()) {
            if (x == processes.front().arrivingTime) {
                Sendmsg(processes.front());
                processes.pop();
                if (algorithm == 2){
                    sendMassage = true;
                }
            } else {
                if (algorithm == 2 && sendMassage) {
                    sendMassage = false;
                    kill(schedulerPid, SIGUSR1);
                    printf("send the receive signal at %d\n",x);
                    sleep(1);
                }
                break;
            }
        }
    }

    //no more processes, send end of transmission message
    sleep(1);
    lastSend();
    if (algorithm == 2) {
        kill(schedulerPid, SIGUSR1);
    }
    printf("send the lastSend\n");
    //the process generator is waiting until the scheduler finishes its work and terminates to clear resources
    int tempPid;
    do {
        tempPid = waitpid(schedulerPid, &schedulerStatus, WNOHANG);
        sleep(1);
    }while (tempPid != schedulerPid);
    printf("the process generator is terminating\n");
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
    return algorithm;
}

int readQuanta(){
    int quanta;
    printf("enter the quanta\n");
    scanf("%d",&quanta);
    return quanta;
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

pid_t createScheduler(int algorithm, int quanta){
    pid_t schedulerPid = fork();
    if (schedulerPid == -1) {
        perror("failed to create scheduler fork");
    } else if (schedulerPid == 0) {
        char algoChar = algorithm + '0';
        char algorithmParam [] = {algoChar,NULL};

        std::string valueString = std::to_string(quanta);
        char* valueParam = new char [valueString.length()+1];
        std::strcpy(valueParam,valueString.c_str());
        valueParam[valueString.length()]='\0';

        char *parms[] = {"scheduler.out\0",algorithmParam, valueParam, NULL};
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

void readProcessFile (char* fileName, int algorihtm){
    std::ifstream input;
    input.open(fileName);
    if (! input.is_open()){
        printf("can't open the file\n");
        exit(1);
    }

    int id, arrivalTime, priority, fullRunTime;
    while (! input.eof()){
        input>>id;
        if (id == 0){
            input.clear();
            std::string comment;
            getline(input,comment,'\n');
            continue;
        }
        input >> arrivalTime >> fullRunTime>>priority;
        processes.push(processData(id,priority,arrivalTime,fullRunTime, fullRunTime,-1,-1,algorihtm));
    }
    //FOR TEST
    /*int length = processes.size();
    for (int i = 0; i < length; ++i) {
        processData temp = processes.front();
        processes.pop();
        printf("id %d arr %d\n",temp.id, temp.arrivingTime);
        processes.push(temp);
    }*/
}


