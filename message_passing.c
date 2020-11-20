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
        my_message.msg_type = 2;    // 여기 이 msg_type이랑...
        strncpy(my_message.msg_txt, msg_txt, sizeof(msg_txt));

        msgsnd(key_id, &my_message, sizeof(my_message.msg_txt), 0);
        puts("sent");

    } else if(!strncmp(argv[1], "receive", 7)) {
        msgrcv(key_id, &my_message, sizeof(my_message.msg_txt), 2, 0);  // ... 여기 뒤에서 두 번째 인자 2가 같아야 수신함.
        // 마지막 flag가 0이면 blocking, IPC_NOWAIT이면 non-blocking (msgsnd에서 마지막 인자도 같은 역할)
        puts("recv:");
        puts(my_message.msg_txt);

        msgctl(key_id, IPC_RMID, NULL); 

    } else {
        puts("error");
        exit(1);
    }

    return 0;
}
