## OS Term project #1 [ scheduling.cpp / schedule_dump.txt ]
---  
  
### 구현 사항
- TaskStruct, DoublyLinkedList: 공룡 책에 나오는 task_struct를 모방하여 정의한 PCB와 연결 리스트
- initialize() 함수에서 time_quantum 설정 가능 (현재 설정 값 = 100 ms)
- parent 프로세스는 scheduling을 담당하는 OS의 역할이고, fork한 child 프로세스는 OS에서 실행되는 실제 프로세스의 역할
- 각 child 프로세스의 **i/o burst는 1회**씩 있는 것으로 가정 (프로세스별 실행 순서는 **cpu burst 0 -> i/o burst -> cpu burst 1**)
- priority에 기반한 MLFQ는 추후 추가적으로 구현해볼 예정

### 코드의 전체적인 동작 순서 (scheduling.cpp)
1. main 함수에서 createChildren() 함수 호출로 10개의 child 프로세스를 fork한다. (line 552)
2. parentProcess()와 childProcess()가 각각 parent 프로세스, child 프로세스를 담당한다.
3. fork된 child 프로세스들은 uniform_int_distribution&#60;int&#62;에 따라 random한 burst time을 생성한다. (line 451~456)
4. 이후 규칙을 정한 format에 따라 message queue의 내용을 결정하여 위의 burst time을 IPC를 이용해 parent 프로세스로 전달한다. (line 463~466)
5. parent 프로세스에서는 10개의 child 프로세스로부터의 IPC message를 대기한 후 각 정보에 따라 **PCB를 생성**한다. (line 284)
6. 설정한 itimer에서의 signal에 따라 **Round-Robin 알고리즘**을 수행한다. (line 305~)
7. dispatch된 프로세스에 대해서 해당 child 프로세스로 IPC message를 보내어 현재 run하게 되는 것을 알린다. (line 341)
8. child 프로세스에서는 while loop를 돌며 message queue를 확인한다. 만약 자신의 pid에 해당하는 message가 도착한다면, cpu burst time을 주어진 time quantum만큼 감소시킨다. (line 474~501)
9. 8번 과정을 수행하며 cpu_burst 값이 0이 된다면, parent 프로세스로 **IPC message를 보내어 I/O 처리**를 해야함을 알린다. (cb_flag == 0이 true인 경우) (line 519)
10. parent 프로세스에서는 해당 IPC message를 받아 해당 프로세스의 **control block을 i/o waiting queue로** 집어 넣는다. (line 390~409)
11. parent 프로세스에서 10번을 수행하기 전 processWaitingQueue() 함수를 호출(line 384)하여 현재 i/o waiting queue에 들어있는 프로세스들을 처리해준다.
12. i/o queue는 하나의 queue 안에 들어있지만 사실상 각각 다른 i/o를 의미하는 것으로 가정하고, 매 time quantum마다 그 값만큼 remaining i/o burst time을 병렬적으로 빼 준다. (line 179~183)
13. 만약 remaining burst time이 0이 된다면, **waiting queue에서 제거 후 다시 ready queue로**(이후 cb_flag = 1, 즉 cpu_burst&#91;1&#93;에 해당하는 각 프로세스별 두 번째 cpu_burst scheduling 수행) 집어 넣는다. (line 184~201)

### 결과 텍스트 파일 읽는 법 (schedule_dump.txt)
- main_pid, _th process created, message queue id 출력은 main 함수와 createChildren() 함수에서 출력한다.
- &#42; burst time generated (format...)은 childProcess() 에서 random한 burst time을 생성한 것이다.
- child pid: ...부터, ... is registered! 까지는 parentProcess() 에서 IPC message를 받은 후 PCB를 생성 한 후 남기는 log이다.
- &#91;parent&#93; initial status of ready queue 이후 10줄은 **front부터 rear까지 순서대로 ready queue의 내용**을 출력한다.
- **time-tick** 이후...
- ready queue에는 front에 위치한 process가 childProcess()에서 처리한 후 다시 ready queue의 rear로 들어간 것을 출력한다. (pid 879의 첫 cpu_burst_0이 1045였는데, time_quantum인 100만큼 감소한 945가 ready queue의 rear에 있는 것을 확인할 수 있다.)
- waiting queue에는 아무런 내용이 들어 있지 않을 때 "waiting queue: "라는 글자만 출력하고 우측은 비어 있는 것을 확인할 수 있다.
- 매 time-tick마다 &#91;child&#93;의 내용은 **현재 dispatch되어 처리되는 프로세스**의 내용을 확인할 수 있다.
- queue를 출력하는 형식은 (**pid, remaining cpu burst**)->(pid, remaining cpu burst)->&#91;rear&#93;이다.
- **i/o waiting queue의 동작**은 54번째 time-tick에서 처음 확인할 수 있다. (line 324) 또한 pid 872의 i/o burst가 0이 되는 것은 57번째 time-tick에서 확인 가능하다. (line 340) 이후 다시 ready queue로 들어가서, dispatch 될 때 두 번째 cpu burst (cpu_burst&#91;1&#93;)를 사용한다는 사실을 알린다. (line 392) i/o waiting queue에 여러 프로세스가 들어와 있는 것은 113번째 time-tick에서 확인할 수 있다. (line 632)
- child 프로세스가 두 번의 cpu burst phase와 한 번의 i/o phase를 모두 완료하면, 해당 프로세스를 완전히 종료하고 **waiting time을 출력**한다. (line 655에서 처음 나타남) -> remaining cpu_burst &#60; time_quantum인 경우, 구현의 용이를 위하여 해당 차이 값을 time_diff 변수에 누적한 후 최종 waiting time, elapsed time을 구할 때 전체 timer의 경과 시간에서 time_diff를 빼 주는 것으로 처리하였다. (scheduling.cpp의 line 308, 332, 362, 414)
- 모든 scheduling 과정이 끝나면, 모든 동작이 종료되었음을 알리고 message queue를 삭제한다. (line 1083~1084)
