/***************** Centralized Mutual Exclusion Algorithm *********************/
/* This example implements a centralized mutual exclusion algorithm.          */
/* CS requests of client 2 not satisfied                                      */
/******************************************************************************/

#include "msg/msg.h"
#include "mc/mc.h"
#include "xbt/automaton.h"
#include "xbt/automatonparse_promela.h"
#include "example_liveness_with_cycle.h"
#include "y.tab.c"

#define AMOUNT_OF_CLIENTS 2
#define CS_PER_PROCESS 3

XBT_LOG_NEW_DEFAULT_CATEGORY(example_liveness_with_cycle2, "my log messages");


int p=0; 
int q=0;

int predP(){
  return p;
}

int predQ(){
  return q;
}


int coordinator(int argc, char *argv[])
{
  xbt_dynar_t requests = xbt_dynar_new(sizeof(char *), NULL);   // dynamic vector storing requests (which are char*)
  int CS_used = 0;              // initially the CS is idle

  while(1){
    m_task_t task = NULL;
    MSG_task_receive(&task, "coordinator");
    const char *kind = MSG_task_get_name(task); //is it a request or a release?
    if (!strcmp(kind, "request")) {     // that's a request
      char *req = MSG_task_get_data(task);
      if (CS_used) {            // need to push the request in the vector
        XBT_INFO("CS already used. Queue the request of client %d", atoi(req) +1);
        xbt_dynar_push(requests, &req);
      } else {                  // can serve it immediatly
	if(strcmp(req, "1") == 0){
	  m_task_t answer = MSG_task_create("grant", 0, 1000, NULL);
	  MSG_task_send(answer, req);
	  CS_used = 1;
	  XBT_INFO("CS idle. Grant immediatly");
	}
      }
    } else {                    // that's a release. Check if someone was waiting for the lock
      if (xbt_dynar_length(requests) > 0) {
        XBT_INFO("CS release. Grant to queued requests (queue size: %lu)", xbt_dynar_length(requests));
        char *req ;
	xbt_dynar_get_cpy(requests, (xbt_dynar_length(requests) - 1), &req);
	if(strcmp(req, "1") == 0){
	  xbt_dynar_pop(requests, &req);
	  MSG_task_send(MSG_task_create("grant", 0, 1000, NULL), req);
	}else{
	  CS_used = 0;
	}
      } else {                  // nobody wants it
        XBT_INFO("CS release. resource now idle");
        CS_used = 0;
      }
    }
    MSG_task_destroy(task);
  }
  XBT_INFO("Received all releases, quit now"); 
  return 0;
}

int client(int argc, char *argv[])
{
  int my_pid = MSG_process_get_PID(MSG_process_self());

  char *my_mailbox = bprintf("%s", argv[1]);
 
  while(1){  

    XBT_INFO("Ask the request");
   
    MSG_task_send(MSG_task_create("request", 0, 1000, my_mailbox), "coordinator");
    
    if(strcmp(my_mailbox, "2") == 0){
      p = 1;
      q = 0;
      XBT_INFO("Propositions changed (p=1, q=0)");
    }

    // wait the answer

    m_task_t grant = NULL;
    MSG_task_receive(&grant, my_mailbox);

    if((strcmp(my_mailbox, "2") == 0) && (strcmp(MSG_task_get_name(grant), "grant") == 0)){
      q = 1;
      p = 0;
      XBT_INFO("Propositions changed (q=1, p=0)");
    }

    MSG_task_destroy(grant);
    XBT_INFO("%s got the answer. Sleep a bit and release it", argv[1]);
    MSG_process_sleep(1);
    
    MSG_task_send(MSG_task_create("release", 0, 1000, NULL), "coordinator");

    MSG_process_sleep(my_pid);
    
    if(strcmp(my_mailbox, "2") == 0){
      q=0;
      p=0;
      XBT_INFO("Propositions changed (q=0, p=0)");
    }
    
  }

  XBT_INFO("Got all the CS I wanted (%s), quit now", my_mailbox);
  return 0;
}

int main(int argc, char *argv[])
{
  init();
  yyparse();
  automaton = get_automaton();
  xbt_new_propositional_symbol(automaton,"p", &predP); 
  xbt_new_propositional_symbol(automaton,"q", &predQ); 
  
  MSG_global_init(&argc, argv);
  MSG_create_environment("../msg_platform.xml");
  MSG_function_register("coordinator", coordinator);
  MSG_function_register("client", client);
  MSG_launch_application("deploy_mutex2.xml");
  MSG_main_liveness(automaton, argv[0]);
  
  return 0;
}
