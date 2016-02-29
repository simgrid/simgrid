/* Bug report: https://github.com/mquinson/simgrid/issues/40
 * 
 * Task.listen used to be on async mailboxes as it always returned false.
 * This occures in Java and C, but is only tested here in C.
 */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

static int server(int argc, char *argv[])
{
   msg_task_t task =  MSG_task_create("a", 0, 0, (char*)"Some data");
   MSG_task_isend(task, "mailbox");

   xbt_assert(MSG_task_listen("mailbox")); // True (1)
   XBT_INFO("Task listen works on regular mailboxes");
   task = NULL;
   MSG_task_receive(&task, "mailbox");
   xbt_assert(!strcmp("Some data", MSG_task_get_data(task)), "Data received: %s", (char*)MSG_task_get_data(task));
   MSG_task_destroy(task);
   XBT_INFO("Data successfully received from regular mailbox");

   MSG_mailbox_set_async("mailbox2");
   task = MSG_task_create("b", 0, 0, (char*)"More data");
   MSG_task_isend(task, "mailbox2");

   xbt_assert(MSG_task_listen("mailbox2")); // used to break.
   XBT_INFO("Task listen works on asynchronous mailboxes");
   task = NULL;
   MSG_task_receive(&task, "mailbox2");
   xbt_assert(!strcmp("More data", MSG_task_get_data(task)));
   MSG_task_destroy(task);
   XBT_INFO("Data successfully received from asynchronous mailbox");

   return 0;
}

int main(int argc, char *argv[])
{
   MSG_init(&argc, argv);
   xbt_assert(argc==2);
   MSG_create_environment(argv[1]);
   MSG_process_create("test", server, NULL, MSG_host_by_name("Tremblay"));
   MSG_main();

   return 0;
}
