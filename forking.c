#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    pid_t pid = fork();
    printf("current pid = %d\n", getpid());
    printf("current pid by fork = %d\n", pid);

    if(pid == 0) { // child process
        printf("child pid: %d\n", getpid());
        printf("child pid by fork: %d\n", pid);
    } else if(pid > 0) { // parent process        
        wait(NULL);
        printf("parent pid: %d\n", getpid());
        printf("parent pid by fork: %d\n", pid);
    }
}