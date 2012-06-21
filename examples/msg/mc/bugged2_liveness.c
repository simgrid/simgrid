/***************** Producer/Consumer Algorithm *************************/
/* This example implements a producer/consumer algorithm.              */
/* If consumer work before producer, message is empty                  */
/***********************************************************************/


#include "msg/msg.h"
#include "mc/mc.h"
#include "xbt/automaton.h"
#include "bugged2_liveness.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(bugged2_liveness, "my log messages");

char* buffer;

int consume = 0;
int produce = 0;
int cready = 0;
int pready = 0;

int predPready(){
  return pready;
}

int predCready(){
  return cready;
}

int predConsume(){
  return consume;
}

int predProduce(){
  return produce;
}

int coordinator(int argc, char *argv[])
{
  xbt_dynar_t requests = xbt_dynar_new(sizeof(char *), NULL);
  int CS_used = 0;

  while(1) {
    m_task_t task = NULL;
    MSG_task_receive(&task, "coordinator");
    const char *kind = MSG_task_get_name(task);
    if (!strcmp(kind, "request")) {
      char *req = MSG_task_get_data(task);
      if (CS_used) {
  XBT_INFO("CS already used. Queue the request");
  xbt_dynar_push(requests, &req);
      } else {
  m_task_t answer = MSG_task_create("grant", 0, 1000, NULL);
  MSG_task_send(answer, req);
  CS_used = 1;
  XBT_INFO("CS idle. Grant immediatly");
      }
    } else {
      if (xbt_dynar_length(requests) > 0) {
  XBT_INFO("CS release. Grant to queued requests");
  char *req;
  xbt_dynar_pop(requests, &req);
  MSG_task_send(MSG_task_create("grant", 0, 1000, NULL), req);
      } else {
  XBT_INFO("CS_realase, ressource now idle");
  CS_used = 0;
      }
    }

    MSG_task_destroy(task);

  }

  return 0;

}

int producer(int argc, char *argv[])
{

  char * my_mailbox = bprintf("%s", argv[1]);
  
  while(1) {
    
    /* Create message */
    const char *mess = "message";

    pready = 1;
    XBT_INFO("pready = 1");
    
    /* CS request */
    XBT_INFO("Producer ask the request");
    MSG_task_send(MSG_task_create("request", 0, 1000, my_mailbox), "coordinator");

    /* Wait the answer */
    m_task_t grant = NULL;
    MSG_task_receive(&grant, my_mailbox);
    MSG_task_destroy(grant);

    /* Push message (size of buffer = 1) */
    buffer = strdup(mess);

    produce = 1;
    XBT_INFO("produce = 1");

    /* CS release */
    MSG_task_send(MSG_task_create("release", 0, 1000, my_mailbox), "coordinator");

    produce = 0;
    pready = 0;

    XBT_INFO("pready et produce = 0");

  }

  return 0;

}

int consumer(int argc, char *argv[])
{

  char * my_mailbox = bprintf("%s", argv[1]);
  char *mess;


  while(1) {
    
    /* CS request */
    XBT_INFO("Consumer ask the request");
    MSG_task_send(MSG_task_create("request", 0, 1000, my_mailbox), "coordinator");

    cready = 1;
    XBT_INFO("cready = 1");

    /* Wait the answer */
    m_task_t grant = NULL;
    MSG_task_receive(&grant, my_mailbox);
    MSG_task_destroy(grant);

    /* Pop message  */
    mess = malloc(8*sizeof(char));
    mess = strdup(buffer);
    buffer[0] = '\0'; 

    /* Display message */
    XBT_INFO("Message : %s", mess);
    if(strcmp(mess, "") != 0){
      consume = 1;
      XBT_INFO("consume = 1");
    }

    /* CS release */
    MSG_task_send(MSG_task_create("release", 0, 1000, my_mailbox), "coordinator");

    free(mess);

    consume = 0;
    cready = 0;

    XBT_INFO("cready et consume = 0");

  }

  return 0;

}


int main(int argc, char *argv[])
{

  buffer = malloc(8*sizeof(char));
  buffer[0]='\0';

  MSG_init(&argc, argv);

  MSG_config("model-check/property","promela2_bugged2_liveness");
  MC_automaton_new_propositional_symbol("pready", &predPready);
  MC_automaton_new_propositional_symbol("cready", &predCready);
  MC_automaton_new_propositional_symbol("consume", &predConsume);
  MC_automaton_new_propositional_symbol("produce", &predProduce);
  
  MSG_create_environment("../msg_platform.xml");
  MSG_function_register("coordinator", coordinator);
  MSG_function_register("consumer", consumer);
  MSG_function_register("producer", producer);
  MSG_launch_application("deploy_bugged2_liveness.xml");
  MSG_main();

  return 0;

}
