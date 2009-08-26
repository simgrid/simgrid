#include <stdio.h>
#include <stdlib.h>
#include "msg/msg.h"
#include "xbt/log.h"
#include "xbt/asserts.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(test_trace_integration,
                             "Messages specific for this msg example");

int test_trace(int argc, char *argv[]);

/** test the trace integration cpu model */
int test_trace(int argc, char *argv[])
{
	m_task_t task;
	double task_comp_size = 2800;
	double task_prio = 1.0;

	if (argc != 3) {
		printf("Wrong number of arguments!\nUsage:\n\t1) task computational size in FLOPS\n\t2 task priority\n");
	    exit(1);
	}

	task_comp_size = atof(argv[1]);
	task_prio = atof(argv[2]);

	INFO0("Testing the trace integration cpu model: CpuTI");
	INFO1("Task size: %lf", task_comp_size);
	INFO1("Task prio: %lf", task_prio);

	/* Create and execute a single task. */
	task = MSG_task_create("proc 0", task_comp_size, 0, NULL);
	MSG_task_set_priority(task, task_prio);
	MSG_task_execute(task);

	INFO0("Test finished");


	return 0;
}

/** Main function */
int main(int argc, char *argv[])
{
  MSG_error_t res = MSG_OK;

  /* Verify if the platform xml file was passed by command line. */
  MSG_global_init(&argc, argv);
  if (argc < 2) {
    printf("Usage: %s test_trace_integration_model.xml\n", argv[0]);
    exit(1);
  }

  /* Register SimGrid process function. */
  MSG_function_register("test_trace", test_trace);
  /* Use the same file for platform and deployment. */
  MSG_create_environment(argv[1]);
  MSG_launch_application(argv[1]);
  /* Run the example. */
  res = MSG_main();

  MSG_clean();

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}
