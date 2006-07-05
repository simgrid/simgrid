#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mixtesim_prototypes.h"

/**           **
 ** STRUCTURE **
 **           **/

/*
 * newDAG()
 */
DAG newDAG()
{
  return (DAG)calloc(1,sizeof(struct _DAG));  
}

/*
 * freeDAG()
 */
void freeDAG(DAG dag)
{
  int i;

  for (i=0;i<dag->nb_nodes;i++) {
    freeNode(dag->nodes[i]);
  }
  if (dag->nodes)
    free(dag->nodes);
  free(dag);
  return;
}

/*
 * newNode()
 */
Node newNode()
{
  return (Node)calloc(1,sizeof(struct _Node));
}

/*
 * freeNode()
 */
void freeNode(Node n)
{
  if (n->parents)
    free(n->parents);
  if (n->children)
    free(n->children);
  free(n);
  return;
}

/*
 * printDAG()
 */
void printDAG(DAG dag)
{
  fprintf(stderr,"%d nodes: # (children) [parents]\n",dag->nb_nodes);
  printNode(dag,dag->root);
}

/*
 * printNode()
 */
void printNode(DAG dag, Node node)
{
  int i;
  fprintf(stderr,"%d (",node->global_index);

  /* print children info */
  for (i=0;i<node->nb_children-1;i++) {
    fprintf(stderr,"%d,",node->children[i]->global_index);
  }
  if (node->nb_children> 0)
    fprintf(stderr,"%d",node->children[node->nb_children-1]->global_index);
  fprintf(stderr,") ");

  /* print parent info */
  fprintf(stderr,"[");
  for (i=0;i<node->nb_parents-1;i++) {
    fprintf(stderr,"%d,",node->parents[i]->global_index);
  }
  if (node->nb_parents > 0)
    fprintf(stderr,"%d",node->parents[node->nb_parents-1]->global_index);
  fprintf(stderr,"] ");

  /* print special inf */
  if (dag->root == node) {
    fprintf(stderr,"ROOT\n");
  } else if (dag->end == node) {
    fprintf(stderr,"END\n");
  } else {
    fprintf(stderr,"\n");
  }

  for (i=0;i<node->nb_children;i++) {
    printNode(dag, node->children[i]);
  }
  return;
}

/**         **
 ** PARSING **
 **         **/

static int parseLine(DAG,char*);
static int parseNodeLine(DAG,char*);
static int parseNodeCountLine(DAG,char*);
static node_t string2NodeType(const char*);
static int string2NodeList(const char*,int**,int*);
static int finalizeDAG(DAG);

/* temporary array in which children lists are stored. Once all nodes
 * have been parsed, one can go through these lists and assign actual
 * Node pointers to real children lists
 */
char **tmp_childrens;

/*
 * parseDAGFile()
 */
DAG parseDAGFile(char *filename)
{
  FILE *f;
  char line[4024];
  int i,count=0;
  char *tmp;
  DAG new;

  /* initialize the global children list array */
  tmp_childrens=NULL;

  /* opening the DAG description file */
  if (!(f=fopen(filename,"r"))) {
    fprintf(stderr,"Impossible to open file '%s'\n",filename);
    return NULL;
  }

  /* allocate memory for the DAG */
  new = newDAG();

  /* Go through the lines of the file */
  while(fgets(line,4024,f)) {
    count++;
    if (line[0]=='#') /* comment lines */
      continue;
    if (line[0]=='\n') /* empty lines */
      continue;
    tmp=strdup(line); /* somehow, strduping makes it work ? */
    if (parseLine(new,tmp) == -1) {
      fprintf(stderr,"Error in DAG description file at line %d\n",count);
      fclose(f);
      return NULL;
    } 
    free(tmp);
  }
  fclose(f);

  /* fill in child lists, parent lists, and perform a few
   * reality checks
   */
  if (finalizeDAG(new) == -1) {
    freeDAG(new);
    return NULL;
  }

  /* free the temporary global child list array */
  for (i=0;i<new->nb_nodes;i++) {
    if (tmp_childrens[i])
      free(tmp_childrens[i]);
  }
  free(tmp_childrens);

  return new;
}

/*
 * finalizeDAG(): fills in the child lists with actual pointers,
 * creates the parent lists, checks some things about the Root
 * and the End node.
 */
static int finalizeDAG(DAG dag)
{
  int i,j;
  int *childrenlist,nb_children;

  /* Set the children pointers */
  for (i=0;i<dag->nb_nodes;i++) {
    Node node=dag->nodes[i];

    /* convert the textual list to an integer list */
    if (string2NodeList(tmp_childrens[i],&childrenlist,&nb_children) == -1) {
      fprintf(stderr,"Error: invalid child list '%s'\n",
		      tmp_childrens[i]);
      return -1;
    }
    /* set the number of children */
    node->nb_children=nb_children;

    /* create the child list with pointers to nodes */
    if (nb_children) {
      node->children=(Node*)calloc(nb_children,sizeof(Node));
      for (j=0;j<nb_children;j++) {
        node->children[j]=dag->nodes[childrenlist[j]];
      }
      free(childrenlist);
    }
  }

  /* Set the parent pointers */
  for (i=0;i<dag->nb_nodes;i++) {
    for (j=0;j<dag->nodes[i]->nb_children;j++) {
      Node node=dag->nodes[i]->children[j];
      node->parents=(Node*)realloc(node->parents,
		      (node->nb_parents+1)*sizeof(Node));
      node->parents[node->nb_parents]=dag->nodes[i];
      (node->nb_parents)++;
    }
  }

  /* A few sanity checks */
  if (!(dag->root) || !(dag->end)) {
    fprintf(stderr,"Error: Graph has missing end points\n");
    return -1;
  }

  if (dag->root->nb_parents != 0) {
    fprintf(stderr,"Error: The Root should have no parents\n");
    return -1;
  }
  if (dag->end->nb_children != 0) {
    fprintf(stderr,"Error: The End should have no children\n");
    return -1;
  }
  return 0;
}

/*
 * parseLine()
 */
static int parseLine(DAG dag, char *line)
{
  char *ptr;

  /* If the line does not start with a keyword, then bail */
  ptr=strchr(line,' ');
  if (ptr==NULL) {
    fprintf(stderr,"Syntax error\n");
    return -1;
  } else{
    *ptr='\0';
  }

  /* deal with the two possible keywords */
  if (!strncmp(line,"NODE_COUNT",strlen("NODE_COUNT")))
    return parseNodeCountLine(dag,ptr+1);
  if (!strncmp(line,"NODE",strlen("NODE")))
    return parseNodeLine(dag,ptr+1);

  fprintf(stderr,"Unknown keyword %s\n",line);
  return -1;
}

/*
 * parseNodeCountLine()
 *
 * NODE_COUNT <number>
 */
int parseNodeCountLine(DAG dag, char *line)
{
  int count;

  /* more than one NODE_COUNT statements ? */
  if (dag->nb_nodes != 0) {
    fprintf(stderr,"Error: Only one Node Count specification allowed\n");
    return -1;
  }

  /* get the count and checks that it's >0 */
  count=atoi(line);
  if (count == 0) {
    fprintf(stderr,"Error: invalid Node count\n");
    return -1;
  }

  /* allocate the node array within the dag */
  dag->nb_nodes=count;
  dag->nodes=(Node *)calloc(dag->nb_nodes,sizeof(Node));
  /* allocate space in the temporary gobal childlist array */
  tmp_childrens=(char **)calloc(dag->nb_nodes,sizeof(char*));
  return 0;
}

/*
 * parseNodeLine()
 *
 * NODE <index> <childlist> <type> <cost>
 */
int parseNodeLine(DAG dag, char *line)
{
  char *ptr;
  Node node;

  char *s_index,*s_childlist,*s_type,*s_cost;
  int index;
  node_t type;
  double cost;

  /* NODE_COUNT should be called before NODE */
  if (dag->nb_nodes == 0) {
    fprintf(stderr,"Error: Node Count must be specified before Nodes\n");
    return -1;
  }

  /* Get index */
  ptr=strchr(line,' ');
  if (!ptr) {
    fprintf(stderr,"Syntax error\n");
    return -1;
  }
  s_index=line;
  *ptr='\0';
  line=ptr+1;


  /* Get childlist */
  ptr=strchr(line,' ');
  if (!ptr) {
    fprintf(stderr,"Syntax error\n");
    return -1;
  }
  s_childlist=line;
  *ptr='\0';
  line=ptr+1;

  /* get node type */
  ptr=strchr(line,' ');
  if (!ptr) {
    fprintf(stderr,"Syntax error\n");
    return -1;
  }
  s_type=line;
  *ptr='\0';
  line=ptr+1;

  /* get cost */
  ptr=strchr(line,'\n');
  if (!ptr) {
    fprintf(stderr,"Syntax error\n");
    return -1;
  }
  s_cost=line;
  *ptr='\0';

  /* Process strings, but store childlist for later */
  index=atoi(s_index);
  if ((type=string2NodeType(s_type)) ==-1)
    return -1;
  cost=atof(s_cost);
  tmp_childrens[index]=strdup(s_childlist);

  /* Creating the node */
  node = newNode();
  node->global_index = index;
  node->cost=cost;
  node->type=type;

  /* Is this the root ? */
  if (node->type == NODE_ROOT) {
    if (dag->root) {
      fprintf(stderr,"Error: only one Root node allowed\n");
      return -1;
    } else {
      dag->root = node;
      
    }
  }
  /* Is this the end ? */
  if (node->type == NODE_END) {
    if (dag->end) {
      fprintf(stderr,"Error: only one End node allowed\n");
      return -1;
    } else {
      dag->end = node;
    }
  }

  /* Is the node beyond the total number of nodes ? */
  if (node->global_index >= dag->nb_nodes) {
    fprintf(stderr,"Error: More nodes than the node count\n");
    return -1;
  }

  /* Have we seen that node before ? */
  if (dag->nodes[node->global_index] != NULL) {
    fprintf(stderr,"Error: Two nodes share index %d\n",
		    node->global_index);
    return -1;
  }
  /* add the node to the global array */
  dag->nodes[node->global_index]=node;
  return 1;
}

/* 
 * string2NodeList()
 * parser x,y,z and returns {x,y,z} and 3
 * Does not really do good error checking
 */
static int string2NodeList(const char *string, int **list, int *nb_nodes)
{
  char *start,*ptr;
  int count=0;

  *list=NULL;

  /* no children "-" */
  if (!strcmp(string,"-")) {
    *nb_nodes=0;
    *list=NULL;
    return 0;
  }

  /* Get all indexes in the list */
  start = (char*)string;
  while((ptr = strchr(start,','))) {
    *ptr='\0';
    *list=(int*)realloc(*list,(count+1)*sizeof(int));
    (*list)[count]=atoi(start);
    count++;
    start = ptr+1;
  }
  *list=(int*)realloc(*list,(count+1)*sizeof(int));
  (*list)[count]=atoi(start);
  count++;

  *nb_nodes=count;
  return 0;
}

/*
 * string2NodeType()
 */
static node_t string2NodeType(const char *string) {
  if (!strcmp(string,"ROOT")) {
    return NODE_ROOT;
  } else if (!strcmp(string,"END")) {
    return NODE_END;
  } else if (!strcmp(string,"COMPUTATION")) {
    return NODE_COMPUTATION;
  } else if (!strcmp(string,"TRANSFER")) {
    return NODE_TRANSFER;
  }
  fprintf(stderr,"Error: Unknown Node Type '%s'\n",string);
  return -1;
}

