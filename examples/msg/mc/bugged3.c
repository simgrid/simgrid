/**************** Shared buffer between asynchronous receives *****************/
/* Server process assumes that the data from the second communication comm2   */
/* will overwrite the one from the first communication, because of the order  */
/* of the wait calls. This is not true because data copy can be triggered by  */
/* a call to wait on the other end of the communication (client).             */
/* NOTE that the communications use different mailboxes, but they share the   */
/* same buffer for reception (task1).                                         */
/******************************************************************************/

#include <msg/msg.h>
#include <mc/modelchecker.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(bugged3, "this example");

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
  INFO1("Received %lu", val1);

  MC_assert(val1 == 2);

  INFO0("OK");
  return 0;
}

int client(int argc, char *argv[])
{
  msg_comm_t comm;
  char *mbox;
  m_task_t task1 =
      MSG_task_create("task", 0, 10000, (void *) atol(argv[1]));

  mbox = bprintf("mymailbox%s", argv[1]);

  INFO1("Send %d!", atoi(argv[1]));
  comm = MSG_task_isend(task1, mbox);
  MSG_comm_wait(comm, -1);

  xbt_free(mbox);

  return 0;
}

int main(int argc, char *argv[])
{
  MSG_global_init(&argc, argv);

  MSG_create_environment("platform.xml");

  MSG_function_register("server", server);

  MSG_function_register("client", client);

  MSG_launch_application("deploy_bugged3.xml");

  MSG_main();

  return 0;
}
