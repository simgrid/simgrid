/***************** Centralized Mutual Exclusion Algorithm *********************/
/* This example implements a centralized mutual exclusion algorithm.          */
/* LTL property checked : !(GFcs)                                            */
/******************************************************************************/

#include "msg/msg.h"
#include "mc/mc.h"
#include "xbt/automaton.h"
#include "centralized_liveness.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(centralized, "my log messages");
 
int cs = 0;

int predCS(){
  return cs;
}


int coordinator(int argc, char **argv);
int client(int argc, char **argv);

int coordinator(int argc, char *argv[])
{
  int CS_used = 0;              // initially the CS is idle
  
  while (1) {
    m_task_t task = NULL;
    MSG_task_receive(&task, "coordinator");
    const char *kind = MSG_task_get_name(task); //is it a request or a release?
    if (!strcmp(kind, "request")) {     // that's a request
      char *req = MSG_task_get_data(task);
      if (CS_used) { 
        XBT_INFO("CS already used.");
  m_task_t answer = MSG_task_create("not grant", 0, 1000, NULL);
        MSG_task_send(answer, req);
      } else {                  // can serve it immediatly
        XBT_INFO("CS idle. Grant immediatly");
        m_task_t answer = MSG_task_create("grant", 0, 1000, NULL);
        MSG_task_send(answer, req);
        CS_used = 1;
      }
    } else {                    // that's a release. Check if someone was waiting for the lock
      XBT_INFO("CS release. resource now idle");
      CS_used = 0;
    }
    MSG_task_destroy(task);
  }
  
  return 0;
}

int client(int argc, char *argv[])
{
  int my_pid = MSG_process_get_PID(MSG_process_self());
  char *my_mailbox = bprintf("%s", argv[1]);
  const char* kind;
 
  while(1){

    XBT_INFO("Client (%s) asks the request", my_mailbox);
    MSG_task_send(MSG_task_create("request", 0, 1000, my_mailbox),
                  "coordinator");
    // wait the answer
    m_task_t answer = NULL;
    MSG_task_receive(&answer, my_mailbox);

    kind = MSG_task_get_name(answer);
    
    if (!strcmp(kind, "grant")) {

      XBT_INFO("Client (%s) got the answer (grant). Sleep a bit and release it", my_mailbox);

      if(!strcmp(my_mailbox, "1"))
  cs = 1;

      /*MSG_process_sleep(my_pid);
      MSG_task_send(MSG_task_create("release", 0, 1000, NULL),
        "coordinator");
        XBT_INFO("Client (%s) releases the CS", my_mailbox);
      
      if(!strcmp(my_mailbox, "1"))
  cs = 0;*/
      
    }else{
      
      XBT_INFO("Client (%s) got the answer (not grant). Try again", my_mailbox);
      
    }

    MSG_task_destroy(answer);
    
    MSG_process_sleep(my_pid);
  }

  return 0;
}

int main(int argc, char *argv[])
{
  
  xbt_automaton_t a = MC_create_automaton("promela_centralized_liveness");
  xbt_new_propositional_symbol(a,"cs", &predCS); 
  
  MSG_global_init(&argc, argv);
  MSG_create_environment("../msg_platform.xml");
  MSG_function_register("coordinator", coordinator);
  MSG_function_register("client", client);
  MSG_launch_application("deploy_centralized_liveness.xml");
  MSG_main_liveness(a);

  return 0;

}
