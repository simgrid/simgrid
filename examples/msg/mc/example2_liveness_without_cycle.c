/**************** Shared buffer between asynchronous receives *****************/
/* Server process assumes that the data from the second communication comm2   */
/* will overwrite the one from the first communication, because of the order  */
/* of the wait calls. This is not true because data copy can be triggered by  */
/* a call to wait on the other end of the communication (client).             */
/* NOTE that the communications use different mailboxes, but they share the   */
/* same buffer for reception (task1).                                         */
/******************************************************************************/

#include "xbt/automaton.h"
#include "xbt/automatonparse_promela.h"
#include "example2_liveness_without_cycle.h"
#include "msg/msg.h"
#include "mc/mc.h"

#include "y.tab.c"

XBT_LOG_NEW_DEFAULT_CATEGORY(example_liveness_with_cycle, "Example liveness with cycle");

extern xbt_automaton_t automaton;


int p=0;
int q=0;


int predP(){
  return p;
}


int predQ(){
  return q;
}


int server(int argc, char *argv[]);
int client(int argc, char *argv[]);

int server(int argc, char *argv[])
{
  m_task_t task1;
  long val1;
  msg_comm_t comm1, comm2;

  comm1 = MSG_task_irecv(&task1, "mymailbox1");
  comm2 = MSG_task_irecv(&task1, "mymailbox2");
  MSG_comm_wait(comm1, -1);
  MSG_comm_wait(comm2, -1);

  val1 = (long) MSG_task_get_data(task1);
  XBT_INFO("Received %lu", val1);

  MC_assert_pair_stateless(val1 == 2);

  /*if(val1 == 2)
    q = 1;
  else
  q = 0;*/

  //XBT_INFO("q (server) = %d", q);

  XBT_INFO("OK");
 
  return 0;
}

int client(int argc, char *argv[])
{
  msg_comm_t comm;
  char *mbox;
  m_task_t task1 =
      MSG_task_create("task", 0, 10000, (void *) atol(argv[1]));

  mbox = bprintf("mymailbox%s", argv[1]);

  XBT_INFO("Send %d!", atoi(argv[1]));
  comm = MSG_task_isend(task1, mbox);

  MSG_comm_wait(comm, -1);

  xbt_free(mbox);

  return 0;
}

int main(int argc, char *argv[])
{

  init();
  yyparse();
  automaton = get_automaton();
  xbt_propositional_symbol_t ps = xbt_new_propositional_symbol(automaton,"p", &predP); 
  ps = xbt_new_propositional_symbol(automaton,"q", &predQ); 
      
  //display_automaton();

  MSG_global_init(&argc, argv);

  MSG_create_environment("platform.xml");

  MSG_function_register("server", server);

  MSG_function_register("client", client);

  MSG_launch_application("deploy_bugged3.xml");

  MSG_main_liveness_stateless(automaton);

  return 0;
}
