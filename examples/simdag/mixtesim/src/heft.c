#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mixtesim_prototypes.h"
#include "list_scheduling.h"

#include "simdag/simdag.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mixtesim_heft, mixtesim,
				"Logging specific to Mixtesim (heft)");

/* static function prototypes */

static Node *computeNodeList(DAG dag);
static int rankUCompareNode(const void *n1, const void *n2);
static int scheduleList(DAG dag, Node *L);
static void scheduleNode(Node node);

static double computeAverageLinkSpeed();
static double computeAverageLinkLatency();
static double computeAverageExecutionTime(Node node);

static void assignMeanCost(Node node);
static double computeAndAssignRankU(Node node);


static double computeEarliestStartTime(Node node, SD_workstation_t host);
static double computeEarliestFinishTime(Node node, SD_workstation_t host);
static void scheduleNodeOnHost(Node node, SD_workstation_t host);


/*
 * HEFT():  Implementation of HEFT
 */
int HEFT(DAG dag)
{
  Node *L=NULL;

  /* Compute mean cost for each node */
  assignMeanCost(dag->root);
  
  /* Compute Rank_u for each node */
  computeAndAssignRankU(dag->root);

  /* Compute the list for the schedule */
  if ((L = computeNodeList(dag)) == NULL)
    return -1;

  /* Schedule */
  if (scheduleList(dag,L) == -1)
    return -1;

  /* Analyze */
/*   if (analyzeSchedule (grid,dag,L) == -1) */
/*     return -1; */

  /* Implement the Simgrid schedule */
  implementSimgridSchedule(dag,L);

  if (L)
    free (L);

  return 0;
}

static Node *computeNodeList(DAG dag)
{
  int i;
  int nb_comp_nodes=0;
  Node *L=NULL;
  
  for (i=0;i<dag->nb_nodes;i++) {
    if (dag->nodes[i]->type != NODE_COMPUTATION)
      continue;
    L = (Node *)realloc(L,(nb_comp_nodes+1)*sizeof(Node));
    L[nb_comp_nodes] = dag->nodes[i];
    nb_comp_nodes++;
  }

  /* sort L by rank_u */
  qsort(L,nb_comp_nodes,sizeof(Node),rankUCompareNode);

  /* NULL-terminate the list */
  L = (Node *)realloc(L,(nb_comp_nodes+1)*sizeof(Node));
  L[nb_comp_nodes]=NULL;
  
  return L;
}

/*
 * rankUCompareNode()
 */
static int rankUCompareNode(const void *n1, const void *n2)
{
  double rank_u1, rank_u2;

  rank_u1 = NODE_ATTR((*((Node *)n1)))->rank_u;
  rank_u2 = NODE_ATTR((*((Node *)n2)))->rank_u;

  if (rank_u1 > rank_u2)
    return -1;
  else if (rank_u1 == rank_u2)
    return 0;
  else
    return 1;
}

/*
 * scheduleList()
 */
static int scheduleList(DAG dag, Node *L)
{
  int i=0;
  Node node;

  /* Schedules the nodes in the order of list L */
  while ((node=L[i++])) {
    scheduleNode(node);
  }
  return 0;
}

/*
 * scheduleNode()
 *   pick a host
 */
static void scheduleNode(Node node)
{
  int i;
  SD_workstation_t host;
  SD_workstation_t best_host;
  double EFT;
  double min_EFT=-1.0;
  int nb_hosts = SD_workstation_get_number();
  const SD_workstation_t *hosts = SD_workstation_get_list();

  /* pick the best host */
  for (i=0;i<nb_hosts;i++) {
    host = hosts[i];
    /* compute the start_time on that host */
    EFT = computeEarliestFinishTime(node, host);
/*     fprintf(stderr,"EFT %s on %s = %f\n",node->sg_task->name, */
/* 	    host->sg_host->name, EFT); */

    /* is it the best one ? */
    if ((min_EFT == -1.0)||
        (EFT < min_EFT)) {
      min_EFT = EFT;
      best_host = host;
    }
  }
  
  scheduleNodeOnHost(node, best_host);

  return;
}

/*
 * computeAverageLinkSpeed()
 */
static double computeAverageLinkSpeed()
{
  int i;
  double bandwidth, ave;
  int nb_links = SD_link_get_number();
  const SD_link_t *links = SD_link_get_list();

  ave=0.0;
  for (i=0;i<nb_links;i++) {
    bandwidth = SD_link_get_current_bandwidth(links[i]);
    ave += bandwidth;
  }
  return (ave / nb_links);
}

/*
 * computeAverageLinkLatency()
 */
static double computeAverageLinkLatency()
{
  int i;
  double latency, ave;
  int nb_links = SD_link_get_number();
  const SD_link_t *links = SD_link_get_list();

  ave=0.0;
  for (i=0;i<nb_links;i++) {
    latency = SD_link_get_current_latency(links[i]);
    ave += latency;
  }
  return (ave / nb_links);
}

/*
 *computeAverageExecutionTime()
 */
static double computeAverageExecutionTime(Node node)
{
  int i;
  double exec_time, ave;
  int nb_hosts = SD_workstation_get_number();
  const SD_workstation_t *hosts = SD_workstation_get_list();
  double communication_amount = 0.0;

  ave=0.0;
  for (i=0;i<nb_hosts;i++) {
    exec_time = 
      SD_task_get_execution_time(node->sd_task, 1, &hosts[i],
				 &node->cost, &communication_amount, -1.0);

      ave += exec_time;
    }

  return (ave / nb_hosts);
}

/*
 * computeAverageCommunicationTime()
 */
static double computeAverageCommunicationTime(Node node)
{
  return (computeAverageLinkLatency() + 
	  (node->cost / computeAverageLinkSpeed()));  
}
 
/*
 * computeEarliestFinishTime()
 */
static double computeEarliestFinishTime(Node node, SD_workstation_t host)
{
  double executionTime, EST;
  double communication_amount = 0.0;
  
  EST = computeEarliestStartTime(node, host);
  executionTime =
    SD_task_get_execution_time(node->sd_task, 1, &host, &node->cost,
			       &communication_amount, -1.0);

  return executionTime + EST;
}

/*
 * computeEarliestStartTime()
 */
static double computeEarliestStartTime(Node node, SD_workstation_t host)
{  
  double earliest_start_time=0.0;
  int i;
  double host_available = HOST_ATTR(host)->first_available;
  double data_available;
  double transfer_time;
  double last_data_available;
  Node parent,grand_parent;
  SD_workstation_t grand_parent_host;
/*   SD_link_t link; */

  if (node->nb_parents != 0) {
    /* compute last_data_available */
    last_data_available=-1.0;
    for (i=0;i<node->nb_parents;i++) {
      parent = node->parents[i];

      /* No transfers to schedule */
      if (parent->type == NODE_ROOT) {
	data_available=0.0;
	break;
      }
      /* normal case */
      if (parent->type == NODE_TRANSFER) {
	if (parent->nb_parents > 1) {
	  fprintf(stderr,"Warning: transfer has 2 parents\n");
	}
	grand_parent = parent->parents[0];
	grand_parent_host = NODE_ATTR(grand_parent)->host;
	transfer_time =
	  SD_route_get_communication_time(grand_parent_host, host, parent->cost);

/* 	link = SD_workstation_route_get_list(grand_parent_host, host)[0]; */
	
/* 	if (link->global_index == -1) { */
/* 	  /\* it is the local link so transfer_time = 0 *\/ */
/* 	  transfer_time = 0.0; */
/* 	} else { */
/* 	  transfer_time = SG_getLinkLatency(link->sg_link,  */
/* 					    NODE_ATTR(grand_parent)->finish) +  */
/* 	    parent->cost/SG_getLinkBandwidth(link->sg_link,  */
/* 					     NODE_ATTR(grand_parent)->finish); */
/* 	} */
	data_available = NODE_ATTR(grand_parent)->finish + transfer_time;
      }
  
      /* no transfer */
      if (parent->type == NODE_COMPUTATION) {
	data_available = NODE_ATTR(parent)->finish;
      }
      
      if (last_data_available < data_available)
	last_data_available = data_available;
    }
    
    /* return the MAX "*/
    earliest_start_time = MAX(host_available,last_data_available);
    
  }
  return earliest_start_time;
}


/*
 * computeAndAssignRankU()
 */
static double computeAndAssignRankU(Node node)
{
  int i;
  double my_rank_u = 0.0, max_rank_u, current_child_rank_u;
  Node child,grand_child;

  my_rank_u = NODE_ATTR(node)->mean_cost;
  max_rank_u = -1.0;
  
  for (i=0;i<node->nb_children;i++) {
    child = node->children[i];
    
    if (node->type == NODE_ROOT)
      {
	my_rank_u = computeAndAssignRankU(child);
      }

    if (child->type == NODE_TRANSFER) { /* normal case */
      if (child->nb_children > 1) {
	fprintf(stderr,"Warning: transfer has 2 children\n");
      }
      grand_child = child->children[0];
      current_child_rank_u = NODE_ATTR(child)->mean_cost + 
	computeAndAssignRankU(grand_child);
      if (max_rank_u < current_child_rank_u)
	max_rank_u = current_child_rank_u;
    }
    else /* child is the end node */
      max_rank_u = 0.0;
      
  }
  
  my_rank_u += max_rank_u;
  
  NODE_ATTR(node)->rank_u = my_rank_u;
  return my_rank_u ;
}

/*
 * assignMeanCost()
 */
static void assignMeanCost(Node node)
{
  int i;
  
  for (i=0;i<node->nb_children;i++) {
    if (node->children[i]->type == NODE_COMPUTATION)
      NODE_ATTR(node->children[i])->mean_cost =
	computeAverageExecutionTime(node->children[i]);
    else
      NODE_ATTR(node->children[i])->mean_cost =
	computeAverageCommunicationTime(node->children[i]);
    assignMeanCost(node->children[i]);
  }
  return;
}


/*
 * scheduleNodeOnHost()
 */
static void scheduleNodeOnHost(Node node, SD_workstation_t host)
{
  int i;
  double data_available, transfer_time,  last_data_available;
/*   SD_link_t link; */
  Node parent, grand_parent;
  SD_workstation_t grand_parent_host;
  double communication_amount = 0.0;

  INFO2("Affecting node '%s' to host '%s'", SD_task_get_name(node->sd_task),
	 SD_workstation_get_name(host));

  last_data_available=-1.0;
  for (i=0;i<node->nb_parents;i++) {
    parent = node->parents[i];

    if (parent->type == NODE_ROOT) {/* No transfers to schedule */
      data_available=0.0;
      break;
    }
  
    if (parent->type == NODE_TRANSFER) {  /* normal case */
      if (parent->nb_parents > 1) {
        fprintf(stderr,"Warning: transfer has 2 parents\n");
      }
      grand_parent = parent->parents[0];
      grand_parent_host = NODE_ATTR(grand_parent)->host;
      transfer_time =
	SD_route_get_communication_time(grand_parent_host, host, parent->cost);

/*       link = SD_workstation_route_get_list(grand_parent_host, host)[0]; */

/*       if (link->global_index == -1) { */
/* 	/\* it is the local link so transfer_time = 0 *\/ */
/* 	transfer_time = 0.0; */
/*       } else { */
/* 	transfer_time = SG_getLinkLatency(link->sg_link,  */
/* 					  NODE_ATTR(grand_parent)->finish) +  */
/* 	  parent->cost/SG_getLinkBandwidth(link->sg_link,  */
/* 					   NODE_ATTR(grand_parent)->finish); */
/*       } */
     
      data_available = NODE_ATTR(grand_parent)->finish + transfer_time;
    }
  
    /* no transfer */
    if (parent->type == NODE_COMPUTATION) {
      data_available = NODE_ATTR(parent)->finish;
    }

    if (last_data_available < data_available)
      last_data_available = data_available;
  }

  /* node attributes */
  NODE_ATTR(node)->start=MAX(last_data_available,
		  HOST_ATTR(host)->first_available);
  NODE_ATTR(node)->finish=NODE_ATTR(node)->start + 
    SD_task_get_execution_time(node->sd_task, 1, &host, &node->cost,
			       &communication_amount, -1.0);
/*     SG_getPrediction(SG_PERFECT, 0.0, NULL,host->sg_host, */
/* 		     NODE_ATTR(node)->start,node->cost); */

  NODE_ATTR(node)->host = host;
  NODE_ATTR(node)->state = LIST_SCHEDULING_SCHEDULED;

  /* host attributes */
  HOST_ATTR(host)->first_available = NODE_ATTR(node)->finish;

  return;
}
