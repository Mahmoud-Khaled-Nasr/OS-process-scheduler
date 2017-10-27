#include "clkUtilities.h"
#include "queueUtilities.h"
#include <cstdio>
#include <iostream>

int main(int argc, char* argv[]) {
    initQueue(false);
    initClk();

    switch (*argv[1]){
        case '1':
            printf("using HPF\n");
            break;
        case '2':
            printf("using shortest time remainder\n");
            break; 
        case '3':
            printf("using RR\n");
            break;
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
