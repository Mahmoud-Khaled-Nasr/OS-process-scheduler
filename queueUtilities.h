
#include <stdio.h>      //if you don't use scanf/printf change this include
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define QUEUEKEY 777
#define ENDTYPE 19L

///==============================
//don't mess with this variable//
int qid;
//===============================

struct processData {

    enum processStat {
        started,finished,stoped,resumed,notstarted
    };

    int id, priority, fullRunningTime, arrivingTime, remainingTime, finsihTime, waitingTime, currentUsedAlgo;
    processStat stat;
    pid_t processId;
    processData(){ id = -1; }
    processData(int id, int priority, int arrivingTime, int fullRunningTime,int remainingTime,int finsihTime
            ,int waitingTime, int algo)
            :id(id),priority(priority), arrivingTime(arrivingTime),fullRunningTime(fullRunningTime),
             remainingTime(remainingTime),finsihTime(finsihTime),waitingTime(waitingTime),processId(-1),currentUsedAlgo(algo)
    {
        stat= notstarted;
    }

    bool operator < (const processData &b) const {
        if (currentUsedAlgo == 1)
            return priority < b.priority;
        else if (currentUsedAlgo == 2) {
            if (remainingTime == b.remainingTime) {
                return id > b.id;
            } else {
                return remainingTime > b.remainingTime;
            }
        }
    }
};

struct messagebuffer
{
 long mtype;
 struct processData data;
 
};


void initQueue(bool flag) {
  if(flag) {
    qid= msgget(QUEUEKEY,IPC_CREAT  |   0644);
    if(qid == -1)
    {
        perror("queue not created");
    }
  }

  else
  {
     qid = msgget(QUEUEKEY, 0444);
    if(qid == -1)
      {
            //Make sure that the Clock exists
          printf("Error, queue not initialized yet\n");
          kill(getpgrp(),SIGINT);
      }
  }
  
}



int Sendmsg(struct processData pData) {
  struct messagebuffer msg;
  msg.data = pData;
  msg.mtype = 1L;
  return msgsnd(qid, &msg, sizeof(msg)-sizeof(long), !IPC_NOWAIT);
}


//return -1 when failure or no processes available
//      0 when on sccessfull process recive
//      1 when lastSend
int Recmsg( processData &pData) {
  struct messagebuffer msg;
  msg.mtype = 1L;
  int ret=msgrcv(qid,&msg,sizeof(msg)-sizeof(long),0,IPC_NOWAIT);
  pData=msg.data;
  if (ret == -1)
    return -1;
  if(msg.mtype == ENDTYPE)
      return  1;
  return 0;
  
}

void lastSend() {
  struct messagebuffer msg;
  msg.mtype = ENDTYPE;
  msgsnd(qid, &msg, sizeof(msg)-sizeof(long), !IPC_NOWAIT);
}

void destroyQueueAndExit(int)
{
    msgctl(qid, IPC_RMID, (struct msqid_ds*)0); 
    exit(0);
}


