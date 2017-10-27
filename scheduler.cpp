#include "clkUtilities.h"
#include "queueUtilities.h"
#include <cstdio>
#include <fstream>
#include <queue>

std::queue <processData> recievedProcesses;
bool processGeneratorFinish = false;
pid_t currentProcessId = -1;
std::priority_queue <processData> processes;
bool processRunning = false;

void newProcessHandler (int signal);
void HPFScheduler (std::priority_queue <processData> & processes);
void SRTScheduler ();
void createNewProcessSRT();
void deadProcess(int signal);
void saveDeadProcessData (processData process);
int main(int argc, char* argv[]) {

    initQueue(false);
    initClk();

    switch (*argv[1]){
        case '1': {
            printf("using HPF\n");
            std::priority_queue<processData> HPFQueue;
            break;
        }
        case '2': {
            printf("using shortest time remainder\n");
            signal(SIGUSR1,newProcessHandler);
            signal(SIGCHLD, deadProcess);
            SRTScheduler();
            //TODO claculate the calculations
            break;
        }
        case '3': {
            printf("using RR\n");
            std::priority_queue<processData> RRQueue;
            break;
        }
        default: {
            printf("wrong arguments\n");
        }
    }



    //TODO: implement the scheduler :)


    //===================================
    //Preimplemented Functions examples

    /////Toget time use the following function
    int x= getClk();
    printf("current time is %d\n",x);

    //////To receive something from the generator, for example  id 2
    struct processData pD;
    Recmsg(pD); //returns -1 on failure; 1 on the end of processes, 0 no processes available yet
    printf("current received data %d\n",pD.id);

    return 0;
    
}

void HPFScheduler (std::priority_queue <processData> & processes){
    /*bool runningProcess = false;
    pid_t currentProcessId;
    processData temp;
    int currentTime, previousTime;
    currentTime = previousTime = getClk();
    while ()*/
}

void SRTScheduler (){
    std::ofstream logFile;
    logFile.open("scheduler.log");
    if (! logFile.is_open()){
        printf("can't open log file\n");
    }
    //printf("i am before the while time %d\n",getClk());
    while (! processGeneratorFinish || ! recievedProcesses.empty() || ! processes.empty()){
        //currentTime = getClk();
        //FOR TEST
        printf("i am in the while time %d",getClk());
        int processStatus;
        while (! recievedProcesses.empty()){
            processData temp = recievedProcesses.front();
            recievedProcesses.pop();
            processes.push(temp);
        }
        if (! processes.empty()){
            //run the process
            if (processes.top().processId == -1){
                //printing im the log file
                logFile << "At time "<< getClk()<< " process " << processes.top().id << " started arr "
                        << processes.top().arrivingTime << " total "<< processes.top().fullRunningTime <<" remain "
                        << processes.top().remainingTime <<" wait " << getClk() - processes.top().arrivingTime << std::endl;
                //Create new process
                createNewProcessSRT();
            } else {
                currentProcessId = processes.top().processId;
                kill(currentProcessId,SIGCONT);
            }
            //waitpid(currentProcessId, &processStatus, WNOHANG);
        }
        pause();
    }

    printf("i am after the loop\n");
}

void deadProcess (int signal){
    processData temp = processes.top();
    processes.pop();
    temp.finsihTime = getClk();
    saveDeadProcessData(temp);
    currentProcessId = -1;
}

void saveDeadProcessData (processData process){
    //TODO save the required data
}

void createNewProcessSRT(){
    processData temp = processes.top();
    temp.processId = currentProcessId = fork();
    processes.push(temp);
    if (currentProcessId == -1 ){
        perror("can't fork new process\n");
    }else if (currentProcessId == 0){
        //TODO send the parameters to the process
        char* parms []={NULL};
        int result = 0;
        result = execvp("./process.out", parms);
        if (result == -1) {
            perror("error can't exec the new process\n");
        } else if (result == 0) {
            perror("sucess exec the new process\n");
        }
    }
}

void newProcessHandler (int signal){
    processData temp;
    int result;
    do {
        result = Recmsg(temp);
        if (result == -1) {
            printf("Can't recieve massage for process generator");
        } else if (result == 0) {
            recievedProcesses.push(temp);
            if (currentProcessId != -1) {
                kill(currentProcessId, SIGTSTP);
                processRunning = false;
            }
            printf("received id %d arr time %d and clk %d",temp.id,temp.arrivingTime,getClk());
        } else if (result == 1) {
            processGeneratorFinish = true;
        }
    }while (result != -1);
    return;
}