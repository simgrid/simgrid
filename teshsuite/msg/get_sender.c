#include <stdio.h>
#include "msg/msg.h"
#include <float.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(test,"Messages specific to this example");


static int send(int argc, char *argv[]){
  INFO0("Sending");
  MSG_task_put(MSG_task_create("Blah", 0.0, 0.0, NULL),
	       MSG_host_self(), 0);
  MSG_process_sleep(1.); /* FIXME: if the sender exits before the receiver calls get_sender(), bad thing happens */
  INFO0("Exiting");
  return 0;
}

static int receive(int argc, char *argv[]) {
  INFO0("Receiving");
  m_task_t task = NULL;
  MSG_task_get_with_timeout(&task, 0, DBL_MAX);
  xbt_assert0(MSG_task_get_sender(task), "No sender received");
  INFO1("Got a message sent by '%s'", MSG_process_get_name(MSG_task_get_sender(task)));
  return 0;
}

/** Main function */
int main(int argc, char *argv[]) {
  MSG_error_t res = MSG_OK;

  MSG_global_init(&argc,argv);
  MSG_set_channel_number(100);

  /*   Application deployment */
  MSG_function_register("send", &send);
  MSG_function_register("receive", &receive);

  MSG_create_environment(argv[1]);
  MSG_launch_application(argv[1]);
  res = MSG_main();
  MSG_clean();
  if(res==MSG_OK) return 0;
  else return 1;
}

