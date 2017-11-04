#include "queueUtilities.h"
#include "clkUtilities.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <fstream>
#include <queue>


#define FILE_NAME "scheduler.log"

std::ofstream logFile;
std::queue <processData> recievedProcesses;
bool processGeneratorFinish = false;
pid_t currentProcessId = -1;
std::priority_queue <processData> processes;
std::deque<processData> processesRR;
bool processRunning = false;
char currentAlgorithm;
processData previousAddedProcess;


//utility functions
void saveDeadProcessData (processData process);
char* createProcessPrameters (int value);
void logNewProcess(processData process);

//SRT functions
void newProcessHandlerSRT(int signal);
void SRTScheduler ();
void createNewProcessSRT();
void deadProcessHandlerSRT(int signal);

//HPF functions
void HPFScheduler ();
void getNewProcessesFromProcessGenerator();
void createNewProcess();
void deadChildHandlerHPF(int signal);

//RR functions
void RRscheduler(int quanta);
void deadProcessHandlerRR (int signal);
void quantaHandlerRR (int signal);
//void handlerRR(int);

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
            signal(SIGCHLD, deadChildHandlerHPF);
            HPFScheduler();
            //TODO claculate the calculations
            break;
        }
        case '2': {
            printf("using shortest time remainder\n");
            signal(SIGUSR1, newProcessHandlerSRT);
            signal(SIGCHLD, deadProcessHandlerSRT);
            SRTScheduler();
            //TODO claculate the calculations
            break;
        }
        case '3': {
            printf("using RR with quanta %d\n", std::atoi(argv[2]));
            signal(SIGALRM, quantaHandlerRR);
            signal(SIGCHLD, deadProcessHandlerRR);
            RRscheduler(std::atoi(argv[2]));
            break;
        }
        default: {
            printf("wrong arguments\n");
        }
    }
    printf("scheduler is exiting\n");
    return 0;
}


//SRT Function definetions
////The scheduler main loop
void SRTScheduler (){
    while (! processGeneratorFinish || ! recievedProcesses.empty() || ! processes.empty()){
        //FOR TEST
        printf("i am in the while time %d\n",getClk());
        int processStatus;
        //Add the revieved processes to the queue of processes
        processData lastAddedProcess;
        while (! recievedProcesses.empty()){
            processData temp = recievedProcesses.front();
            recievedProcesses.pop();
            //Check if the new process is added before to not add it again
            if (temp.id != lastAddedProcess.id) {
                processes.push(temp);
            }
            lastAddedProcess=temp;
        }
        if (! processes.empty()){
            //run the process with the right turn
            processData temp = processes.top();
            if (temp.processId == -1){
                //Updating the process data (PCB)
                temp.waitingTime = getClk() - temp.arrivingTime;
                temp.startRunningTime= getClk();
                //Log the new process
                logNewProcess(temp);
                processes.pop();
                processes.push(temp);
                //Fork the new process
                createNewProcessSRT();
            } else {
                currentProcessId = temp.processId;
                temp.startRunningTime = getClk();
                processes.pop();
                processes.push(temp);
                logFile.open(FILE_NAME, std::fstream::app);
                if (! logFile.is_open()){
                    printf("can't open the file\n");
                    exit(1);
                }
                logFile << "At time "<< getClk()<< " process " << temp.id << " resumed arr "
                        << temp.arrivingTime << " total "<< temp.fullRunningTime <<" remain "
                        << temp.remainingTime <<" wait " << getClk() - temp.arrivingTime << std::endl;
                logFile.close();
                kill(currentProcessId,SIGCONT);
                pause();
            }
        }
        if (! processGeneratorFinish || ! recievedProcesses.empty() || ! processes.empty())
        {
            pause();
        }
    }
}

////The handler of SIGCHLD sent by the process upon termination or any state change
void deadProcessHandlerSRT(int signal) {

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
        printf("the process id %d %d stopped or resummed \n", processes.top().id, currentProcessId);
    } else {
        printf("something is wrong in the deadhandler %d\n", triggeredProcessId);
    }

}

////Fork and exec the new process with the proper parameters
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
        }
    }
    temp.processId = currentProcessId;
    processes.push(temp);
    return ;
}

////The handler of SIGURS1 sent by the process generator as a signal after adding new process to the shared queue
////And stop the current running process and save its status and update it in the process queue
void newProcessHandlerSRT(int signal){
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
                kill(currentProcessId, SIGSTOP);
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

//HPF Function definetions
////The scheduler main loop
void HPFScheduler(){
    while (! processGeneratorFinish || !processes.empty()){
        processData process;
        int result = Recmsg(process);
        if (result == 0) {
            //hna lma el result btrg3 -1 bardo byfdl fel queue a5er process fel file leeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeh ???????????????
            if (process.id != previousAddedProcess.id) {
                processes.push(process);
                previousAddedProcess = process;
                printf("received id %d priority %d arr time %d and clk %d \n", process.id, process.priority,
                       process.arrivingTime, getClk());
            }
            getNewProcessesFromProcessGenerator();
        } else if (result == 1){
            processGeneratorFinish = true;
            printf("lastSend recieved\n");
        }
        if (! processes.empty()) {
            createNewProcess();
            pause();
        }
    }
}

////Add the new processes to the ready queue
void getNewProcessesFromProcessGenerator() {
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
            if (temp.id != previousAddedProcess.id) {
                printf("received id %d priority %d arr time %d and clk %d \n", temp.id, temp.priority,
                       temp.arrivingTime, getClk());
                if(currentAlgorithm=='1'){
                    processes.push(temp);
                }else {
                    processesRR.push_back(temp);
                }
                previousAddedProcess = temp;
            }
        }
        result = Recmsg(temp);
    }while (result != -1);
    printf("ending the new process handler\n");
    return;
}

void createNewProcess(){
    processRunning = true;
    processData temp;
    if(currentAlgorithm == '1') {
        temp = processes.top();
        processes.pop();
    }else if(currentAlgorithm == '3')
    {
        temp = processesRR.front();
        processesRR.pop_front();
    }
    currentProcessId = fork();
    if (currentProcessId == -1 ){
        perror("can't fork new process\n");
    }else if (currentProcessId == 0){
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
    temp.startRunningTime = getClk();
    temp.waitingTime = getClk() - temp.arrivingTime;
    if(currentAlgorithm == '1') {
        processes.push(temp);
    }else if(currentAlgorithm =='3'){
        processesRR.push_front(temp);
    }
    logNewProcess(temp);
    printf("starting new process id %d, priority %d, arr %d, clk %d\n",temp.id,temp.priority,temp.arrivingTime,getClk());
}

void deadChildHandlerHPF(int signal) {
    processRunning = false;
    processData process = processes.top();
    processes.pop();
    printf("process id %d terminated clk %d\n",process.id, getClk());
    process.remainingTime=0;
    process.finsihTime=getClk();
    logFile.open(FILE_NAME, std::fstream::app);
    if (! logFile.is_open()){
        printf("can't open log file\n");
        exit(1);
    }
    logFile << "At time " << getClk() << " process " << process.id << " finished arr "
            << process.arrivingTime << " total " << process.fullRunningTime << " remain "
            << process.remainingTime << " wait " << process.waitingTime << std::endl;
    logFile.close();
    //TODO save the data needed for the calculations
    //saveDeadProcessData(process);
}

//RR functions
void RRscheduler(int quanta){
    while(!processGeneratorFinish || !processesRR.empty())
    {
        processData newProcess;
        int result=Recmsg(newProcess);
        if(result == 0 && !processGeneratorFinish) {
            if (newProcess.id != previousAddedProcess.id) {
                processesRR.push_back(newProcess);
                previousAddedProcess = newProcess;
                printf("received id %d priority %d arr time %d and clk %d \n", newProcess.id, newProcess.priority,
                       newProcess.arrivingTime, getClk());
            }
            getNewProcessesFromProcessGenerator();
        }else if(result == 1) {
            processGeneratorFinish=true;
            printf("lastSend recieved \n");
        }

        if(!processesRR.empty()){
            //create new process
            processData temp = processesRR.front();
            if (temp.processId == -1) {
                createNewProcess();
            } else {
                currentProcessId = temp.processId;
                printf("process %d is resuming with remainder time %d at %d \n",temp.id ,temp.remainingTime, getClk());
                kill(currentProcessId,SIGCONT);
                temp.startRunningTime = getClk();
                processesRR.pop_front();
                processesRR.push_front(temp);
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
            // if the remaining time is less than the quanta don't set the alarm to avoid the condition where the
            // quanta = reamaing time which will cause signal collision between SIGCHLD and SIGALRM which leads to
            // unexpected behaviour
            if (temp.remainingTime > quanta) {
                alarm(STEP_TIME * quanta);
                pause();
            }
            if (currentProcessId != -1) {
                pause();
            } else {
                printf("pausing when i shouldn't\n");
            }
        }
    }
}

void deadProcessHandlerRR (int signal){
    processData currentProcess = processesRR.front();
    int status;
    int triggeredProcessId = waitpid(currentProcessId, &status, WNOHANG);
    if (triggeredProcessId == currentProcessId) {
        alarm(0);
        logFile.open(FILE_NAME, std::fstream::app);
        if (! logFile.is_open()){
            printf("can't open log file\n");
            exit(1);
        }
        logFile << "At time " << getClk() << " process " << currentProcess.id << " finished arr "
                << currentProcess.arrivingTime << " total " << currentProcess.fullRunningTime << " remain "
                << currentProcess.remainingTime << " wait " << currentProcess.waitingTime << std::endl;
        logFile.close();
        if (currentProcess.processId != currentProcessId) {
            printf("killing something wrong queue process id = %d id %d currentProcessId %d\n",
                   currentProcess.processId,
                   currentProcess.id,
                   currentProcessId);
        } else {
            printf("I am doing the right thing \n");
        }
        saveDeadProcessData(currentProcess);
        processesRR.pop_front();
    } else{
        printf("the child is either stopping or cont\n");
    }
}

void quantaHandlerRR (int signal){
    processData currentProcess = processesRR.front();
    int status;
    logFile.open(FILE_NAME, std::fstream::app);
    if (! logFile.is_open()){
        printf("can't open the file\n");
        exit(1);
    }
    currentProcess.remainingTime = currentProcess.remainingTime - (getClk() - currentProcess.startRunningTime);
    processesRR.pop_front();
    processesRR.push_back(currentProcess);
    logFile << "At time "<< getClk()<< " process " << currentProcess.id << " stopped arr "
            << currentProcess.arrivingTime << " total "<< currentProcess.fullRunningTime <<" remain "
            << currentProcess.remainingTime <<" wait " << currentProcess.waitingTime << std::endl;
    logFile.close();
    printf("the process %d is stopping\n",currentProcess.id);
    kill(currentProcessId, SIGSTOP);
    //currentProcessId=-1;
}

/*void handlerRR(int signal) {

    //if(currentProcessId==-1)return;
    processData currentProcess = processesRR.front();
    int status;
    if (signal == SIGCHLD) {
        int triggeredProcessId = waitpid(currentProcessId, &status, WNOHANG);
        if (triggeredProcessId == currentProcessId) {
            alarm(0);
            logFile.open(FILE_NAME, std::fstream::app);
            if (! logFile.is_open()){
                printf("can't open log file\n");
                exit(1);
            }
            logFile << "At time " << getClk() << " process " << currentProcess.id << " finished arr "
                    << currentProcess.arrivingTime << " total " << currentProcess.fullRunningTime << " remain "
                    << currentProcess.remainingTime << " wait " << currentProcess.waitingTime << std::endl;
            logFile.close();
            if (currentProcess.processId != currentProcessId) {
                printf("killing something wrong queue process id = %d id %d currentProcessId %d\n",
                       currentProcess.processId,
                       currentProcess.id,
                       currentProcessId);
            } else {
                printf("I am doing the right thing \n");
            }
            saveDeadProcessData(currentProcess);
            processesRR.pop_front();
        } else{
            printf("the child is either stopping or cont\n");
        }
    }else if(signal==SIGALRM){
        logFile.open(FILE_NAME, std::fstream::app);
        if (! logFile.is_open()){
            printf("can't open the file\n");
            exit(1);
        }
        currentProcess.remainingTime = currentProcess.remainingTime - (getClk() - currentProcess.startRunningTime);
        processesRR.pop_front();
        processesRR.push_back(currentProcess);
        currentProcess = processesRR.back();
        logFile << "At time "<< getClk()<< " process " << currentProcess.id << " stopped arr "
                << currentProcess.arrivingTime << " total "<< currentProcess.fullRunningTime <<" remain "
                << currentProcess.remainingTime <<" wait " << currentProcess.waitingTime << std::endl;
        logFile.close();
        kill(currentProcessId, SIGTSTP);
        currentProcessId=-1;
    }
}*/

////Utility function to create the parameters sent to the new process
char* createProcessPrameters (int value){
    std::string valueString = std::to_string(value);
    char* valueParam = new char [valueString.length()+1];
    std::strcpy(valueParam,valueString.c_str());
    valueParam[valueString.length()]='\0';
    return valueParam;
}

////Utility function to Save the data of the process before destroying it
void saveDeadProcessData (processData process){
    //TODO save the required data
}

////Utility function to write in the log file the data of the new forked process
void logNewProcess(processData process) {
    logFile.open(FILE_NAME, std::fstream::app);
    if (! logFile.is_open()){
        printf("can't open log file\n");
        exit(1);
    }
    logFile << "At time "<< getClk()<< " process " << process.id << " started arr "
            << process.arrivingTime << " total "<< process.fullRunningTime <<" remain "
            << process.remainingTime <<" wait " << process.waitingTime << std::endl;
    logFile.close();
}