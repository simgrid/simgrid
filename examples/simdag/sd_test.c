#include <stdio.h>
#include <stdlib.h>
#include "simdag/simdag.h"
#include "xbt/ex.h"

int main(int argc, char **argv) {
  
  /* initialisation of SD */
  SD_init(&argc, argv);

  if (argc < 2) {
     printf ("Usage: %s platform_file\n", argv[0]);
     printf ("example: %s msg_platform.xml\n", argv[0]);
     exit(1);
  }

  /* creation of the environment */
  char * platform_file = argv[1];
  SD_create_environment(platform_file);

  /* creation of the tasks and their dependencies */
  SD_task_t taskA = SD_task_create("Task A", NULL, 10.0);
  SD_task_t taskB = SD_task_create("Task B", NULL, 40.0);
  SD_task_t taskC = SD_task_create("Task C", NULL, 30.0);
  SD_task_t taskD = SD_task_create("Task D", NULL, 60.0);
  

  SD_task_dependency_add(NULL, NULL, taskB, taskA);
  SD_task_dependency_add(NULL, NULL, taskC, taskA);
  SD_task_dependency_add(NULL, NULL, taskD, taskB);
  SD_task_dependency_add(NULL, NULL, taskD, taskC);
  /*  SD_task_dependency_add(NULL, NULL, taskA, taskD); /\* deadlock */

/*   xbt_ex_t ex; */

/*   TRY { */
/*     SD_task_dependency_add(NULL, NULL, taskA, taskA); /\* shouldn't work and must raise an exception *\/ */
/*     xbt_assert0(0, "Hey, I can add a dependency between Task A and Task A!"); */
/*   } */
/*   CATCH (ex) { */
/*   } */
  
/*   TRY { */
/*     SD_task_dependency_add(NULL, NULL, taskA, taskB); /\* shouldn't work and must raise an exception *\/ */
/*     xbt_assert0(0, "Oh oh, I can add an already existing dependency!"); */
/*   } */
/*   CATCH (ex) { */
/*   } */

/*   SD_task_dependency_remove(taskA, taskB); */

/*   TRY { */
/*     SD_task_dependency_remove(taskC, taskA); /\* shouldn't work and must raise an exception *\/ */
/*     xbt_assert0(0, "Dude, I can remove an unknown dependency!"); */
/*   } */
/*   CATCH (ex) { */
/*   } */

/*   TRY { */
/*     SD_task_dependency_remove(taskC, taskC); /\* shouldn't work and must raise an exception *\/ */
/*     xbt_assert0(0, "Wow, I can remove a dependency between Task C and itself!"); */
/*   } */
/*   CATCH (ex) { */
/*   } */


  /* if everything is ok, no exception is forwarded or rethrown by main() */

  /* watch points */
  /*  SD_task_watch(taskB, SD_DONE);*/


  /* let's launch the simulation! */

  int workstation_number = 2;
  SD_workstation_t *workstation_list = SD_workstation_get_list();
  double computation_amount[] = {100, 200};
  double communication_amount[] =
    {
      0, 30,
      20, 0
    };
  double rate = 1;

  SD_task_schedule(taskA, workstation_number, workstation_list,
		   computation_amount, communication_amount, rate);
  SD_task_schedule(taskB, workstation_number, workstation_list,
		   computation_amount, communication_amount, rate);
  SD_task_schedule(taskC, workstation_number, workstation_list,
		   computation_amount, communication_amount, rate);
  SD_task_schedule(taskD, workstation_number, workstation_list,
		   computation_amount, communication_amount, rate);

  printf("Launching simulation...\n");
  SD_simulate(100);

  SD_task_destroy(taskA);
  SD_task_destroy(taskB);
  SD_task_destroy(taskC);
  SD_task_destroy(taskD);
  SD_exit();
  return 0;
}
