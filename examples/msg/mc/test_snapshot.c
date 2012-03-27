#include "msg/msg.h"
#include "mc/mc.h"
#include "xbt/automaton.h"
#include "xbt/automatonparse_promela.h"
#include "test_snapshot.h"
#include "y.tab.c"
#include <stdlib.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(test_snapshot, "my log messages");

int r=0; 
int cs=0;

int i = 1;
xbt_dynar_t d1 = NULL;
xbt_dynar_t d2 = NULL;
char *c1;

int predR(){
  return r;
}

int predCS(){
  return cs;
}

void check(){
  XBT_INFO("*** Check ***"); 
  if(d1!=NULL){
    XBT_INFO("Dynar d1 (%p -> %p) length : %lu", &d1, d1, xbt_dynar_length(d1));
    unsigned int cursor = 0;
    char *elem;
    xbt_dynar_foreach(d1, cursor, elem){
      XBT_INFO("Elem in dynar d1 : %s", elem);
    }
  }else{
    XBT_INFO("Dynar d1 NULL");
  }
  if(d2!=NULL){
    XBT_INFO("Dynar d2 (%p -> %p) length : %lu", &d2, d2, xbt_dynar_length(d2));
    unsigned int cursor = 0;
    char *elem;
    xbt_dynar_foreach(d2, cursor, elem){
      XBT_INFO("Elem in dynar d2 : %s", elem);
    }
  }else{
    XBT_INFO("Dynar d2 NULL");
  }
}


int coordinator(int argc, char *argv[])
{

  while(i>0){

    m_task_t task = NULL;
    MSG_task_receive(&task, "coordinator");
    const char *kind = MSG_task_get_name(task);

    //check();

    if (!strcmp(kind, "request")) { 
      char *req = MSG_task_get_data(task);
      m_task_t answer = MSG_task_create("received", 0, 1000, NULL);
      MSG_task_send(answer, req); 
    }else{
      XBT_INFO("End of coordinator");
    }

  }

  return 0;
}

int client(int argc, char *argv[])
{
  int my_pid = MSG_process_get_PID(MSG_process_self());

  char *my_mailbox = bprintf("%s", argv[1]);

  while(i>0){

    XBT_INFO("Ask the request");
    MSG_task_send(MSG_task_create("request", 0, 1000, my_mailbox), "coordinator");

    r = 1;

    check();

    // wait the answer
    m_task_t task = NULL;
    MSG_task_receive(&task, my_mailbox);
    MSG_task_destroy(task);

    check();
 
    XBT_INFO("*** Update ***"); 
    xbt_dynar_reset(d1);
    free(c1);
    xbt_dynar_free(&d1);
    d2 = xbt_dynar_new(sizeof(char *), NULL);
    XBT_INFO("Dynar d2 : %p -> %p", &d2, d2);
    char *c2 = strdup("boooooooo");
    xbt_dynar_push(d2, &c2);

    cs = 1;

    check();
 
    MSG_process_sleep(1);
    MSG_task_send(MSG_task_create("release", 0, 1000, NULL), "coordinator");

    check();

    MSG_process_sleep(my_pid);

    i--;
  }
    
  return 0;
}

int main(int argc, char *argv[])
{

  d1 = xbt_dynar_new(sizeof(char *), NULL);
  XBT_DEBUG("Dynar d1 : %p -> %p", &d1, d1);
  c1 = strdup("coucou");
  xbt_dynar_push(d1, &c1);
  xbt_dynar_push(d1, &c1);

  init();
  yyparse();
  automaton = get_automaton();
  xbt_new_propositional_symbol(automaton,"r", &predR); 
  xbt_new_propositional_symbol(automaton,"cs", &predCS); 
  
  MSG_global_init(&argc, argv);
  MSG_create_environment("../msg_platform.xml");
  MSG_function_register("coordinator", coordinator);
  MSG_function_register("client", client);
  MSG_launch_application("deploy_test_snapshot.xml");
  MSG_main_liveness(automaton, argv[0]);

  return 0;
}
