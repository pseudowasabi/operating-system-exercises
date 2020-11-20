// OS Term Project #1 by Yoseob Kim
// Simple scheduling (with Round-Robin)
// Environments: MBP(13-inch, 2017, Two Thunderbolt 3 ports, Intel Core i5)
//                  (https://support.apple.com/kb/SP754?locale=ko_KR)
//                  (os version: macOS Catalina (10.15.6))

// Headers from ./signaling.c and ./message_passing.c
// Reference - https://www.geeksforgeeks.org/ipc-using-message-queues/
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include <cstring>
#include <cstdlib>
#include <iostream>
#include <random>

// reference - Operating system concepts textbook (8/e International edition, p106)
class TaskStruct { // process control block
public:
    pid_t pid, parent_pid;
    unsigned int state, priority;
    unsigned int cpu_burst_time, io_burst_time;   // like time_slice;
    unsigned int child_count;

    TaskStruct(pid_t pid, pid_t main_pid) {
        this->pid = pid;
        this->parent_pid = main_pid;
    }
};

class DoublyLinkedList {
public:
    DoublyLinkedList *left, *right;
    TaskStruct *process_info;

    DoublyLinkedList() {
        left = right = nullptr;
    }
};

DoublyLinkedList *rq_front, *rq_rear;    // ready queue
DoublyLinkedList *wq_front, *wq_rear;    // waiting queue
DoublyLinkedList *ioq_front, *ioq_rear;  // i/o queue

void initialize() {
    // make ready, waiting, i/o queues
    rq_front = new DoublyLinkedList;
    rq_rear = new DoublyLinkedList;
    wq_front = new DoublyLinkedList;
    wq_rear = new DoublyLinkedList;
    ioq_front = new DoublyLinkedList;
    ioq_rear = new DoublyLinkedList;

    rq_front->right = rq_rear;
    rq_rear->left = rq_front;
    wq_front->right = wq_rear;
    wq_rear->left = wq_front;
    ioq_front->right = ioq_rear;
    ioq_rear->left = ioq_front;
}

TaskStruct* child_list[10];
unsigned int time_quantum;

TaskStruct* createChild(pid_t parent_pid) {
    pid_t pid = fork();
    TaskStruct *pcb = new TaskStruct(pid, parent_pid);

    pcb->cpu_burst_time = 300;
    pcb->io_burst_time = 20;

    DoublyLinkedList *ddl = new DoublyLinkedList;
    ddl->process_info = pcb;

    // rl->ddl->r
    rq_rear->left->right = ddl;
    rq_rear->left = ddl;
    ddl->left = rq_rear->left;
    ddl->right = rq_rear;

    return pcb;
}

void createChildren() {
    pid_t main_pid = getpid();

    for(int i=0;i<10;++i) {
        if(getpid() == main_pid) {
            // parent process
            //if(i < 10 && getpid() == main_pid) {
            child_list[i] = createChild(main_pid);
            //}
            if(getpid() == main_pid) {
                // parent process
                std::cout << i << "th process created, " << child_list[i]->pid << std::endl;
            }
        } else {
            // child process
            //break;
            std::cout << getpid() << ' ' << i << std::endl;
        }
    }

    std::cout << "hi from " << getpid() << std::endl;
}

int main() {
    initialize();

    // random number generator - reference: https://modoocode.com/304
    std::random_device rd0;
    std::mt19937 gen(rd0());
    std::uniform_int_distribution<int> rand_distrib(0, 999);

    pid_t main_pid = getpid();
    std::cout << "main is " << main_pid << std::endl;

    createChildren();
    if(getpid() == main_pid) {
        // parent process
        for(int i=0;i<10;++i) {
            std::cout << child_list[i]->pid << ' ';
        }
        std::cout << std::endl;
    } else {
        // child process
        std::cout << getpid() << std::endl;
    }
    return 0;
}