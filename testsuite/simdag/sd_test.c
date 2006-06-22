#include <stdio.h>
#include <stdlib.h>
#include "simdag/simdag.h"
#include "xbt/ex.h"

int main(int argc, char **argv) {
  
  /* No deployment file
  if (argc < 3) {
     printf ("Usage: %s platform_file deployment_file\n", argv[0]);
     printf ("example: %s msg_platform.xml msg_deployment.xml\n", argv[0]);
     exit(1);
  }
  */

  if (argc < 2) {
     printf ("Usage: %s platform_file\n", argv[0]);
     printf ("example: %s msg_platform.xml\n", argv[0]);
     exit(1);
  }

  /* initialisation of SD */
  SD_init(&argc, argv);

  /* creation of the environment */
  char * platform_file = argv[1];
  SD_create_environment(platform_file);

  /* creation of the tasks and their dependencies */
  SD_task_t volatile taskA = SD_task_create("Task A", NULL, 10.0);
  SD_task_t volatile taskB = SD_task_create("Task B", NULL, 40.0);
  SD_task_t volatile taskC = SD_task_create("Task C", NULL, 30.0);

  SD_task_dependency_add(NULL, NULL, taskA, taskB);
  SD_task_dependency_add(NULL, NULL, taskA, taskC);

  xbt_ex_t ex;

  TRY {
    SD_task_dependency_add(NULL, NULL, taskA, taskA); /* shouldn't work and must raise an exception */
    xbt_assert0(0, "Hey, I can add a dependency between Task A and Task A!");
  }
  CATCH (ex) {
  }
  
  TRY {
    SD_task_dependency_add(NULL, NULL, taskA, taskB); /* shouldn't work and must raise an exception */
    xbt_assert0(0, "Oh oh, I can add an already existing dependency!");
  }
  CATCH (ex) {
  }

  SD_task_dependency_remove(taskA, taskB);

  TRY {
    SD_task_dependency_remove(taskC, taskA); /* shouldn't work and must raise an exception */
    xbt_assert0(0, "Dude, I can remove an unknown dependency!");
  }
  CATCH (ex) {
  }

  TRY {
    SD_task_dependency_remove(taskC, taskC); /* shouldn't work and must raise an exception */
    xbt_assert0(0, "Wow, I can remove a dependency between Task C and itself!");
  }
  CATCH (ex) {
  }
  /* if everything is ok, no exception is forwarded or rethrown by main() */

  /* watch points */
  SD_task_watch(taskA, SD_SCHEDULED);
  SD_task_watch(taskA, SD_DONE);
  SD_task_unwatch(taskA, SD_SCHEDULED);
  SD_task_watch(taskA, SD_DONE);
  SD_task_watch(taskA, SD_SCHEDULED);
  
  /* let's launch the simulation! */
  SD_simulate(100);

  SD_task_destroy(taskA);
  SD_task_destroy(taskB);
  SD_task_destroy(taskC);
  SD_exit();
  return 0;
}
