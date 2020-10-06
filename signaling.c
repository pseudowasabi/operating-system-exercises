// 2015312229 Yoseob Kim, signaling homework
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

void handler(int signum) {
    char *signal_type, *signal_info;

    switch(signum) {
    case SIGINT:
        signal_type = "SIGINT";
        signal_info = "interrupt from keyboard";
        break;
    case SIGFPE:
        signal_type = "SIGFPE";
        signal_info = "floating point exception";
        break;
    case SIGKILL: 
        signal_type = "SIGKILL" ;
        signal_info = "terminate receiving process";
        break;
    case SIGCHLD:
        signal_type = "SIGCHLD";
        signal_info = "child process stopped or terminated";
        break;
    case SIGSEGV:
        signal_type = "SIGSEGV";
        signal_info = "segment access violation";
        break;
    }

    printf("Signal type: %s (%s)\n", signal_type, signal_info);
    if(signum == SIGFPE || signum == SIGSEGV) {
        exit(1);
    }
}

int main() {
    int array[5], i;
    pid_t pid = fork();
    printf("current pid = %d\n", getpid());
    printf("current pid by fork = %d\n", pid);

    signal(SIGINT, handler);
    signal(SIGFPE, handler);
    signal(SIGKILL, handler);
    signal(SIGCHLD, handler);
    signal(SIGSEGV, handler);

    if(pid == 0) { // child process
        printf("child pid: %d\n", getpid());
        printf("child pid by fork: %d\n", pid);

        // SIGINT example - CTRL+C within 21 seconds
        /*
        for(i=0;i<0x7fffffff;++i) {
            if(!(i % 100000000)) {
                printf("%d ", i / 100000000);
            }
        }*/

        // SIGFPE example - DIV0 error
        // i = 5 / 0;

        // SIGKILL example
        //while(1);

        // SIGSEGV example
        array[10] = 10;

    } else if(pid > 0) { // parent process
        wait(NULL); // wait for child process termination

        // SIGCHLD example - call exit(0) in child process, not exit(1)
        printf("parent pid: %d\n", getpid());
        printf("parent pid by fork: %d\n", pid);

        //while(1);
    }

    return 0;
}