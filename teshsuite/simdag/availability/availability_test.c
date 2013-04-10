#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <simdag/simdag.h>
#include <xbt/log.h>
#include <xbt/ex.h>
#include <signal.h>


typedef struct {
  FILE *daxFile;
  FILE *envFile;
} XMLfiles;


static void usage(char *name)
{
  fprintf(stdout, "Error on parameters.\n");
  fprintf(stdout, "usage: %s <XML environment file>  <DAX file>\n", name);
}

static void checkParameters(int argc, char *argv[])
{
  if (argc != 3) {
    int i;
    printf("====%d===\n",argc);
    for(i=0; i<argc; i++) {
      printf("\t%s\n",argv[i]);
    }
    usage(argv[0]);
    exit(-1);
  }
 
 /* Check that files exist */
  XMLfiles xmlFiles;
  if ((xmlFiles.envFile = fopen(argv[1], "r")) == NULL) {
    fprintf(stderr, "Error while opening XML file %s.\n", argv[1]);
    exit(-1);
  }
  fclose(xmlFiles.envFile);

  if ((xmlFiles.daxFile = fopen(argv[2], "r")) == NULL) {
    fprintf(stderr, "Error while opening DAX file %s.\n", argv[2]);
    exit(-1);
  }
  fclose(xmlFiles.daxFile);
}

static int name_compare_hosts(const void *n1, const void *n2)
{
  char name1[80], name2[80];
  strcpy(name1, SD_workstation_get_name(*((SD_workstation_t *) n1)));
  strcpy(name2, SD_workstation_get_name(*((SD_workstation_t *) n2)));

  return strcmp(name1, name2);
}

static void scheduleDAX(xbt_dynar_t dax)
{
  unsigned int cursor;
  SD_task_t task;

  const SD_workstation_t *ws_list = SD_workstation_get_list();
  int totalHosts = SD_workstation_get_number();
  qsort((void *) ws_list, totalHosts, sizeof(SD_workstation_t),
        name_compare_hosts);

  int count = SD_workstation_get_number();
  //fprintf(stdout, "No. workstations: %d, %d\n", count, (dax != NULL));

  xbt_dynar_foreach(dax, cursor, task) {
    if (SD_task_get_kind(task) == SD_TASK_COMP_SEQ) {
      if (!strcmp(SD_task_get_name(task), "end")
          || !strcmp(SD_task_get_name(task), "root")) {
        fprintf(stdout, "Scheduling %s to node: %s\n", SD_task_get_name(task),
                SD_workstation_get_name(ws_list[0]));
        SD_task_schedulel(task, 1, ws_list[0]);
      } else {
        fprintf(stdout, "Scheduling %s to node: %s\n", SD_task_get_name(task),
                SD_workstation_get_name(ws_list[(cursor) % count]));
        SD_task_schedulel(task, 1, ws_list[(cursor) % count]);
      }
    }
  }
}

/* static void printTasks(xbt_dynar_t completedTasks) */
/* { */
/* 	unsigned int cursor; */
/* 	SD_task_t task; */

/* 	xbt_dynar_foreach(completedTasks, cursor, task) */
/* 	{ */
/* 		if(SD_task_get_state(task) == SD_DONE) */
/* 		{ */
/* 			fprintf(stdout, "Task done: %s, %f, %f\n", */
/* 						SD_task_get_name(task), SD_task_get_start_time(task), SD_task_get_finish_time(task)); */
/* 		} */
/* 	} */
/* } */


/* void createDottyFile(xbt_dynar_t dax, char *filename) */
/* { */
/* 	char filename2[1000]; */
/* 	unsigned int cursor; */
/* 	SD_task_t task; */

/* 	sprintf(filename2, "%s.dot", filename); */
/* 	FILE *dotout = fopen(filename2, "w"); */
/* 	fprintf(dotout, "digraph A {\n"); */
/* 	xbt_dynar_foreach(dax, cursor, task) */
/* 	{ */
/* 		SD_task_dotty(task, dotout); */
/* 	} */
/* 	fprintf(dotout, "}\n"); */
/* 	fclose(dotout); */
/* } */

static xbt_dynar_t initDynamicThrottling(int *argc, char *argv[])
{
  /* Initialize SD */
  SD_init(argc, argv);

  /* Check parameters */
  checkParameters(*argc,argv);

  /* Create environment */
  SD_create_environment(argv[1]);
  /* Load DAX file */
  xbt_dynar_t dax = SD_daxload(argv[2]);

  //  createDottyFile(dax, argv[2]);

  // Schedule DAX
  fprintf(stdout, "Scheduling DAX...\n");
  scheduleDAX(dax);
  fprintf(stdout, "DAX scheduled\n");
  xbt_dynar_t ret = SD_simulate(-1);
  xbt_dynar_free(&ret);
  fprintf(stdout, "Simulation end. Time: %f\n", SD_get_clock());

  return dax;
}

/**
 * Garbage collector :D
 */
static void garbageCollector(xbt_dynar_t dax)
{
  while (!xbt_dynar_is_empty(dax)) {
    SD_task_t task = xbt_dynar_pop_as(dax, SD_task_t);
    SD_task_destroy(task);
  }
  xbt_dynar_free(&dax);
  SD_exit();
}



/**
 * Main procedure
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[])
{

  /* Start process... */
  xbt_dynar_t dax = initDynamicThrottling(&argc, argv);

  // Free memory
  garbageCollector(dax);
  return 0;
}
