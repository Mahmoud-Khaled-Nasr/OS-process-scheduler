#include "HPF.h"

HPF::HPF() {
    printf("i am in the constructor\n");
    logFile.open(FILE_NAME);
    if (! logFile.is_open()){
        printf("can't open the log file\n");
        exit(1);
    }
    processGeneratorFinish = false;
    currentProcessId = -1;
    signal(SIGCHLD,HPF::deadChildHandler);
    HPFScheduler();
}

//TODO get the finishing time value when the process ends
void HPF::HPFScheduler(){
    while (! processGeneratorFinish || !processes.empty()){
        getNewProcessesFromProcessGenerator();
        createNewProcess();
        if (! processGeneratorFinish || !processes.empty()) {
            pause();
            deleteDeadProcess();
        }
    }
}

void HPF::getNewProcessesFromProcessGenerator() {
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
            printf("received id %d priority %d arr time %d and clk %d\n", temp.id, temp.priority, temp.arrivingTime, getClk());
            processes.push(temp);
        }
        result = Recmsg(temp);
    }while (result != -1);
    return;
}

void HPF::createNewProcess(){
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
        } else if (result == 0) {
            perror("sucess exec the new process\n");
        }
    }
    temp.processId = currentProcessId;
    temp.currentUsedAlgo = 1;
    temp.startRunningTime = getClk();
    temp.waitingTime = getClk() - temp.arrivingTime;
    processes.push(temp);
    logNewProcess(temp);
    printf("starting new process id %d, priority %d, arr %d, clk %d",temp.id,temp.priority,temp.arrivingTime,getClk());
}

char *HPF::createProcessPrameters(int value) {
    std::string valueString = std::to_string(value);
    char* valueParam = new char [valueString.length()+1];
    std::strcpy(valueParam,valueString.c_str());
    valueParam[valueString.length()]='\0';
    return valueParam;
}

void HPF::logNewProcess(processData process) {
    logFile << "At time "<< getClk()<< " process " << process.id << " started arr "
            << process.arrivingTime << " total "<< process.fullRunningTime <<" remain "
            << process.remainingTime <<" wait " << process.waitingTime << std::endl;
}

void HPF::deadChildHandler(int signal) {
    printf("a child died\n");
}

void HPF::deleteDeadProcess() {
    processData process = processes.top();
    processes.pop();
    process.remainingTime=0;
    process.finsihTime=getClk();
    logDeadProcess(process);
    //TODO save the data needed for the calculations
    saveData(process);
}

void HPF::logDeadProcess(processData process) {
    logFile << "At time " << getClk() << " process " << process.id << " finished arr "
            << process.arrivingTime << " total " << process.fullRunningTime << " remain "
            << process.remainingTime << " wait " << process.waitingTime << std::endl;
}

void HPF::saveData(processData process) {

}

HPF::~HPF() {

}
