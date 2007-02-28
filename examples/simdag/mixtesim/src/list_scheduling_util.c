#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mixtesim_prototypes.h"
#include "list_scheduling.h"

#include "simdag/simdag.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mixtesim_list_scheduling, mixtesim,
				"Logging specific to Mixtesim (list scheduling)");

/*
 * freeHostAttributes()
 */
void freeHostAttributes()
{
  int i;
  int nb_hosts = SD_workstation_get_number();
  const SD_workstation_t *hosts = SD_workstation_get_list();

  for (i=0;i<nb_hosts;i++) {
    free(SD_workstation_get_data(hosts[i]));
    SD_workstation_set_data(hosts[i], NULL);
  }
  return;
}

/*
 * allocateHostAttributes()
 */
void allocateHostAttributes()
{
  int i;
  int nb_hosts = SD_workstation_get_number();
  const SD_workstation_t *hosts = SD_workstation_get_list();
  void *data;

  for (i=0;i<nb_hosts;i++) {
    data = calloc(1,sizeof(struct _HostAttribute));
    SD_workstation_set_data(hosts[i], data);
  }
  return;
}

/*
 * freeNodeAttributes()
 */
void freeNodeAttributes(DAG dag)
{
  int i;

  for (i=0;i<dag->nb_nodes;i++) {
    if (dag->nodes[i]->metadata)  {
      free(dag->nodes[i]->metadata);
    }
  }
  return;
}

/*
 * allocateNodeAttributes()
 */
void allocateNodeAttributes(DAG dag)
{
  int i;

  for (i=0;i<dag->nb_nodes;i++) {
    dag->nodes[i]->metadata = 
	    (NodeAttribute)calloc(1,sizeof(struct _NodeAttribute));
    if (dag->nodes[i]->type == NODE_COMPUTATION)
      NODE_ATTR(dag->nodes[i])->state = LIST_SCHEDULING_NOT_SCHEDULED;
  }
  return;
}

/*
 * implementSimgridSchedule()
 */
void implementSimgridSchedule(DAG dag, Node *list)
{
  int j,k;
  Node node;
  Node grand_parent;
  SD_workstation_t grand_parent_host;
  SD_workstation_t host;
/*   Link link; */
/*  SD_link_t *route;*/

  const double no_cost[] = {0.0, 0.0};
  const double small_cost = 1.0; /* doesn't work with 0.0 */
  e_SD_task_state_t state;
  SD_workstation_t host_list[] = {NULL, NULL};
  double communication_amount[] = {0.0, 0.0,
				   0.0, 0.0};

  /* Schedule Root */
  if (SD_task_get_state(dag->root->sd_task) == SD_NOT_SCHEDULED) {
    DEBUG1("Dag root cost = %f", dag->root->cost);
    DEBUG1("Scheduling root task '%s'", SD_task_get_name(dag->root->sd_task));
    SD_task_schedule(dag->root->sd_task, 1, SD_workstation_get_list(),
		     &small_cost, no_cost, -1.0);
    DEBUG2("Task '%s' state: %d", SD_task_get_name(dag->root->sd_task), SD_task_get_state(dag->root->sd_task));
  }

  /* Schedule the computation */
  for (j=0;list[j];j++) {
    node=list[j];

    /* schedule the task */
    DEBUG1("Scheduling computation task '%s'", SD_task_get_name(node->sd_task));
    SD_task_schedule(node->sd_task, 1, &NODE_ATTR(node)->host,
		     &node->cost, no_cost, -1.0);
    DEBUG2("Task '%s' state: %d", SD_task_get_name(node->sd_task), SD_task_get_state(node->sd_task));

    /* schedule the parent transfer */
    for (k=0;k<node->nb_parents;k++) {
      if (node->parents[k]->type == NODE_ROOT)
        break;
      /* no transfer */
      if (node->parents[k]->type == NODE_COMPUTATION)
        continue;
      /* normal case */
      if (node->parents[k]->type == NODE_TRANSFER) {
	state = SD_task_get_state(node->parents[k]->sd_task);
	if (state != SD_RUNNING && state != SD_DONE) {
          grand_parent=node->parents[k]->parents[0];
          grand_parent_host=NODE_ATTR(grand_parent)->host;
	  host=NODE_ATTR(node)->host;
	  
	  if (host != grand_parent_host) {
	    host_list[0] = grand_parent_host;
	    host_list[1] = host;
	    communication_amount[1] = node->parents[k]->cost;
	    DEBUG1("Scheduling transfer task '%s'",
		   SD_task_get_name(node->parents[k]->sd_task));
	    SD_task_schedule(node->parents[k]->sd_task, 2, host_list,
			     no_cost, communication_amount, -1.0);
	    DEBUG2("Task '%s' state: %d",
		   SD_task_get_name(node->parents[k]->sd_task),
		   SD_task_get_state(node->parents[k]->sd_task));
	  } else {
	    SD_task_schedule(node->parents[k]->sd_task, 1, host_list,
			     no_cost, communication_amount, -1.0);
/* 	    SD_task_schedule(node->parents[k]->sd_task, 1, &host, */
/* 			     no_cost, &(node->parents[k]->cost), -1.0); */
	  }
/* 	  host_list[0] = grand_parent_host; */
/* 	  host_list[1] = host; */
/* 	  communication_amount[1] = node->parents[k]->cost; */
/* 	  DEBUG1("Scheduling transfer task '%s'", SD_task_get_name(node->parents[k]->sd_task)); */
/* 	  SD_task_schedule(node->parents[k]->sd_task, 2, host_list, */
/* 			   no_cost, communication_amount, -1.0); */
/* 	  DEBUG2("Task '%s' state: %d", SD_task_get_name(node->parents[k]->sd_task), */
/* 		 SD_task_get_state(node->parents[k]->sd_task)); */

/* /\*           if (!route) { *\/ */
/* /\*             if (SG_scheduleTaskOnResource(node->parents[k]->sg_task, *\/ */
/* /\*  					  local_link->sg_link) *\/ */
/* /\* 		== -1) { *\/ */
/* /\* 	      fprintf(stderr,"Task '%s' already in state %d\n", *\/ */
/* /\* 		      node->parents[k]->sg_task->name, *\/ */
/* /\* 		      node->parents[k]->sg_task->state); *\/ */
/* /\* 	    } *\/ */
/* /\* 	  } else { *\/ */
/* /\*             if (SG_scheduleTaskOnResource(node->parents[k]->sg_task, *\/ */
/* /\* 					  route)  *\/ */
/* /\* 		== -1) { *\/ */
/* /\* 	     fprintf(stderr,"Task '%s' already in state %d\n", *\/ */
/* /\* 		      node->parents[k]->sg_task->name, *\/ */
/* /\* 		      node->parents[k]->sg_task->state); *\/ */
/* /\* 	    }  *\/ */
/* /\* 	  } *\/ */
	}
      }
    }
  }

  /* Schedule End */
/*   SG_scheduleTaskOnResource(dag->end->sg_task,local_link->sg_link); */
  SD_task_schedule(dag->end->sd_task, 1, SD_workstation_get_list(),
		   &small_cost, no_cost, -1.0);


  return;
}


