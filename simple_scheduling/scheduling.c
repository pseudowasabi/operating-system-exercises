// OS Term Project #1 by Yoseob Kim
// Simple scheduling (with Round-Robin)
// Environments: MBP(13-inch, 2017, Two Thunderbolt 3 ports, Intel Core i5)
//                  (https://support.apple.com/kb/SP754?locale=ko_KR)
//                  (os version: macOS Catalina (10.15.6))

// Headers from ./signaling.c and ./message_passing.c
// Reference - https://www.geeksforgeeks.org/ipc-using-message-queues/
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint32;
typedef unsigned long long uint64;

// reference - Operating system concepts textbook (8/e International edition, p106)
typedef struct _task_struct { // process control block
    pid_t pid;
    uint8 state, priority;
    uint16 cpu_burst_time, io_burst_time;   // like time_slice

    pid_t parent, child[10];
    uint8 child_count;
} pcb;

uint16 time_quantum;

struct my_task_struct* createChild() {
    pcb *test = malloc(sizeof(pcb));
    test->cpu_burst_time = 450;
    test->io_burst_time = 20;
    test->priority = 1;

    return test;
}


int main() {
    pcb *test1 = createChild();

    printf("%u %u %u", test1->cpu_burst_time, test1->io_burst_time, test1->priority);

    return 0;
}