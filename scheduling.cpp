// OS Term Project #1 by Yoseob Kim
// Simple scheduling (with Round-Robin)
// Develop environments:    MBP(13-inch, 2017, Two Thunderbolt 3 ports, Intel Core i5)
//                          (https://support.apple.com/kb/SP754?locale=ko_KR)
//                          (os version: macOS Catalina (10.15.6))
// Test environments        0 - same as develop environments
//                          1 - WSL2, Ubuntu 20.04.1 LTS

// ** Important NOTICE **
// When IPC is not working properly, 
// run zsh(or bash) shell and type "ipcs" then check "msqid", 
// then type "ipcrm -q ${msqid}" and re-excute this program(./scheduling.out).
// reference - https://iamhjoo.tistory.com/4

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
    unsigned int cpu_burst, io_burst;
    unsigned int state, priority;
    // [state] 0: ready, 1: dispatch(running), 2: i/o waiting, 3: terminated
    // [priority] for now, constant number 0 is assigned.

    TaskStruct(pid_t _pid, pid_t _parent_pid, unsigned int _cpu_burst, unsigned int _io_burst) {
        this->pid = _pid;
        this->parent_pid = _parent_pid;
        this->cpu_burst = _cpu_burst;
        this->io_burst = _io_burst;
        this->state = 0;
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

void initialize(key_t& main_key, unsigned int& msg_que_id, unsigned int& time_quantum) {
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
    time_quantum = 500;
}

void createChildren(int num_of_child_process) {
    pid_t main_pid = getpid();

    for(int i=0;i<num_of_child_process;++i) {
        if(getpid() == main_pid) {
            // parent process
            pid_t child_pid = fork();
            if(child_pid > 0) {
                // parent process
                cout << i + 1 << "th process created, " << child_pid << endl;
            }
        }
    }
}

bool alarm_signal = false;
void timerHandler(int signum) {
    alarm_signal = true;
}

TaskStruct* createPCB(pid_t _pid, pid_t _parent_pid, unsigned int _cpu_burst, unsigned int _io_burst) {
    TaskStruct *pcb = new TaskStruct(_pid, _parent_pid, _cpu_burst, _io_burst);

    DoublyLinkedList *ddl = new DoublyLinkedList;
    ddl->process_info = pcb;

    // rl->ddl->r
    rq_rear->left->right = ddl;
    ddl->left = rq_rear->left;
    rq_rear->left = ddl;
    ddl->right = rq_rear;

    return pcb;
}

void stringParsing(string& msg_txt, char& type, pid_t& pid_info, void *data_0, void *data_1) {
    type = msg_txt[0];
    string::size_type pos;
    pos = msg_txt.find(",");
    pid_info = stoi(msg_txt.substr(1, pos - 1));
    msg_txt = msg_txt.substr(pos+1);
    pos = msg_txt.find(",");
    if(type == 'R') {                       // Register new child process
        *(unsigned int *)data_0 = stoi(msg_txt.substr(0, pos));
        *(unsigned int *)data_1 = stoi(msg_txt.substr(pos+1));
    } else if(type == 'D' || type == 'I') { // Dispatch or I/O request
        *(unsigned int *)data_0 = stoi(msg_txt.substr(0, pos));
        if(sizeof(size_t) == 4) {
            *(size_t *)data_1 = stoi(msg_txt.substr(pos+1));
        } else if(sizeof(size_t) == 8) {
            *(size_t *)data_1 = stoll(msg_txt.substr(pos+1));
        }
        
    }
}

void processWaitingQueue(unsigned int time_quantum) {
    for(DoublyLinkedList *it=wq_front->right; it != wq_rear; it = it->right) {
        TaskStruct *pcb = it->process_info;

        if(pcb->io_burst > time_quantum) {
            pcb->io_burst -= time_quantum;
        } else {
            // if cpu_burst remains, moves to ready queue.
            DoublyLinkedList *it_backup = it;
            
            // if not, terminate process. 
            cout << pcb->pid << " io_burst is 0.\n";
            pcb->io_burst = 0;
            pcb->state = 3;     // terminate.

            // delete from waiting queue
            it->left->right = it->right;
            it->right->left = it->left;
            it = it->left;
        }
    }

    for(DoublyLinkedList *it=wq_front->right; it != wq_rear; it = it->right) {
        cout << "(" << it->process_info->pid << "," << it->process_info->io_burst << ")->";
    }
    cout << endl;
}

void parentProcess(unsigned int msg_que_id, unsigned int time_quantum) {
    //  PARENT PROCESS is like an OPERATING SYSTEM which controls process-scheduling.
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

    //cout << "here is parent " << getpid() << endl;

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

    unsigned int elapsed_time_by_timer, time_diff;
    elapsed_time_by_timer = time_diff = 0;
    // final elapsed time is elapsed_time_by_timer - time_diff
    // time_diff is calculated by some child process which (remain burst time) < (universal time quantum).

    /*
     cout << "in linked list" <<  endl;
    for(DoublyLinkedList *it=rq_front->right;it->right != nullptr;it = it->right) {
         cout << "ddl " << (it->process_info)->pid <<  endl;
    }*/

    struct msg_buffer send_msg, recv_msg;
    string tmp_txt;
    char type;
    pid_t child_pid;
    size_t pcb_address;

    unsigned int process_cnt = 0;
    while(process_cnt < 10) {
        // reference 1 - https://pubs.opengroup.org/onlinepubs/7908799/xsh/msgrcv.html
        // reference 2 - https://lacti.github.io/2011/01/08/different-between-size-t-ssize-t/
        ssize_t msg_len = msgrcv(msg_que_id, &recv_msg, sizeof(recv_msg.msg_txt), getpid(), IPC_NOWAIT);

        if(msg_len > 0) {
            cout << recv_msg.msg_txt << endl;
            if(recv_msg.msg_txt[0] != 'R') {
                continue;
            }
            tmp_txt = recv_msg.msg_txt;
            unsigned int _cpu_burst, _io_burst;
            stringParsing(tmp_txt, type, child_pid, &_cpu_burst, &_io_burst);
            
            //cout << "child_pid: " << child_pid << " cpu_burst: " << _cpu_burst << " io_burst: " << _io_burst << endl;
            TaskStruct *pcb = createPCB(child_pid, getpid(), _cpu_burst, _io_burst);
            cout << pcb->pid << " is registered!" << endl;

            process_cnt ++;
        }
    }

    cout << "initial status of ready queue" << endl;
    for(DoublyLinkedList *it = rq_front->right; it != rq_rear; it = it->right) {
        TaskStruct *ts = it->process_info;
        cout << "RQ: " << ts->pid << " " << ts->cpu_burst << " " << ts->io_burst << endl;
    }

    while(true) {
        if(alarm_signal) {
            alarm_signal = false;

            elapsed_time_by_timer += time_quantum;  // time_quantum is universal time quantum calculated using timer.
            cout << elapsed_time_by_timer << " elapsed\n";

            // child process scheduling
            if(rq_front->right == rq_rear) {
                // no process in ready queue.
                // check if there is no process in wait queue, quit parent process.
                if(wq_front->right == wq_rear) {
                    break;  // whole cpu scheduling is done.
                }
            }

            if(rq_front->right != rq_rear) {
                DoublyLinkedList *dispatched_task_ptr = rq_front->right;
                TaskStruct *dispatched_task = dispatched_task_ptr->process_info;
                send_msg.msg_type = dispatched_task->pid;

                // [Dispatch] message format
                // Dchild_pid,time_quantum,pcb_address
                unsigned int _time_quantum;     // _time_quantum is real time quantum assigned to each child process.

                if(dispatched_task->cpu_burst < time_quantum) {
                    _time_quantum = dispatched_task->cpu_burst;
                    time_diff += (time_quantum - _time_quantum);
                } else {
                    _time_quantum = time_quantum;
                }

                unsigned int _cpu_burst = dispatched_task->cpu_burst;
                string tmp_txt = "D" + to_string(dispatched_task->pid) + "," + to_string(_time_quantum) + "," + to_string((size_t)dispatched_task_ptr) + '\0';
                strncpy(send_msg.msg_txt, tmp_txt.c_str(), tmp_txt.length() + 1);

                msgsnd(msg_que_id, &send_msg, sizeof(send_msg.msg_txt), IPC_NOWAIT);

                // delete from ready queue
                rq_front->right = rq_front->right->right;
                rq_front->right->left = rq_front;
                dispatched_task->state = 1;
                dispatched_task->cpu_burst -= _time_quantum;

                if(dispatched_task->cpu_burst > 0){
                    // re-insert current process if not finished yet.
                    // rl->task->r
                    rq_rear->left->right = dispatched_task_ptr;
                    dispatched_task_ptr->left = rq_rear->left;
                    rq_rear->left = dispatched_task_ptr;
                    dispatched_task_ptr->right = rq_rear;
                    dispatched_task->state = 0;
                } else {
                    // no remaining cpu_burst -> moves to i/o waiting queue in start of next time quantum.
                    // parent process receives IPC message when remaining cpu burst time of child process becomes 0.
                }    
            }

            processWaitingQueue(time_quantum);  // decrease i/o burst of each pcb in waiting queues.

            //  check there is IPC message from child process.
            //  then move each process to i/o waiting queue.
            //
            while(true) {
                ssize_t msg_len = msgrcv(msg_que_id, &recv_msg, sizeof(recv_msg.msg_txt), getpid(), IPC_NOWAIT);
                if(msg_len <= 0) {
                    break;
                }
                //cout << recv_msg.msg_txt << endl;
                
                tmp_txt = recv_msg.msg_txt;
                unsigned int _io_burst;
                stringParsing(tmp_txt, type, child_pid, &_io_burst, &pcb_address);
                //cout << "type: " << type << ", child_pid: " << child_pid << ", io_burst: " << _io_burst << ", pcb_address: " << pcb_address << endl;

                DoublyLinkedList *dispatched_task_1_ptr = (DoublyLinkedList *)pcb_address;
                TaskStruct *dispatched_task_1 = dispatched_task_1_ptr->process_info;
                dispatched_task_1->state = 2;
                // rl->task->r
                wq_rear->left->right = dispatched_task_1_ptr;
                dispatched_task_1_ptr->left = wq_rear->left;    // ...
                wq_rear->left = dispatched_task_1_ptr;
                dispatched_task_1_ptr->right = wq_rear;
            }
        }
    }

    msgctl(msg_que_id, IPC_RMID, NULL); // delete message queue
    cout << "message queue is deleted !\n";
}

void childProcess(pid_t parent_pid, unsigned int msg_que_id) {
    //  CHILD PROCESS is like a PROCESS in an operating system.
    //  child process TODOs
    //  0. process register to parent process
    //  1. infinite-loop
    //  2. receive IPC messages from parent process, decrease CPU burst time.
    //
    //  (Opt.)
    //  3. simulate I/O waiting process.

    // cout << "here is child " << getpid() <<  endl;

    pid_t my_pid = getpid();

    struct msg_buffer send_msg, recv_msg;

    // TODO 0 - process register to parent process
    // Though fork() is executed in parent process,
    // child process have to send cpu_burst and io_burst values to parent process immediately.

    // random number generator - reference: https://modoocode.com/304
    random_device rd0; // , rd1;
    mt19937 gen0(rd0()); // , gen1(rd1());
    uniform_int_distribution<int> rand_distrib0(0, 2999), rand_distrib1(0, 999);
    // cpu_burst range to [0, 1000), io_burst range to [0, 100).
    size_t pcb_address;

    unsigned int cpu_burst, io_burst;
    cpu_burst = rand_distrib0(gen0);
    io_burst = rand_distrib1(gen0);
    cout << "* burst time generated: " << getpid() << " " << cpu_burst << " " << io_burst << endl;

    // [Register] message(msg_txt) format:
    // Rmy_pid,cpu_burst,io_burst
    // ex. my_pid = 55302, cpu_burst = 550, io_burst = 30
    // "R55302,550,30"

    send_msg.msg_type = parent_pid;
    string tmp_txt = "R" + to_string(getpid()) + "," + to_string(cpu_burst) + "," + to_string(io_burst) + '\0';
    strncpy(send_msg.msg_txt, tmp_txt.c_str(), tmp_txt.length() + 1);
    msgsnd(msg_que_id, &send_msg, sizeof(send_msg.msg_txt), IPC_NOWAIT);
    //std::cout << getpid() << " " << send_msg.msg_type << " " << send_msg.msg_txt << " sent!\n";

    // TODO 1 - infinite-loop
    while(cpu_burst > 0) {
        // reference 1 - https://pubs.opengroup.org/onlinepubs/7908799/xsh/msgrcv.html
        // reference 2 - https://lacti.github.io/2011/01/08/different-between-size-t-ssize-t/
        ssize_t msg_len = msgrcv(msg_que_id, &recv_msg, sizeof(recv_msg.msg_txt), getpid(), IPC_NOWAIT);

        // cout << "rcv " << msg_len <<  endl;

        if(msg_len > 0) {
            // receive something from parent process !! -> means SCHEDULED now!
            //cout << "recv: " << getpid() << ": " << recv_msg.msg_txt <<  endl;

            string tmp_txt = recv_msg.msg_txt;
            char type;
            pid_t _0;
            unsigned int _time_quantum;

            stringParsing(tmp_txt, type, _0, &_time_quantum, &pcb_address);
            cout << "recv: " << getpid() << " " << type << " " << _time_quantum << " " << cpu_burst << endl;

            // TODO 2 - decrease cpu_burst
            
            // process by assigned time quantum (data is in receive message buffer)
            // (each process would vary since remain cpu_burst could be lower than time_quantum)

            cpu_burst -= _time_quantum;
            //DoublyLinkedList *dispatched_task_ptr = (DoublyLinkedList *)pcb_address;
            //TaskStruct *dispatched_task = dispatched_task_ptr->process_info;
            //dispatched_task->cpu_burst -= _time_quantum;
            
            // pcb access by parent, child process concurrently --> error occurs!
        }
    }

    // TODO 3- i/o request to parent process
    // [I/O] message(msg_txt) format:
    // "Imy_pid,io_burst,pcb_address"
    tmp_txt = "I" + to_string(my_pid) + "," + to_string(io_burst) + "," + to_string(pcb_address) + '\0';
    strncpy(send_msg.msg_txt, tmp_txt.c_str(), tmp_txt.length() + 1);

    msgsnd(msg_que_id, &send_msg, sizeof(send_msg.msg_txt), IPC_NOWAIT);
    cout << getpid() << "cpu_burst done!\n";

    // local io_burst value is not updated.
    // however, after cpu_burst become 0, above while loop terminated,
    // then, parent process will manage "remaining io_burst value of PCB" in waiting queue.
    cout << "** Remaining CPU-burst time of " << getpid() << " is 0 now. This process moves to I/O waiting-queue.\n";
}

int main() {
    pid_t main_pid = getpid();
    cout << "main_pid is " << main_pid <<  endl;

    key_t main_key;
    unsigned int msg_que_id, time_quantum;
    initialize(main_key, msg_que_id, time_quantum);

    const unsigned int num_of_child_process = 10;
    createChildren(num_of_child_process);
    if(getpid() == main_pid) {
        // parent process
        //wait(NULL);
        /*
        for(int i=0;i<10;++i) {
            cout << child_list[i]->pid << ' ';
        }
        cout <<  endl;
*/
        cout << "message queue id is " << msg_que_id << endl;
        parentProcess(msg_que_id, time_quantum);
    } else {
        // child process
        //cout << getpid() <<  endl;

        childProcess(main_pid, msg_que_id);
    }

    return 0;
}