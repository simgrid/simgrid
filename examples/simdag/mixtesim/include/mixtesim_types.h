#ifndef MIXTESIM_TYPES_H
#define MIXTESIM_TYPES_H

typedef struct _DAG 	*DAG;
typedef struct _Node 	*Node;

typedef enum {
  NODE_COMPUTATION=0,
  NODE_TRANSFER,
  NODE_ROOT,
  NODE_END
} node_t;

#endif /* MIXTESIM_TYPES_H */
