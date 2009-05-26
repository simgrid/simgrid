#include <stdio.h>
#include "msg/msg.h"            /* Yeah! If you want to use msg, you
                                   need to include msg/msg.h */
#include "xbt/sysdep.h"         /* calloc, printf */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
                             "Messages specific for this msg example");

int execute(int argc, char *argv[]);
int redistribute(int argc, char *argv[]);
MSG_error_t test_all(const char *platform_file, const char *application_file);

typedef enum {
  PORT_22 = 0,
  MAX_CHANNEL
} channel_t;


int execute(int argc, char *argv[])
{
  char buffer[32];
  int i, j;
  m_host_t *m_host_list = NULL;
  m_task_t task = NULL;
  int host_list_size;
  double *computation_duration = NULL;
  double *communication_table = NULL;
  double communication_amount;
  double computation_amount;
  double execution_time;


  host_list_size = argc - 3;
  DEBUG1("host_list_size=%d", host_list_size);
  m_host_list = calloc(host_list_size, sizeof(m_host_t));
  for (i = 1; i <= host_list_size; i++) {
    m_host_list[i - 1] = MSG_get_host_by_name(argv[i]);
    xbt_assert1(m_host_list[i - 1] != NULL,
                "Unknown host %s. Stopping Now! ", argv[i]);
  }

  xbt_assert1(sscanf(argv[argc - 2], "%lg", &computation_amount),
              "Invalid argument %s\n", argv[argc - 2]);
  xbt_assert1(sscanf(argv[argc - 1], "%lg", &communication_amount),
              "Invalid argument %s\n", argv[argc - 1]);
  computation_duration = (double *) calloc(host_list_size, sizeof(double));
  communication_table =
    (double *) calloc(host_list_size * host_list_size, sizeof(double));
  for (i = 0; i < host_list_size; i++) {
    computation_duration[i] = computation_amount / host_list_size;
    for (j = 0; j < host_list_size; j++)
      communication_table[i * host_list_size + j] =
        communication_amount / (host_list_size * host_list_size);
  }

  sprintf(buffer, "redist#0\n");
  task = MSG_parallel_task_create(buffer,
                                  host_list_size,
                                  m_host_list,
                                  computation_duration,
                                  communication_table, NULL);

  execution_time = MSG_get_clock();
  MSG_parallel_task_execute(task);
  execution_time = MSG_get_clock() - execution_time;

  INFO1("execution_time=%g ", execution_time);

  return 0;
}


int redistribute(int argc, char *argv[])
{
  char buffer[32];
  int i, j;
  m_host_t *m_host_list = NULL;
  m_task_t task = NULL;
  int host_list_size;
  double *computation_duration = NULL;
  double *communication_table = NULL;
  double communication_amount;
  double redistribution_time;


  host_list_size = argc - 2;
  DEBUG1("host_list_size=%d", host_list_size);
  m_host_list = calloc(host_list_size, sizeof(m_host_t));
  for (i = 1; i <= host_list_size; i++) {
    m_host_list[i - 1] = MSG_get_host_by_name(argv[i]);
    xbt_assert1(m_host_list[i - 1] != NULL,
                "Unknown host %s. Stopping Now! ", argv[i]);
  }

  xbt_assert1(sscanf(argv[argc - 1], "%lg", &communication_amount),
              "Invalid argument %s\n", argv[argc - 1]);
  computation_duration = (double *) calloc(host_list_size, sizeof(double));
  communication_table =
    (double *) calloc(host_list_size * host_list_size, sizeof(double));
  for (i = 0; i < host_list_size; i++) {
    for (j = 0; j < host_list_size; j++)
      communication_table[i * host_list_size + j] =
        communication_amount / (host_list_size * host_list_size);
  }

  sprintf(buffer, "redist#0\n");
  task = MSG_parallel_task_create(buffer,
                                  host_list_size,
                                  m_host_list,
                                  computation_duration,
                                  communication_table, NULL);

  redistribution_time = MSG_get_clock();
  MSG_parallel_task_execute(task);
  redistribution_time = MSG_get_clock() - redistribution_time;

  INFO1("redistribution_time=%g ", redistribution_time);

  return 0;
}


MSG_error_t test_all(const char *platform_file, const char *application_file)
{
  MSG_error_t res = MSG_OK;


  MSG_config("workstation_model", "ptask_L07");

  /*  Simulation setting */
  MSG_set_channel_number(MAX_CHANNEL);
  MSG_create_environment(platform_file);

  /*   Application deployment */
  MSG_function_register("execute", execute);
  MSG_function_register("redistribute", redistribute);
  MSG_launch_application(application_file);

  res = MSG_main();

  INFO1("Simulation time %g", MSG_get_clock());
  return res;
}


int main(int argc, char *argv[])
{
  MSG_error_t res = MSG_OK;

  MSG_global_init(&argc, argv);
  if (argc < 3) {
    printf("Usage: %s platform_file deployment_file\n", argv[0]);
    exit(1);
  }
  res = test_all(argv[1], argv[2]);
  MSG_clean();

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}
