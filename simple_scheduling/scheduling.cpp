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

struct msg_buffer {     // message buffer for IPC
    long msg_type;
    char msg_txt[100];
};

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

DoublyLinkedList *rq_front, *rq_rear;    // ready queue (= run queue)
DoublyLinkedList *wq_front, *wq_rear;    // waiting queue (= i/o queue)

int msg_que_id;

void initialize(key_t& main_key) {
    // make ready, waiting, i/o queues
    rq_front = new DoublyLinkedList;
    rq_rear = new DoublyLinkedList;
    wq_front = new DoublyLinkedList;
    wq_rear = new DoublyLinkedList;

    rq_front->right = rq_rear;
    rq_rear->left = rq_front;
    wq_front->right = wq_rear;
    wq_rear->left = wq_front;

    // create message queue
    main_key = ftok(".", 'a');  // reference - http://jullio.pe.kr/cs/lpg/lpg_6_4_1.html
    msg_que_id = msgget(main_key, IPC_CREAT|0666);
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
            child_list[i] = createChild(main_pid);
            if(getpid() == main_pid) {
                // parent process
                //std::cout << i << "th process created, " << child_list[i]->pid << std::endl;
            }
        } else {
            // child process
            //break;
            //std::cout << getpid() << ' ' << i << std::endl;
        }
    }
    //std::cout << "hi from " << getpid() << std::endl;
}

void timer(int signum) {
    if(signum == SIGALRM) {
        printf("tt.");
    }
}

void parentProcess() {
    //  parent process TODOs
    //  1. receive alarm signal(SIGALRM) by timer
    //  2. maintain run-queue(=ready queue, rq), wait-queue(=i/o queue, wq)
    //  3. perform scheduling of child processes
    //      3-1. check remain burst time
    //      3-2. sending IPC message using message queue (give time clice to child process)
    //      3-3. using IPC_NOWAIT flag (non-blocking mode, reference - https://blog.naver.com/gaechuni/221156537840)
    //  4. check all waiting time of child processes.
    //
    //  (Opt.)
    //  5. receive IPC messages from child process, check if child process begins I/O burst.
    //  6. then, take out of ready queue, and moves to i/o waiting queue. (CANNOT be scheduled until I/O completion)
    //  7. parent process saves I/O burst time of each child processes.
    //  8. decrease time of all processes in I/O queue (waiting queue).
    //  9. ++ multiple queue (MLFQ based on priority)

    std::cout << "here is parent" << std::endl;
}

void childProcess() {       
    //  child process TODOs
    //  1. infinite-loop
    //  2. receive IPC messages from parent process, decrease CPU burst time.
    //
    //  (Opt.)
    //  3. simulate I/O waiting process.

    std::cout << "here is child " << getpid() << std::endl;
}


int main() {
    pid_t main_pid = getpid();
    std::cout << "main is " << main_pid << std::endl;

    key_t main_key;
    initialize(main_key);

    // set timer
    signal(SIGALRM, timer);

    // random number generator - reference: https://modoocode.com/304
    std::random_device rd0;
    std::mt19937 gen(rd0());
    std::uniform_int_distribution<int> rand_distrib(0, 999);

    createChildren();
    if(getpid() == main_pid) {
        // parent process
        wait(NULL);
        /*
        for(int i=0;i<10;++i) {
            std::cout << child_list[i]->pid << ' ';
        }
        std::cout << std::endl;
*/
        parentProcess();
    } else {
        // child process
        //std::cout << getpid() << std::endl;

        childProcess();
    }

    return 0;
}