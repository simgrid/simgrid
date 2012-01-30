#include <msg/msg.h>
/* Create a log channel to have nice outputs. */
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
                             "Messages specific for this msg example");
#define MAX_ITER 200000
#define WORK 100000

int master(int argc, char **argv);
int slave(int argc, char **argv);

MSG_error_t test_all(const char *, const char *);
int main(int argc, char *argv[]);

int master(int argc, char** argv)
{
  int i,j, id;
  m_task_t task;
  char mailbox[80];

  sscanf(argv[1], "%d", &id);
  sprintf(mailbox, "slave-%d", id);

  for(i=0; i < MAX_ITER; i++){
    task = MSG_task_create("test", 100000000, 1, NULL);

    for(j=0; j < WORK; j++);

    MSG_task_send(task, mailbox);
    XBT_INFO("Task sent to %s", mailbox);
  }

  return 0;
}

int slave(int argc, char **argv)
{
  int i, id;
  m_task_t task;
  char mailbox[80];

  sscanf(argv[1], "%d", &id);
  sprintf(mailbox, "slave-%d", id);

  for(i=0; i < MAX_ITER; i++){
    MSG_task_receive(&task, mailbox);
    XBT_INFO("Task received to %s", mailbox);
    MSG_task_destroy(task);
    task=NULL;
  }

  return 0;
}

/** Test function */
MSG_error_t test_all(const char *platform_file,
                     const char *application_file)
{
  MSG_error_t res = MSG_OK;

  /* MSG_config("workstation/model","KCCFLN05"); */
  {                             /*  Simulation setting */
    MSG_create_environment(platform_file);
  }
  {                             /*   Application deployment */
    MSG_function_register("master", master);
    MSG_function_register("slave", slave);
    MSG_launch_application(application_file);
  }
  res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());
  return res;
}                               /* end_of_test_all */


/** Main function */
int main(int argc, char *argv[])
{
  MSG_error_t res = MSG_OK;

  MSG_global_init(&argc, argv);
  if (argc < 3) {
    printf("Usage: %s platform_file deployment_file\n", argv[0]);
    printf("example: %s msg_platform.xml msg_deployment.xml\n", argv[0]);
    exit(1);
  }
  res = test_all(argv[1], argv[2]);
  MSG_clean();

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}                               /* end_of_main */
