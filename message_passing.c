// 2015312229 Yoseob Kim, message passing homework
// Reference - https://www.geeksforgeeks.org/ipc-using-message-queues/
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

struct msg_buffer {
    long msg_type;
    char msg_txt[100];
};

int main(int argc, char* argv[]) {
    int key_id;
    struct msg_buffer my_message;
    if(argc < 2) {
        puts("error");
        exit(1);
    }

    key_id = msgget((key_t)1234, IPC_CREAT|0666);
    if(!strncmp(argv[1], "send", 4)) {
        char msg_txt[] = "hello i am sending a message...";
        my_message.msg_type = 1;
        strncpy(my_message.msg_txt, msg_txt, sizeof(msg_txt));

        msgsnd(key_id, &my_message, sizeof(my_message.msg_txt), 0);
        puts("sent");

    } else if(!strncmp(argv[1], "receive", 7)) {
        msgrcv(key_id, &my_message, sizeof(my_message.msg_txt), 1, 0);
        puts("recv:");
        puts(my_message.msg_txt);

        msgctl(key_id, IPC_RMID, NULL); 

    } else {
        puts("error");
        exit(1);
    }

    return 0;
}
