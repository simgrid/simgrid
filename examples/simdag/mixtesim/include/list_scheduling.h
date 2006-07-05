#ifndef LIST_SCHEDULING_H
#define LIST_SCHEDULING_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void allocateNodeAttributes(DAG dag);
void freeNodeAttributes(DAG dag);
void allocateHostAttributes();
void freeHostAttributes();
void implementSimgridSchedule(DAG dag, Node *list);

/* Link local_link; */

#define LIST_SCHEDULING_NOT_SCHEDULED 0
#define LIST_SCHEDULING_READY         1
#define LIST_SCHEDULING_SCHEDULED     2
#define LIST_SCHEDULING_RUNNING       3



/**************/
/* Attributes */
/**************/

typedef struct _NodeAttribute *NodeAttribute;
typedef struct _HostAttribute *HostAttribute;

struct _NodeAttribute {
  SD_workstation_t   host;   /* The host on which the node has been scheduled */
  SD_link_t   link;   /* The link on which the transfer is running */
  double start;  /* time the task is supposed to start            */
  double finish; /* time the task is supposed to finish           */
  int state;     /* scheduled, not scheduled, ready               */
  double SL;
  int tag;

  double mean_cost; /* used by heft */
  double rank_u;    /* used by heft */
  
  double data_available;
};

struct _HostAttribute {
  double first_available;
  int nb_nodes;
  int *nodes;
  int state;
};

#define NODE_ATTR(x) ((NodeAttribute)(x->metadata))
#define HOST_ATTR(x) ((HostAttribute)(SD_workstation_get_data(x)))

#endif
