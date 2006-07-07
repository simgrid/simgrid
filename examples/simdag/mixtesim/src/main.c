#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include "mixtesim_prototypes.h"
#include "list_scheduling.h"

#include "simdag/simdag.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(mixtesim,
			     "Logging specific to this SimDag example");

/* static int createSimgridResources(); */
static int createSimgridTasks();
static void freeSimgridTasks();

DAG dag;
/*extern Link local_link;*/

int main(int argc, char **argv) {
 
  char *platform_file;
  SD_task_t *changed_tasks;
  int i;

/*   xbt_log_control_set("sd.thres=debug"); */
/*   xbt_log_control_set("sd_kernel.thres=debug"); */
/*   xbt_log_control_set("surf_kernel.thres=debug"); */
/*   xbt_log_control_set("mixtesim.thres=debug"); */

  
  if (argc < 1) {
     printf("Usage: %s xml_platform_file dagfile ", argv[0]);
     printf("example: %s gridfile.xml dagfile", argv[0]);
     exit(1);
  }

  /* initialisation of SD */
  SD_init(&argc, argv);

  /* creation of the environment */
  platform_file = argv[1];
  SD_create_environment(platform_file);

  /*Parse DAG file */
  dag = parseDAGFile(argv[2]); 


  allocateNodeAttributes(dag);
  allocateHostAttributes();

  /* create Simgrid objects */
  createSimgridObjects();
  
  HEFT(dag);
  
  /* not recommanded with big DAGs! */
  /* printDAG(dag); */

  changed_tasks = SD_simulate(-1.0);
  INFO0("Tasks whose state has changed:");
  i = 0;
  while(changed_tasks[i] != NULL) {
    switch (SD_task_get_state(changed_tasks[i])) {
    case SD_SCHEDULED:
      INFO1("%s is scheduled.", SD_task_get_name(changed_tasks[i]));
      break;
    case SD_READY:
      INFO1("%s is ready.", SD_task_get_name(changed_tasks[i]));
      break;
    case SD_RUNNING:
      INFO1("%s is running.", SD_task_get_name(changed_tasks[i]));
      break;
    case SD_DONE:
      INFO1("%s is done.", SD_task_get_name(changed_tasks[i]));
      break;
    case SD_FAILED:
      INFO1("%s is failed.", SD_task_get_name(changed_tasks[i]));
      break;
    default:
      INFO1("Unknown status for %s", SD_task_get_name(changed_tasks[i]));
      break;
      }
    i++;
  }
  free(changed_tasks);
  INFO1("Total: %d", i);

  /* clear some memory */
  freeNodeAttributes(dag);
  freeHostAttributes();
  freeSimgridTasks();
  freeDAG(dag);
  
  /* reset SimDag */
  SD_exit();

  return 0;
}

/*
 * createSimgridObjects()
 */
int createSimgridObjects()
{
  /* Create resources */
/*   if (createSimgridResources() == -1) */
/*     return -1; */

  /* Create tasks */
  if (createSimgridTasks() == -1)
    return -1;

  return 0;
}

/*
 * createSimgridResources()
 */
/* static int createSimgridResources() */
/* { */
/*   int i,j; */
/*   char buffer[32]; */
/*   SG_Resource *sg_TCPlinks; */
/*   SG_Resource fast_link; */
  
/*   /\* The almost infinetely fast TCP link *\/  */
/*   fast_link = SG_newTCPLink("infinetly_fast_TCPlink",  */
/* 			    NULL, */
/* 			    0.0, EPSILON, NULL,  */
/* 			    0.0, 1000.0, */
/* 			    SG_TCP_SHARED,NULL); */

/*   sg_TCPlinks = (SG_Resource*) calloc (2, sizeof(SG_Resource)); */
/*   sg_TCPlinks[0] = fast_link; */
/*   sg_TCPlinks[1] = NULL; */

/*   /\* And the almost infinetely fast TCP route *\/  */
/*   local_link = newLink(); */
/*   local_link->sg_link =  */
/*     SG_newTCPRoute ("infinitely_fast_route", sg_TCPlinks, NULL);  */
/*   free(sg_TCPlinks); */

/*   /\* Create hosts *\/ */
/*   for (i=0;i<grid->nb_hosts;i++) { */
/*     if (createSimgridHost(grid->hosts[i]) == -1) */
/*       return -1; */
/*   } */
  
/*   /\* Create TCP links *\/ */
/*   for (i=0;i<grid->nb_links;i++) { */
/*     if (createSimgridLink(grid->links[i]) == -1) */
/*       return -1; */
/*   } */

/*   /\* Create TCP routes *\/ */
/*   grid->routes=(SG_Resource**)calloc(grid->nb_hosts,sizeof(SG_Resource*)); */

/*   for (i=0;i<grid->nb_hosts;i++) { */
/*     grid->routes[i]=(SG_Resource *)calloc(grid->nb_hosts,sizeof(SG_Resource)); */
/*     for (j=0;j<grid->nb_hosts;j++) { */
/*       sprintf(buffer,"route#%d-%d",i,j); */
/*       if (i!=j) { */
/* 	if (grid->hosts[i]->cluster == grid->hosts[j]->cluster) { */
/* 	  /\* Intra cluster route *\/ */
/* 	  /\* src - switch - dest*\/ */
/* 	  sg_TCPlinks = (SG_Resource*) calloc (4, sizeof(SG_Resource)); */
/* 	  sg_TCPlinks[0] = grid->connections[i][j][0]->sg_link; */
/* 	  sg_TCPlinks[1] = grid->connections[i][j][1]->sg_link; */
/* 	  sg_TCPlinks[2] = grid->connections[i][j][2]->sg_link; */
/* 	  sg_TCPlinks[3] = NULL; */
/* 	} else { */
/* 	  /\* Inter cluster route *\/ */
/* 	  /\* src - switch - gateway - backbone - gateway - switch - dest*\/ */
/* 	  sg_TCPlinks = (SG_Resource*) calloc (8, sizeof(SG_Resource)); */
/* 	  sg_TCPlinks[0] = grid->connections[i][j][0]->sg_link; */
/* 	  sg_TCPlinks[1] = grid->connections[i][j][1]->sg_link; */
/* 	  sg_TCPlinks[2] = grid->connections[i][j][2]->sg_link; */
/* 	  sg_TCPlinks[3] = grid->connections[i][j][3]->sg_link; */
/* 	  sg_TCPlinks[4] = grid->connections[i][j][4]->sg_link; */
/* 	  sg_TCPlinks[5] = grid->connections[i][j][5]->sg_link; */
/* 	  sg_TCPlinks[6] = grid->connections[i][j][6]->sg_link; */
/* 	  sg_TCPlinks[7] = NULL; */
/* 	} */
/* 	grid->routes[i][j] = SG_newTCPRoute (buffer, sg_TCPlinks, NULL);  */
/* 	free(sg_TCPlinks); */
/*       } else { */
/* 	/\*Self communication => no route *\/ */
/* 	grid->routes[i][j] = NULL; */
/*       } */
/*     } */
/*   } */
/*   return 0; */
/* } */

/* static int createSimgridHost(Host h) */
/* { */
/*   char buffer[32]; */
/*   char *filename; */
/*   double offset; */
/*   double value; */

/*   sprintf(buffer,"host#%d",h->global_index); */

/*   if (parseTraceSpec(h->cpu_trace,&filename,&offset,&value) == -1) { */
/*     fprintf(stderr,"Syntax error: host#%d\n",h->global_index); */
/*     return -1; */
/*   } */
	  
/*   h->sg_host = SG_newHost(buffer,h->rel_speed,  */
/* 			  h->mode, filename,offset,value, */
/* 			  NULL, NULL, 0, h); */
/*   free(filename); */

/*   return 0; */
/* } */

/* /\* */
/*  * createSimgridLink() */
/*  * */
/*  *\/ */
/* static int createSimgridLink(Link l) */
/* { */
/*   char buffer[32]; */
/*   char *filename1,*filename2; */
/*   double offset1,offset2; */
/*   double value1,value2; */

/*   sprintf(buffer,"link#%d",l->global_index); */

/*   if ((parseTraceSpec(l->latency_trace,&filename1,&offset1,&value1) == -1) || */
/*       (parseTraceSpec(l->bandwidth_trace,&filename2,&offset2,&value2) == -1)) { */
/*     fprintf(stderr,"Syntax error: link#%d\n",l->global_index); */
/*     return -1; */
/*   } */


/*   if (l->mode == SG_TIME_SLICED) { */
/*     l->sg_link=SG_newTCPLink (buffer, */
/* 			      filename1, offset1, value1, */
/* 			      filename2, offset2, value2, */
/* 			      SG_TCP_SHARED,NULL); */
/*   } */

/*   if (l->mode == SG_FAT_PIPE) { */
/*     l->sg_link=SG_newTCPLink (buffer, */
/* 			      filename1, offset1, value1, */
/* 			      filename2, offset2, value2, */
/* 			      SG_TCP_BACKBONE,NULL); */
/*   } */

/*   free(filename1); */
/*   free(filename2); */

/*   return 0; */
/* } */

/*
 * createSimgridTasks()
 *
 */
static int createSimgridTasks()
{
  Node node;
  char buffer[32];
  int i,j;

  /* Creating the tasks */
  for (i=0;i<dag->nb_nodes;i++) {
    node = dag->nodes[i];
    sprintf(buffer,"node#%d",node->global_index);
    node->sd_task = SD_task_create(buffer, node, 1.0);
  }

  /* Set the dependencies */
  for (i=0;i<dag->nb_nodes;i++) {
    node = dag->nodes[i];
    for (j=0;j<node->nb_parents;j++) {
      SD_task_dependency_add(NULL, NULL, node->parents[j]->sd_task, node->sd_task);
    }
  }
  return 0;
}


/*
 * freeSimgridTasks()
 *
 */
static void freeSimgridTasks()
{
  int i;
  for (i=0;i<dag->nb_nodes;i++) {
    SD_task_destroy(dag->nodes[i]->sd_task);
  }
}

/*
 * parseTraceSpec()
 */
/* static int parseTraceSpec(char* spec,char**filename, */
/* 		double *offset, double *value) */
/* { */
/*   char *tmp; */

/*   tmp = strchr(spec,':'); */
/*   if (!tmp) { */
/*     fprintf(stderr,"Parse error: missing ':' in trace specification\n"); */
/*     return -1; */
/*   } */

/*   *filename=NULL; */
/*   *offset=0.0; */
/*   *value=0.0; */
  
/*   if (tmp == spec) { /\* FIXED *\/ */
/*     if (!*(tmp+1)) { */
/*       fprintf(stderr,"Parse error: invalid value specification\n"); */
/*       return -1; */
/*     } */
/*     *value = atof(tmp+1); */
/*     if (*value < 0.0) { */
/*       fprintf(stderr,"Error: invalid value\n"); */
/*       return -1; */
/*     } */
/*     return 0; */
/*   } */

/*   /\* TRACE *\/ */
/*   *tmp='\0'; */
/*   *filename=strdup(spec); */
/*   if (!*(tmp+1)) { */
/*     fprintf(stderr,"Parse error: invalid offset specification\n"); */
/*     return -1; */
/*   } */
/*   *offset = atof(tmp+1); */
/*   if (*offset < 0.0) { */
/*     fprintf(stderr,"Error: invalid offset\n"); */
/*     return -1; */
/*   } */

/*   return 0; */
/* } */
