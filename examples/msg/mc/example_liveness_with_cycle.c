/***************** Centralized Mutual Exclusion Algorithm *********************/
/* This example implements a centralized mutual exclusion algorithm.          */
/* There is no bug on it, it is just provided to test the state space         */
/* reduction of DPOR.                                                         */
/******************************************************************************/

#include "msg/msg.h"
#include "mc/mc.h"
#include "xbt/automaton.h"
#include "xbt/automatonparse_promela.h"
#include "example_liveness_with_cycle.h"
#include "y.tab.c"

#define AMOUNT_OF_CLIENTS 2
#define CS_PER_PROCESS 2

XBT_LOG_NEW_DEFAULT_CATEGORY(example_liveness_with_cycle, "my log messages");

int p=0; 
int q=0;

int predP(){
  return p;
}

int predQ(){
  return q;
}


//xbt_dynar_t listeVars -> { void *ptr; int size (nb octets); } => liste de structures 
//=> parcourir la liste et comparer les contenus de chaque pointeur

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
      //XBT_INFO("Req %s", req);
      if (CS_used) {            // need to push the request in the vector
        XBT_INFO("CS already used. Queue the request");
        xbt_dynar_push(requests, &req);
      } else {                  // can serve it immediatly
        XBT_INFO("CS idle. Grant immediatly");
	if(strcmp(req, "1") == 0){
	  m_task_t answer = MSG_task_create("grant", 0, 1000, NULL);
	  MSG_task_send(answer, req);
	  CS_used = 1;
	}else{
	  m_task_t answer = MSG_task_create("notgrant", 0, 1000, NULL);
	  MSG_task_send(answer, req);
	}
      }
    } else {                    // that's a release. Check if someone was waiting for the lock
      if (xbt_dynar_length(requests) > 0) {
        XBT_INFO("CS release. Grant to queued requests (queue size: %lu)",
              xbt_dynar_length(requests));
        char *req ;
	xbt_dynar_pop(requests, &req);
	if(strcmp(req, "1") == 0){
	  MSG_task_send(MSG_task_create("grant", 0, 1000, NULL), req);
	  todo--;
	}else{
	  MSG_task_send(MSG_task_create("notgrant", 0, 1000, NULL), req);
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
  // use my pid as name of mailbox to contact me
  char *my_mailbox = bprintf("%s", argv[1]);
  // request the CS 2 times, sleeping a bit in between
  int i;
  for (i = 0; i < CS_PER_PROCESS; i++) {
    XBT_INFO("Ask the request");
    MSG_task_send(MSG_task_create("request", 0, 1000, my_mailbox),
                  "coordinator");
    if(strcmp(my_mailbox, "2") == 0){
      p = 1;
      q = 0;
    }
    // wait the answer
    m_task_t grant = NULL;
    MSG_task_receive(&grant, my_mailbox);
    const char *kind = MSG_task_get_name(grant);
    if(!strcmp(kind, "grant")){
      if(strcmp(my_mailbox, "2") == 0){
	q = 1;
	p = 0;
      }
	//answers ++;
      MSG_task_destroy(grant);
      XBT_INFO("got the answer. Sleep a bit and release it");
      MSG_process_sleep(1);
      MSG_task_send(MSG_task_create("release", 0, 1000,NULL ),
		    "coordinator");
    }
    
    MSG_process_sleep(my_pid);

  }

  //XBT_INFO("Got all the CS I wanted (%s), quit now", my_mailbox);
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
  MSG_main_liveness_stateless(automaton);
  //MSG_main();
  
  return 0;
}
