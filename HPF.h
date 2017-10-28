#pragma once
#include "queueUtilities.h"
#include "clkUtilities.h"
#include <queue>
#include <cstdio>
#include <unistd.h>
#include <cstring>

#define FILE_NAME "scheduler.log"
struct processData;

class HPF {
private:
    bool processGeneratorFinish;
    pid_t currentProcessId;
    std::priority_queue <processData> processes;
    std::ofstream logFile;
    void HPFScheduler ();
    void getNewProcessesFromProcessGenerator();
    void createNewProcess();
    void logNewProcess( processData process );
    char* createProcessPrameters (int value);
    void deleteDeadProcess();
    void logDeadProcess(processData process);
    void saveData(processData process);
    static void deadChildHandler(int signal);
public:
    HPF();
    ~HPF();
};