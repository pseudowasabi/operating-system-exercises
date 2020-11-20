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
#include <sys/time.h>

#include <cstring>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
using namespace std;

struct msg_buffer {     // message buffer for IPC
    long msg_type;      // set msg_type as pid of destination process.
    char msg_txt[100];
};

// reference - Operating system concepts textbook (8/e International edition, p106)
class TaskStruct { // process control block
public:
    pid_t pid, parent_pid;
    unsigned int state, priority;
    unsigned int cpu_burst_time, io_burst_time;

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
unsigned int time_quantum;

void initialize(key_t& main_key) {
    // 1. make ready, waiting(i/o) queues
    rq_front = new DoublyLinkedList;
    rq_rear = new DoublyLinkedList;
    wq_front = new DoublyLinkedList;
    wq_rear = new DoublyLinkedList;

    rq_front->right = rq_rear;
    rq_rear->left = rq_front;
    wq_front->right = wq_rear;
    wq_rear->left = wq_front;

    // 2. create message queue
    // reference - http://jullio.pe.kr/cs/lpg/lpg_6_4_1.html
    main_key = ftok(".", 'a');
    msg_que_id = msgget(main_key, IPC_CREAT|0666);

    // 3. set time quantum (unit: ms)
    time_quantum = 250;
}

TaskStruct* createChild(pid_t parent_pid) {
    pid_t pid = fork();
    TaskStruct *pcb = new TaskStruct(pid, parent_pid);

    pcb->cpu_burst_time = 600;
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

TaskStruct* child_list[10]; // !! temporary keeping... (this will be deleted later).
void createChildren() {
    pid_t main_pid = getpid();

    for(int i=0;i<10;++i) {
        if(getpid() == main_pid) {
            // parent process
            child_list[i] = createChild(main_pid);
            if(getpid() == main_pid) {
                // parent process
                // cout << i << "th process created, " << child_list[i]->pid <<  endl;
            }
        } else {
            // child process
            //break;
            // cout << getpid() << ' ' << i << endl;
        }
    }
    // cout << "hi from " << getpid() <<  endl;
}

bool alarm_signal = false;

void timerHandler(int signum) {
    static int alarm_cnt = 0;
    ++alarm_cnt;

    alarm_signal = true;
}

void parentProcess() {
    //  parent process TODOs
    //  1. receive alarm signal(SIGALRM/SIGVTALRM) by timer
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

    // cout << "here is parent" <<  endl;

    // setting a timer (time quantum to 250 ms at first).
    // reference - https://linuxspot.tistory.com/28
    struct sigaction sig_act;
    struct itimerval timer_val;

    memset(&sig_act, 0, sizeof(sig_act));
    sig_act.sa_handler = &timerHandler;
    sigaction(SIGVTALRM, &sig_act, NULL);

    timer_val.it_value.tv_sec = 0;
    timer_val.it_value.tv_usec = time_quantum * 1000;
    timer_val.it_interval = timer_val.it_value; // it_interval is same as it_value.

    setitimer(ITIMER_VIRTUAL, &timer_val, NULL);

    /*
     cout << "in linked list" <<  endl;
    for(DoublyLinkedList *it=rq_front->right;it->right != nullptr;it = it->right) {
         cout << "ddl " << (it->process_info)->pid <<  endl;
    }*/

    int cnt = 0;
    while(true) {
        if(alarm_signal) {
            alarm_signal = false;

            /*++cnt;
            if(!(cnt % 4)) {
                 cout << int(cnt / 4) << " sec elapsed!" <<  endl;
            }*/

            // child process scheduling

            if(rq_front->right == rq_rear) {
                // no process in ready queue.
                // check if there is no process in wait queue, quit parent process.
                continue;
            }

            DoublyLinkedList *dispatched_task_ptr = rq_front->right;
            TaskStruct *dispatched_task = dispatched_task_ptr->process_info;
            struct msg_buffer send_msg;
            send_msg.msg_type = dispatched_task->pid;
            string tmp_txt = "remain cpu burst is " + to_string(dispatched_task->cpu_burst_time);
            strncpy(send_msg.msg_txt, tmp_txt.c_str(), tmp_txt.length() + 1);

            cout << "snd " << msg_que_id << ' ' << send_msg.msg_type << ' ' << send_msg.msg_txt <<  endl;
            msgsnd(msg_que_id, &send_msg, sizeof(send_msg.msg_txt), IPC_NOWAIT);

            dispatched_task->cpu_burst_time -= time_quantum;
            // modify later: upper code to change burst time when receive ACK from child process.

            rq_front->right = rq_front->right->right;
            rq_front->right->left = rq_front;

            // rl->ddl->r
            rq_rear->left->right = dispatched_task_ptr;
            rq_rear->left = dispatched_task_ptr;
            dispatched_task_ptr->left = rq_rear->left;
            dispatched_task_ptr->right = rq_rear;
        }
    }
}

void childProcess() {       
    //  child process TODOs
    //  1. infinite-loop
    //  2. receive IPC messages from parent process, decrease CPU burst time.
    //
    //  (Opt.)
    //  3. simulate I/O waiting process.

    // cout << "here is child " << getpid() <<  endl;

    pid_t my_pid = getpid();

    struct msg_buffer recv_msg;

    while(true) {
        // reference 1 - https://pubs.opengroup.org/onlinepubs/7908799/xsh/msgrcv.html
        // reference 2 - https://lacti.github.io/2011/01/08/different-between-size-t-ssize-t/
        ssize_t msg_len = msgrcv(msg_que_id, &recv_msg, sizeof(recv_msg.msg_txt), getpid(), IPC_NOWAIT);

        // cout << "rcv " << msg_len <<  endl;

        if(msg_len > 0) {
            // receive something from parent process !!

             cout << "recv: " << getpid() << ": " << recv_msg.msg_txt <<  endl;
        }
    }
}


int main() {
    pid_t main_pid = getpid();
     cout << "main_pid is " << main_pid <<  endl;

    key_t main_key;
    initialize(main_key);

    // random number generator - reference: https://modoocode.com/304
     random_device rd0;
     mt19937 gen(rd0());
     uniform_int_distribution<int> rand_distrib(0, 999);

    createChildren();
    if(getpid() == main_pid) {
        // parent process
        //wait(NULL);
        /*
        for(int i=0;i<10;++i) {
             cout << child_list[i]->pid << ' ';
        }
         cout <<  endl;
*/
        parentProcess();
    } else {
        // child process
        // cout << getpid() <<  endl;

        childProcess();
    }

    return 0;
}