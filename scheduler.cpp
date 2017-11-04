#include "queueUtilities.h"
#include "clkUtilities.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <fstream>
#include <queue>
#include <iomanip>
#include <vector>
#include <math.h>

#define FILE_NAME "scheduler.log"
#define STATISTIC_FILE_NAME "scheduler.perf"

//The log file stream
std::ofstream logFile;
//Temp queue to recieve the processes in the signal handler to avoid confilct in the signal handler of SRT
std::queue <processData> recievedProcesses;
//booleam = true if the lastSend function is triggered in the processGenerator
bool processGeneratorFinish = false;
//The process id in action either by running or stopping
pid_t currentProcessId = -1;
//The ready queue of both SRT and HPF because they are have to be sorted
std::priority_queue <processData> processes;
//The ready queue of RR because of double insertion needed for updating process struct data
std::deque<processData> processesRR;
//Char representing the current used algorithm
char currentAlgorithm;
//Temp variable to check for redundant insertion in the ready queue
processData previousAddedProcess;
//For statistics
int totalTurnAroundTime =0, totalWaitingTime=0, finalFinishTime, totalNumberOfProcesses = 0;
double totalWeightedTurnAroundTime=0;
//Vector to store the WTA of all the processes
std::vector<double>WTAOfAllProcesses;

//utility functions
void saveDeadProcessData (processData process);
char* createProcessPrameters (int value);
void logNewProcess(processData process);
void logFinishedProcess (processData process);
void logResumedProcess (processData process);
void logStoppedProcess (processData process);
void printStatistics ();

//SRT functions
void processGeneratorSignalHandlerSRT(int signal);
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
            break;
        }
        case '2': {
            printf("using shortest time remainder\n");
            signal(SIGUSR1, processGeneratorSignalHandlerSRT);
            signal(SIGCHLD, deadProcessHandlerSRT);
            SRTScheduler();
            break;
        }
        case '3': {
            printf("using RR with quanta %d\n", std::atoi(argv[2]));
            signal(SIGCHLD, deadProcessHandlerRR);
            RRscheduler(std::atoi(argv[2]));
            break;
        }
        default: {
            printf("wrong arguments\n");
        }
    }
    printStatistics();
    printf("scheduler is exiting\n");
    return 0;
}


//SRT Function definetions
////The scheduler main loop
void SRTScheduler (){
    while (! processGeneratorFinish || ! recievedProcesses.empty() || ! processes.empty()){
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
                temp.waitingTime += getClk() - temp.finsihTime;
                processes.pop();
                processes.push(temp);
                logResumedProcess(temp);
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
        }
        temp.finsihTime = getClk();
        temp.remainingTime = 0;
        logFinishedProcess(temp);
        saveDeadProcessData(temp);
        currentProcessId = -1;
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
void processGeneratorSignalHandlerSRT(int signal){
    processData temp;
    int result=2;
    do {
        if (result == -1) {
        } else if (result == 1) {
            processGeneratorFinish = true;
            printf("lastSend recieved\n");
        } else if (result == 0 && ! processGeneratorFinish) {
            recievedProcesses.push(temp);
            if (currentProcessId != -1) {
                kill(currentProcessId, SIGSTOP);
                processData temp = processes.top();
                processes.pop();
                temp.remainingTime = temp.remainingTime - (getClk() - temp.startRunningTime);
                temp.finsihTime = getClk();
                processes.push(temp);
                logStoppedProcess(temp);
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
    int result=2;
    do {
        if (result == -1) {
            printf("Can't recieve massage for process generator or no massages to be received\n");
        } else if (result == 1) {
            processGeneratorFinish = true;
            printf("lastSend recieved\n");
        } else if (result == 0 && ! processGeneratorFinish) {
            if (temp.id != previousAddedProcess.id) {
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
    return;
}

////Fork New processes for both RR and HPF
void createNewProcess(){
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
}

////HPF SIGCHLD Handler
////The function remove the finished process from the ready queue and call the log function
void deadChildHandlerHPF(int signal) {
    processData process = processes.top();
    processes.pop();
    process.remainingTime=0;
    process.finsihTime=getClk();
    logFinishedProcess(process);
    saveDeadProcessData(process);
}

//RR functions
////The scheduler main loop
void RRscheduler(int quanta){
    while(!processGeneratorFinish || !processesRR.empty())
    {
        //Check for new processes sent by the processGenerator
        processData newProcess;
        int result=Recmsg(newProcess);
        if(result == 0 && !processGeneratorFinish) {
            if (newProcess.id != previousAddedProcess.id) {
                processesRR.push_back(newProcess);
                previousAddedProcess = newProcess;
            }
            getNewProcessesFromProcessGenerator();
        }else if(result == 1) {
            processGeneratorFinish=true;
            printf("lastSend recieved \n");
        }

        //Run the process in turn
        if(!processesRR.empty()){
            //create new process
            processData temp = processesRR.front();
            if (temp.processId == -1) {
                createNewProcess();
                temp = processesRR.front();
            } else {
                temp = processesRR.front();
                currentProcessId = temp.processId;
                kill(currentProcessId,SIGCONT);
                pause();
                temp.startRunningTime = getClk();
                temp.waitingTime += getClk() - temp.finsihTime;
                logResumedProcess(temp);
                processesRR.pop_front();
                processesRR.push_front(temp);
            }
            // if the remaining time is less than the quanta don't set the alarm to avoid the condition where the
            // quanta = reamaing time which will cause signal collision between SIGCHLD and SIGALRM which leads to
            // unexpected behaviour
            sleep(STEP_TIME * quanta);
            if (currentProcessId == processesRR.front().processId) {
                kill(currentProcessId, SIGSTOP);
                pause();
                processData currentProcess = processesRR.front();
                int status;
                currentProcess.remainingTime =
                        currentProcess.remainingTime - (getClk() - currentProcess.startRunningTime);
                currentProcess.finsihTime = getClk();
                logStoppedProcess(currentProcess);
                processesRR.pop_front();
                processesRR.push_back(currentProcess);
            }
        }
    }
}

////RR SIGCHLD Handler
////The function remove the finished process from the ready queue and call the log function
void deadProcessHandlerRR (int signal){
    processData currentProcess = processesRR.front();
    int status;
    //Check for the reason of the SIGCHLD signal
    int triggeredProcessId = waitpid(currentProcessId, &status, WNOHANG);
    //The signal is from a terminated process
    if (triggeredProcessId == currentProcessId) {
        alarm(0);
        currentProcess.remainingTime = 0;
        currentProcess.finsihTime = getClk();
        logFinishedProcess(currentProcess);
        saveDeadProcessData(currentProcess);
        if (currentProcess.processId != currentProcessId) {
            printf("killing something wrong queue process id = %d id %d currentProcessId %d\n",
                   currentProcess.processId,
                   currentProcess.id,
                   currentProcessId);
        }
        processesRR.pop_front();
    }
}

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
    int TA = process.finsihTime - process.arrivingTime;
    totalTurnAroundTime+=TA;
    totalWeightedTurnAroundTime+=TA / (double)process.fullRunningTime;
    totalWaitingTime+=process.waitingTime;
}

////Utility function to write in the log file the data of the new forked process
void logNewProcess(processData process) {
    totalNumberOfProcesses ++;
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

////Utility function to Log the finished processes and calculate TA and WTA
void logFinishedProcess (processData process){
    int TA = process.finsihTime - process.arrivingTime;
    double WTA = TA / (double) process.fullRunningTime;
    finalFinishTime = process.finsihTime;
    WTAOfAllProcesses.push_back(WTA);
    logFile.open(FILE_NAME, std::fstream::app);
    if (!logFile.is_open()) {
        printf("can't open the file\n");
        exit(1);
    }
    logFile << "At time " << getClk() << " process " << process.id << " finished arr "
            << process.arrivingTime << " total " << process.fullRunningTime << " remain "
            << process.remainingTime << " wait " << process.waitingTime << " TA " << TA
            << " WTA " << std::setprecision(2) << WTA << std::endl;
    logFile.close();
}

////Utility function to log the updated processes
void logResumedProcess (processData process){
    logFile.open(FILE_NAME, std::fstream::app);
    if (! logFile.is_open()){
        printf("can't open the file\n");
        exit(1);
    }
    logFile << "At time "<< getClk()<< " process " << process.id << " resumed arr "
            << process.arrivingTime << " total "<< process.fullRunningTime <<" remain "
            << process.remainingTime <<" wait " << process.waitingTime << std::endl;
    logFile.close();
}

////Utility function to log the stopped processes
void logStoppedProcess (processData process){
    logFile.open(FILE_NAME, std::fstream::app);
    if (! logFile.is_open()){
        printf("can't open the file\n");
        exit(1);
    }
    logFile << "At time "<< getClk()<< " process " << process.id << " stopped arr "
            << process.arrivingTime << " total "<< process.fullRunningTime <<" remain "
            << process.remainingTime <<" wait " << process.waitingTime << std::endl;
    logFile.close();
}

//Print amd calculate the final statistics
void printStatistics (){
    printf("writing Statistics\n");
    double AvgWTA = totalWeightedTurnAroundTime/totalNumberOfProcesses;
    double WTAStandardDiviation = 0, sum =0;
    for (int i = 0; i < WTAOfAllProcesses.size(); ++i) {
        sum += (AvgWTA - WTAOfAllProcesses[i]) * (AvgWTA - WTAOfAllProcesses[i]);
    }
    WTAStandardDiviation= sqrt( sum / totalNumberOfProcesses );
    std::ofstream statisticFile;
    statisticFile.open(STATISTIC_FILE_NAME);
    if (! statisticFile.is_open()){
        printf("can't open the file\n");
        exit(1);
    }
    statisticFile << "CPU utilization=" << (finalFinishTime*100.0)/getClk() <<"%\n"
        << "Avg WTA="<< std::setprecision(2) << AvgWTA <<std::endl
        << "Avg Waiting=" << std::setprecision(2) << totalWaitingTime/totalNumberOfProcesses<<std::endl
        << "std WTA="<< std::setprecision(2)<< WTAStandardDiviation<<std::endl;
}