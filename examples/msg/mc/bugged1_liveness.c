/***************** Centralized Mutual Exclusion Algorithm *********************/
/* This example implements a centralized mutual exclusion algorithm.          */
/* Bug : CS requests of client 1 not satisfied                                      */
/* LTL property checked : G(r->F(cs)); (r=request of CS, cs=CS ok)            */
/******************************************************************************/

#include "msg/msg.h"
#include "mc/mc.h"
#include "xbt/automaton.h"
#include "bugged1_liveness.h"

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

  int CS_used = 0;   
  msg_task_t task = NULL, answer = NULL; 
  xbt_dynar_t requests = xbt_dynar_new(sizeof(char *), NULL);

  while(1){  
    MSG_task_receive(&task, "coordinator");
    const char *kind = MSG_task_get_name(task); 
    if (!strcmp(kind, "request")) {    
      char *req = MSG_task_get_data(task);
      if (CS_used) {           
        XBT_INFO("CS already used. Queue the request.");
        xbt_dynar_push(requests, &req);
      } else {               
        if(strcmp(req, "1") != 0){
          XBT_INFO("CS idle. Grant immediatly");
          answer = MSG_task_create("grant", 0, 1000, NULL);
          MSG_task_send(answer, req);
          CS_used = 1;
          answer = NULL;
        }
      }
    } else {      
      if (!xbt_dynar_is_empty(requests)) {
        XBT_INFO("CS release. Grant to queued requests (queue size: %lu)", xbt_dynar_length(requests));
        char *req;
        xbt_dynar_pop(requests, &req);
        if(strcmp(req, "1") != 0){
          MSG_task_send(MSG_task_create("grant", 0, 1000, NULL), req);
        }else{
          xbt_dynar_push(requests, &req);
          CS_used = 0;
        }
      }else{
        XBT_INFO("CS release. resource now idle");
        CS_used = 0;
      }
    }
    MSG_task_destroy(task);
    task = NULL;
    kind = NULL;
  }
 
  return 0;
}

int client(int argc, char *argv[])
{
  int my_pid = MSG_process_get_PID(MSG_process_self());

  char *my_mailbox = xbt_strdup(argv[1]);
  msg_task_t grant = NULL, release = NULL;
    
  while(1){
    XBT_INFO("Ask the request");
    MSG_task_send(MSG_task_create("request", 0, 1000, my_mailbox), "coordinator");

    if(strcmp(my_mailbox, "1") == 0){
      r = 1;
      cs = 0;
      XBT_INFO("Propositions changed : r=1, cs=0");
    }

    MSG_task_receive(&grant, my_mailbox);
    const char *kind = MSG_task_get_name(grant);

    if((strcmp(my_mailbox, "1") == 0) && (strcmp("grant", kind) == 0)){
      cs = 1;
      r = 0;
      XBT_INFO("Propositions changed : r=0, cs=1");
    }

    MSG_task_destroy(grant);
    grant = NULL;
    kind = NULL;

    XBT_INFO("%s got the answer. Sleep a bit and release it", argv[1]);

    MSG_process_sleep(1);

    release = MSG_task_create("release", 0, 1000, NULL);
    MSG_task_send(release, "coordinator");

    release = NULL;

    MSG_process_sleep(my_pid);
    
    if(strcmp(my_mailbox, "1") == 0){
      cs=0;
      r=0;
      XBT_INFO("Propositions changed : r=0, cs=0");
    }
    
  }

  return 0;
}

int main(int argc, char *argv[])
{

  MSG_init(&argc, argv);

  char **options = &argv[1];

  MSG_config("model-check/property","promela_bugged1_liveness");
  MC_automaton_new_propositional_symbol("r", &predR);
  MC_automaton_new_propositional_symbol("cs", &predCS);

  const char* platform_file = options[0];
  const char* application_file = options[1];
  
  MSG_create_environment(platform_file);
  MSG_function_register("coordinator", coordinator);
  MSG_function_register("client", client);
  MSG_launch_application(application_file);
  MSG_main();

  return 0;
}
