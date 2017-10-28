#include "clkUtilities.h"
#include "queueUtilities.h"
#include "HPF.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <fstream>
#include <queue>

#define FILE_NAME "scheduler.log"

std::queue <processData> recievedProcesses;
bool processGeneratorFinish = false;
pid_t currentProcessId = -1;
std::priority_queue <processData> processes;
bool processRunning = false;
char currentAlgorithm;

void newProcessHandler (int signal);
void SRTScheduler ();
void createNewProcessSRT();
void deadProcess(int signal);
void saveDeadProcessData (processData process);
char* createProcessPrameters (int value);
int main(int argc, char* argv[]) {

    currentAlgorithm = *argv[1];
    initQueue(false);
    initClk();
    //initializing the log file
    logFile.open(FILE_NAME);
    if (! logFile.is_open()){
        printf("can't open log file\n");
        exit(1);
    }
    logFile.close();
    switch (*argv[1]){
        case '1': {
            printf("using HPF\n");
            HPF HPFScheduler;
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
    printf("scheduler is exiting\n");
    return 0;
}

void SRTScheduler (){
    //printf("i am before the while time %d\n",getClk());
    while (! processGeneratorFinish || ! recievedProcesses.empty() || ! processes.empty()){
        //FOR TEST
        printf("i am in the while time %d\n",getClk());
        int processStatus;
        processData lastAddedProcess ;
        while (! recievedProcesses.empty()){
            processData temp = recievedProcesses.front();
            recievedProcesses.pop();
            if (temp.id != lastAddedProcess.id) {
                processes.push(temp);
            }
            lastAddedProcess=temp;
        }
        if (! processes.empty()){
            //run the process
            processData temp = processes.top();
            if (temp.processId == -1){
                //printing in the log file
                temp.waitingTime = getClk() - temp.arrivingTime;
                temp.startRunningTime= getClk();
                logFile.open(FILE_NAME, std::fstream::app);
                if (! logFile.is_open()){
                    printf("can't open the file\n");
                    exit(1);
                }
                logFile << "At time "<< getClk()<< " process " << temp.id << " started arr "
                        << temp.arrivingTime << " total "<< temp.fullRunningTime <<" remain "
                        << temp.remainingTime <<" wait " << temp.waitingTime << std::endl;
                logFile.close();
                processes.pop();
                processes.push(temp);
                //Create new process
                createNewProcessSRT();
            } else {
                currentProcessId = temp.processId;
                temp.startRunningTime = getClk();
                processes.pop();
                processes.push(temp);
                kill(currentProcessId,SIGCONT);
                logFile.open(FILE_NAME, std::fstream::app);
                if (! logFile.is_open()){
                    printf("can't open the file\n");
                    exit(1);
                }
                logFile << "At time "<< getClk()<< " process " << temp.id << " resumed arr "
                        << temp.arrivingTime << " total "<< temp.fullRunningTime <<" remain "
                        << temp.remainingTime <<" wait " << getClk() - temp.arrivingTime << std::endl;
                logFile.close();
            }
        }
        if (! processGeneratorFinish || ! recievedProcesses.empty() || ! processes.empty())
        {
            pause();
        }
    }
}

void deadProcess (int signal) {

    int status, triggeredProcessId;
    triggeredProcessId = waitpid(currentProcessId, &status, WNOHANG);
    if (triggeredProcessId == currentProcessId) {
        processData temp = processes.top();
        processes.pop();
        if (temp.processId != currentProcessId) {
            printf("killing something wrong queue process id = %d id %d currentProcessId %d\n", temp.processId,
                   temp.id,
                   currentProcessId);
        } else {
            printf("I am doing the right thing \n");
        }
        temp.finsihTime = getClk();
        temp.remainingTime = 0;
        saveDeadProcessData(temp);
        logFile.open(FILE_NAME, std::fstream::app);
        if (!logFile.is_open()) {
            printf("can't open the file\n");
            exit(1);
        }
        logFile << "At time " << getClk() << " process " << temp.id << " finished arr "
                << temp.arrivingTime << " total " << temp.fullRunningTime << " remain "
                << temp.remainingTime << " wait " << temp.waitingTime << std::endl;
        logFile.close();
        currentProcessId = -1;
    } else if (triggeredProcessId == 0) {
        printf("the process %d stopped or resummed \n", currentProcessId);
    } else {
        printf("something is wrong in the deadhandler %d\n", triggeredProcessId);
    }

}

void saveDeadProcessData (processData process){
    //TODO save the required data
}

void createNewProcessSRT(){
    processData temp = processes.top();
    processes.pop();
    currentProcessId = fork();
    if (currentProcessId == -1 ){
        perror("can't fork new process\n");
    }else if (currentProcessId == 0){
        //TODO send the parameters to the process
        char* remainingTimeParam = createProcessPrameters(temp.remainingTime);
        char* idParam = createProcessPrameters(temp.id);
        char* arrivingTimeParam =createProcessPrameters(temp.arrivingTime);
        char* fullRunningTimeParam = createProcessPrameters(temp.fullRunningTime);
        char* waitingTimeParam = createProcessPrameters(temp.waitingTime);
        char* parms []={"process.out\0",remainingTimeParam, idParam, arrivingTimeParam, fullRunningTimeParam, waitingTimeParam, NULL};
        int result = 0;
        result = execvp("./process.out", parms);
        if (result == -1) {
            perror("error can't exec the new process\n");
        } else if (result == 0) {
            perror("sucess exec the new process\n");
        }
    }
    temp.processId = currentProcessId;
    processes.push(temp);
}

void newProcessHandler (int signal){
    processData temp;
    printf("starting the new process handler\n");
    int result=2;
    do {
        if (result == -1) {
            printf("Can't recieve massage for process generator or no massages to be received\n");
        } else if (result == 1) {
            processGeneratorFinish = true;
            printf("lastSend recieved\n");
        } else if (result == 0 && ! processGeneratorFinish) {
            recievedProcesses.push(temp);
            printf("received id %d arr time %d and clk %d\n", temp.id, temp.arrivingTime, getClk());
            if (currentProcessId != -1) {
                kill(currentProcessId, SIGTSTP);
                processData temp = processes.top();
                processes.pop();
                logFile.open(FILE_NAME, std::fstream::app);
                if (! logFile.is_open()){
                    printf("can't open the file\n");
                    exit(1);
                }
                temp.remainingTime = temp.remainingTime - (getClk() - temp.startRunningTime);
                processes.push(temp);
                logFile << "At time "<< getClk()<< " process " << temp.id << " stopped arr "
                    << temp.arrivingTime << " total "<< temp.fullRunningTime <<" remain "
                    << temp.remainingTime <<" wait " << temp.waitingTime << std::endl;
                logFile.close();
                processRunning = false;
                currentProcessId = -1;
            }
        }
        result = Recmsg(temp);
    }while (result != -1);
    return;
}

char* createProcessPrameters (int value){
    std::string valueString = std::to_string(value);
    char* valueParam = new char [valueString.length()+1];
    std::strcpy(valueParam,valueString.c_str());
    valueParam[valueString.length()]='\0';
    return valueParam;
}