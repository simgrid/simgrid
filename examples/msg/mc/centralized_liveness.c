#include "msg/msg.h"
#include "mc/mc.h"
#include "xbt/automaton.h"
#include "xbt/automatonparse_promela.h"
#include "centralized_liveness.h"
#include "y.tab.c"

#define AMOUNT_OF_CLIENTS 2

XBT_LOG_NEW_DEFAULT_CATEGORY(centralized, "my log messages");
 
int cs2 = 0;

int predCS2(){
  return cs2;
}


int coordinator(int argc, char **argv);
int client(int argc, char **argv);

int coordinator(int argc, char *argv[])
{
  xbt_dynar_t requests = xbt_dynar_new(sizeof(char *), NULL);   // dynamic vector storing requests (which are char*)
  int CS_used = 0;              // initially the CS is idle
  
  while (1) {
    m_task_t task = NULL;
    MSG_task_receive(&task, "coordinator");
    const char *kind = MSG_task_get_name(task); //is it a request or a release?
    if (!strcmp(kind, "request")) {     // that's a request
      char *req = MSG_task_get_data(task);
      if (CS_used) {            // need to push the request in the vector
        XBT_INFO("CS already used. Queue the request");
        xbt_dynar_push(requests, &req);
      } else {                  // can serve it immediatly
        XBT_INFO("CS idle. Grant immediatly");
        m_task_t answer = MSG_task_create("grant", 0, 1000, NULL);
        MSG_task_send(answer, req);
        CS_used = 1;
      }
    } else {                    // that's a release. Check if someone was waiting for the lock
      if (!xbt_dynar_is_empty(requests)) {
        XBT_INFO("CS release. Grant to queued requests (queue size: %lu)",
              xbt_dynar_length(requests));
        char *req;
        xbt_dynar_pop(requests, &req);
        MSG_task_send(MSG_task_create("grant", 0, 1000, NULL), req);
      } else {                  // nobody wants it
        XBT_INFO("CS release. resource now idle");
        CS_used = 0;
      }
    }
    MSG_task_destroy(task);
  }
  
  return 0;
}

int client(int argc, char *argv[])
{
  int my_pid = MSG_process_get_PID(MSG_process_self());
  char *my_mailbox = bprintf("%s", argv[1]);
 
  while(1){

    if(!strcmp(my_mailbox, "2"))
      cs2 = 0;

    XBT_INFO("Client (%s) ask the request", my_mailbox);
    MSG_task_send(MSG_task_create("request", 0, 1000, my_mailbox),
                  "coordinator");
    // wait the answer
    m_task_t grant = NULL;
    MSG_task_receive(&grant, my_mailbox);
    MSG_task_destroy(grant);
    XBT_INFO("got the answer. Sleep a bit and release it");

    if(!strcmp(my_mailbox, "2"))
      cs2 = 1;

    MSG_process_sleep(1);
    MSG_task_send(MSG_task_create("release", 0, 1000, NULL),
                  "coordinator");
    MSG_process_sleep(my_pid);
  }

  return 0;
}

int main(int argc, char *argv[])
{
  init();
  yyparse();
  automaton = get_automaton();
  xbt_new_propositional_symbol(automaton,"cs2", &predCS2); 
  
  MSG_global_init(&argc, argv);
  MSG_create_environment("../msg_platform.xml");
  MSG_function_register("coordinator", coordinator);
  MSG_function_register("client", client);
  MSG_launch_application("deploy_centralized_liveness.xml");
  MSG_main_liveness(automaton, argv[0]);

  return 0;

}
