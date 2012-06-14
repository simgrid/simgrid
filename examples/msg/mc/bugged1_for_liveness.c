/***************** Centralized Mutual Exclusion Algorithm *********************/
/* This example implements a centralized mutual exclusion algorithm.          */
/* CS requests of client 1 not satisfied                                      */
/* LTL property checked : G(r->F(cs)); (r=request of CS, cs=CS ok)            */
/******************************************************************************/

#include "msg/msg.h"
#include "mc/mc.h"
#include "xbt/automaton.h"
#include "bugged1_liveness.h"

#define AMOUNT_OF_CLIENTS 2
#define CS_PER_PROCESS 2

XBT_LOG_NEW_DEFAULT_CATEGORY(bugged1_liveness, "my log messages");


int r=0; 
int cs=0;

int predR(){
  return r;
}

int predCS(){
  return cs;
}


int coordinator(int argc, char *argv[])
{
  xbt_dynar_t requests = xbt_dynar_new(sizeof(char *), NULL);   // dynamic vector storing requests (which are char*)
  int CS_used = 0;              // initially the CS is idle
  int todo = AMOUNT_OF_CLIENTS * CS_PER_PROCESS;        // amount of releases we are expecting
  while (todo > 0) {
    m_task_t task = NULL;
    MSG_task_receive(&task, "coordinator");
    const char *kind = MSG_task_get_name(task); //is it a request or a release?
    if (!strcmp(kind, "request")) {     // that's a request
      char *req = MSG_task_get_data(task);
      if (CS_used) {            // need to push the request in the vector
        XBT_INFO("CS already used. Queue the request of client %d", atoi(req) +1);
        xbt_dynar_push(requests, &req);
      } else {                  // can serve it immediatly
	if(strcmp(req, "2") == 0){
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
	if(strcmp(req, "2") == 0){
	  xbt_dynar_pop(requests, &req);
	  MSG_task_send(MSG_task_create("grant", 0, 1000, NULL), req);
	  todo--;
	}else{
	  xbt_dynar_pop(requests, &req);
	  MSG_task_send(MSG_task_create("notgrant", 0, 1000, NULL), req);
	  CS_used = 0;
	  todo--;
	}
      } else {                  // nobody wants it
        XBT_INFO("CS release. resource now idle");
        CS_used = 0;
        todo--;
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
  int i;

  // request the CS (CS_PER_PROCESS times), sleeping a bit in between
  for (i = 0; i < CS_PER_PROCESS; i++) {
      
    XBT_INFO("Ask the request");
    MSG_task_send(MSG_task_create("request", 0, 1000, my_mailbox), "coordinator");

    if(strcmp(my_mailbox, "1") == 0){
      r = 1;
      cs = 0;
      XBT_DEBUG("Propositions changed : r=1, cs=0");
    }

    // wait the answer
    m_task_t grant = NULL;
    MSG_task_receive(&grant, my_mailbox);
    const char *kind = MSG_task_get_name(grant);

    if((strcmp(my_mailbox, "1") == 0) && (strcmp("grant", kind) == 0)){
      cs = 1;
      r = 0;
      XBT_DEBUG("Propositions changed : r=0, cs=1");
    }


    MSG_task_destroy(grant);
    XBT_INFO("%s got the answer. Sleep a bit and release it", argv[1]);
    MSG_process_sleep(1);
    MSG_task_send(MSG_task_create("release", 0, 1000, NULL), "coordinator");

    MSG_process_sleep(my_pid);
    
    if(strcmp(my_mailbox, "1") == 0){
      cs=0;
      r=0;
      XBT_DEBUG("Propositions changed : r=0, cs=0");
    }
    
  }

  XBT_INFO("Got all the CS I wanted (%s), quit now", my_mailbox);
  return 0;
}

int main(int argc, char *argv[])
{
  xbt_automaton_t a = MC_create_automaton("promela1_bugged1_liveness");
  xbt_new_propositional_symbol(a,"r", &predR); 
  xbt_new_propositional_symbol(a,"cs", &predCS); 
  
  MSG_global_init(&argc, argv);
  MSG_create_environment("../msg_platform.xml");
  MSG_function_register("coordinator", coordinator);
  MSG_function_register("client", client);
  MSG_launch_application("deploy_bugged1_liveness.xml");
  MSG_main_liveness(a);

  return 0;
}
