#ifndef MIXTESIM_PROTOTYPES_H
#define MIXTESIM_PROTOTYPES_H

#include "mixtesim_types.h"
#include "mixtesim_datastructures.h"

#ifndef MAX
#  define MAX(x,y) ((x) > (y) ? (x) : (y))
#endif
#ifndef MIN
#  define MIN(x,y) ((x) > (y) ? (x) : (y))
#endif


int HEFT(DAG dag);

/* parsing  */
/*Grid parseGridFile(char*filename);*/
DAG parseDAGFile(char*filename);

int createSimgridObjects();

#endif /* MIXTESIM_PROTOTYPES_H */
