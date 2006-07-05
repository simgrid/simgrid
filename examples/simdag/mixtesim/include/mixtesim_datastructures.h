#ifndef MIXTESIM_DATASTRUCTURES_H
#define MIXTESIM_DATASTRUCTURES_H

#include "simdag/simdag.h"
#include "mixtesim_types.h"

/* NODE structure */
struct _Node {
  /* nodes that must complete before this node can start */
  int nb_parents;
  Node *parents; /* array of size nb_parents */

  /* nodes that cannot start until this node has completed */
  int nb_children;
  Node *children; /* array of size nb_children */

  /* index of this node in the DAG->nodes array */
  int global_index;

  node_t type;       /* node type: TRANSFER or COMMUNICATION */

  double cost;       /* cost of the computation        */
	      	     /* or file transfer on a resource */
  
  /* Pointer to the SIMGRID Task that will
   * be used for the simulation. The abstract task should
   * be created upon creation of this node, and will be
   * de-allocated when SG_shutdown() is called
   */
  SD_task_t sd_task;

  void *metadata; /* For use by the scheduling algorithms */
};

Node newNode();  /* Allocates memory for a node */
void freeNode(); /* Free memory for a node */
void printNode(DAG dag, Node node); /* print node info */

/* DAG structure */
struct _DAG {
  Node root;    /* first node */
  Node end;     /* last node */

  /* Array of nodes, for direct access */
  int nb_nodes;
  Node *nodes; /* array of size nb_nodes */
};

DAG parseDAGFile(char *filename); /* Construct a DAG from a */
				  /* DAG description file   */
DAG newDAG(); /* allocate memory for a DAG, called by parseDAGFile() */
void freeDAG(DAG dag); /* free memory for a DAG */
void printDAG(DAG dag); /* prints DAG info */

/* /\* HOST structure *\/ */
/* struct _Host { */
/*   /\* index in the Grid->hosts array *\/ */
/*   int global_index; */

/*    /\* in which cluster this host is *\/ */
/*   int cluster; */

/*   /\* relative speed of the host *\/ */
/*   double rel_speed; */
/*   /\* Pointer to the SIMGRID Resource that will be used */
/*    * for the simulation. The resource should be created upon  */
/*    * creation of this host, and will be de-allocated when */
/*    * SG_shutdown() is called */
/*    *\/ */
/*   /\*  char *cpu_trace;*\/ */

/*   void *metadata; */

/*   /\*  SG_resourcesharing_t mode;*\/ */

/*   SD_workstation_t sd_host; */
/* }; */

/* Host newHost(); /\* Allocate memory for a host *\/ */
/* void freeHost(Host h); /\* free memory for a host *\/ */


/* /\* LINK structure *\/ */
/* struct _Link { */
/*   /\* index of this link in the Grid->links array *\/ */
/*   int global_index; */
/*   /\* Pointer to the SIMGRID Resource that will be used */
/*    * for the simulation. The resource shouild be created upon  */
/*    * creation of this host, and will be de-allocated when */
/*    * SG_shutdown() is called */
/*    *\/ */
/*   /\*  char *latency_trace; */
/*       char *bandwidth_trace;*\/ */

/*   /\*  SG_resourcesharing_t mode;*\/ */

/*   SD_link_t sd_link; */
/* }; */

/* Link newLink(); /\* Allocate memory for a link *\/ */
/* void freeLink(Link l); /\* free memory for a link *\/ */

/* GRID structure */
/* struct _Grid { */
/*   int nb_hosts; /\* Number of hosts in the Grid *\/ */
/*   Host *hosts;  /\* array of size nb_hosts      *\/ */
/*   int nb_links; /\* Number of links in the Grid *\/ */
/*   Link *links;  /\* array of size nb_links      *\/ */

/*   /\* connection matrix. connections[i][j] is a pointer */
/*    * to the network link that connects hosts of */
/*    * global_index i and j, that is grid->hosts[i] */
/*    * and grid->hosts[j]. This matrix is likely to */
/*    * be symetric since we probably assume that links */
/*    * behave the same way in both directions. Note that */
/*    * simulating non-bi-directional performance characteristics */
/*    * would not be trivial as two separate links would not */
/*    * model any contention between traffic going both ways */
/*    *\/ */
/*   /\*  SG_Resource **routes;*\/ */
/*   /\*  SD_link_t **connections;*\/ */
/* }; */

/* Grid parseGridFile(char *filename); /\* Creates a Grid from a *\/ */
/*                                     /\* Grid description file *\/ */
/* Grid newGrid();  /\* Allocates memory for a Grid *\/ */
/* void freeGrid(); /\* frees memory of a Grid. free Hosts and Links *\/ */
/* void printGrid(Grid grid); /\* print Grid info *\/ */


#endif /* MIXTESIM_DATA_STRUCTURES */
