/* Centralized Mutual Exclusion Algorithm 
 *
 * This constitutes the answer to the exercice 2 of the practical 
 * lab on implementing mutual exclusion algorithms with SimGrid.
 * 
 * YOU SHOULD TRY IMPLEMENTING IT YOURSELF BEFORE READING THE SOLUTION.
 */

#include "msg/msg.h"

#define AMOUNT_OF_CLIENTS 5
#define CS_PER_PROCESS 2
XBT_LOG_NEW_DEFAULT_CATEGORY(centralized, "my log messages");

int coordinator(int argc, char** argv);
int client(int argc, char** argv);

int coordinator(int argc, char*argv[]) {
  xbt_dynar_t requests=xbt_dynar_new(sizeof(char*),NULL); // dynamic vector storing requests (which are char*)
  int CS_used=0; // initially the CS is idle
  int todo= AMOUNT_OF_CLIENTS*CS_PER_PROCESS; // amount of releases we are expecting
  while(todo>0) { 
    m_task_t task=NULL; 
    MSG_task_receive(&task,"coordinator");
    const char *kind = MSG_task_get_name(task); //is it a request or a release?
    if (!strcmp(kind,"request")) { // that's a request
      char *req = MSG_task_get_data(task);
      if (CS_used) { // need to push the request in the vector
	INFO0("CS already used. Queue the request");
	xbt_dynar_push(requests, &req);
      } else { // can serve it immediatly
	INFO0("CS idle. Grant immediatly");
	m_task_t answer = MSG_task_create("grant",0,1000,NULL);
	MSG_task_send(answer,req);
	CS_used = 1;
      }
    } else { // that's a release. Check if someone was waiting for the lock
      if (xbt_dynar_length(requests)>0) {
	INFO1("CS release. Grant to queued requests (queue size: %lu)",xbt_dynar_length(requests));
	char *req;
	xbt_dynar_pop(requests,&req);
	MSG_task_send(MSG_task_create("grant",0,1000,NULL),req);	
	todo--;
      } else { // nobody wants it
	INFO0("CS release. resource now idle");
	CS_used=0;
	todo--;
      }
    }
    //MSG_task_destoy(task);
  }
  INFO0("Received all releases, quit now");
  return 0;
}

int client(int argc, char *argv[]) {
  int my_pid=MSG_process_get_PID(MSG_process_self());
  // use my pid as name of mailbox to contact me
  char *my_mailbox=bprintf("%d",my_pid);
  // request the CS 3 times, sleeping a bit in between
  int i;
  for (i=0; i<CS_PER_PROCESS;i++) {
    INFO0("Ask the request");
    MSG_task_send(MSG_task_create("request",0,1000,my_mailbox),"coordinator");
    // wait the answer
    m_task_t grant = NULL;
    MSG_task_receive(&grant,my_mailbox);
    //MSG_task_destoy(grant);
    INFO0("got the answer. Sleep a bit and release it");
    MSG_process_sleep(1);
    MSG_task_send(MSG_task_create("release",0,1000,NULL),"coordinator");    
    MSG_process_sleep(my_pid);
  }
  INFO0("Got all the CS I wanted, quit now");
  return 0;
}

int main(int argc, char*argv[]) {
  MSG_global_init(&argc,argv);
  MSG_create_environment("../msg_platform.xml");
  MSG_function_register("coordinator", coordinator);
  MSG_function_register("client", client);
  MSG_launch_application("deploy.xml");
  MSG_main();
  return 0;
}
