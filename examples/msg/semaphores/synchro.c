#include <stdio.h>
#include <stdlib.h>
#include "msg/msg.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_semaphore_example,
                             "Messages specific for this msg example");

msg_sem_t sem;

int peer(int argc, char* argv[]){

  int i = 0; 
  
  while(i < argc) {
    double wait_time = atof(argv[i++]);
    MSG_process_sleep(wait_time);
    XBT_INFO("Trying to acquire");
    MSG_sem_acquire(sem);
    XBT_INFO("Acquired");

    wait_time = atof(argv[i++]);
    MSG_process_sleep(wait_time);
    XBT_INFO("Releasing");
    MSG_sem_release(sem);
    XBT_INFO("Released");
  }

}

int main(int argc, char* argv[]) {

  MSG_init(&argc, argv);
  MSG_create_environment(argv[1]);
  
  xbt_dynar_t hosts = MSG_hosts_as_dynar();
  msg_host_t h = xbt_dynar_get_as(hosts,0,msg_host_t);


  char* aliceTimes[] = {"0", "1", "3", "5", "1", "2", "5", "0"};
  char* bobTimes[] = {"1", "1", "1", "2", "2", "0", "0", "5"};


  MSG_process_create_with_arguments("Alice", peer, NULL, 
				    h, 8, aliceTimes);
  MSG_process_create_with_arguments("Bob", peer, NULL, 
				    h, 8, bobTimes);

  MSG_main();

}
