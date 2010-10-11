/******************** Non-deterministic message ordering  *********************/
/* Server assumes a fixed order in the reception of messages from its clients */
/* which is incorrect because the message ordering is non-deterministic       */
/******************************************************************************/

#include <msg/msg.h>
#include <mc/modelchecker.h>
#define N 3

XBT_LOG_NEW_DEFAULT_CATEGORY(example, "this example");

int server(int argc, char *argv[]);
int client(int argc, char *argv[]);

int server(int argc, char *argv[])
{
  m_task_t task1, task2;
  long val1, val2;

  MSG_task_receive(&task1, "mymailbox");
  val1 = (long) MSG_task_get_data(task1);
  INFO1("Received %lu", val1);

  MSG_task_receive(&task2, "mymailbox");
  val2 = (long) MSG_task_get_data(task2);
  INFO1("Received %lu", val2);

  MC_assert(min(val1, val2) == 1);

  INFO0("OK");
  return 0;
}

int client(int argc, char *argv[])
{
  m_task_t task1 =
      MSG_task_create("task", 0, 10000, (void *) atol(argv[1]));
  m_task_t task2 =
      MSG_task_create("task", 0, 10000, (void *) atol(argv[1]));

  INFO1("Send %d!", atoi(argv[1]));
  MSG_task_send(task1, "mymailbox");

  INFO1("Send %d!", atoi(argv[1]));
  MSG_task_send(task2, "mymailbox");

  return 0;
}

int main(int argc, char *argv[])
{
  MSG_global_init(&argc, argv);

  MSG_create_environment("platform.xml");

  MSG_function_register("server", server);

  MSG_function_register("client", client);

  MSG_launch_application("deploy_bugged2.xml");

  MSG_main();

  return 0;
}
