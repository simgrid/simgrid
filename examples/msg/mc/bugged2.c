/******************** Non-deterministic message ordering  *********************/
/* Server assumes a fixed order in the reception of messages from its clients */
/* which is incorrect because the message ordering is non-deterministic       */
/******************************************************************************/

#include <msg/msg.h>
#include <simgrid/modelchecker.h>
#define N 3

XBT_LOG_NEW_DEFAULT_CATEGORY(example, "this example");

int server(int argc, char *argv[]);
int client(int argc, char *argv[]);

int server(int argc, char *argv[])
{
  m_task_t task1 = NULL;
  m_task_t task2 = NULL;
  long val1, val2;

  MSG_task_receive(&task1, "mymailbox");
  val1 = (long) MSG_task_get_data(task1);
  MSG_task_destroy(task1);
  task1 = NULL;
  XBT_INFO("Received %lu", val1);

  MSG_task_receive(&task2, "mymailbox");
  val2 = (long) MSG_task_get_data(task2);
  MSG_task_destroy(task2);
  task2 = NULL;
  XBT_INFO("Received %lu", val2);

  MC_assert(min(val1, val2) == 1);

  MSG_task_receive(&task1, "mymailbox");
  val1 = (long) MSG_task_get_data(task1);
  MSG_task_destroy(task1);
  XBT_INFO("Received %lu", val1);

  MSG_task_receive(&task2, "mymailbox");
  val2 = (long) MSG_task_get_data(task2);
  MSG_task_destroy(task2);
  XBT_INFO("Received %lu", val2);

  XBT_INFO("OK");
  return 0;
}

int client(int argc, char *argv[])
{
  m_task_t task1 =
      MSG_task_create("task", 0, 10000, (void *) atol(argv[1]));
  m_task_t task2 =
      MSG_task_create("task", 0, 10000, (void *) atol(argv[1]));

  XBT_INFO("Send %d!", atoi(argv[1]));
  MSG_task_send(task1, "mymailbox");

  XBT_INFO("Send %d!", atoi(argv[1]));
  MSG_task_send(task2, "mymailbox");

  return 0;
}

int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);

  MSG_create_environment("platform.xml");

  MSG_function_register("server", server);

  MSG_function_register("client", client);

  MSG_launch_application("deploy_bugged2.xml");

  MSG_main();

  return 0;
}
