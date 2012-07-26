

#include <simgrid/platf_generator.h>
#include <xbt.h>
#include <xbt/graph.h>
#include <xbt/RngStream.h>


typedef struct {
  double x, y;
  int degree;
  e_platf_node_kind kind;
} s_context_node_t, *context_node_t;

static xbt_graph_t platform_graph = NULL;

static RngStream rng_stream = NULL;

void platf_random_seed(unsigned long seed[6]) {
  
  if(rng_stream == NULL) {
    //stream not created yet, we do it now
    rng_stream = RngStream_CreateStream(NULL);
  }
  if(seed != NULL) {
    RngStream_SetSeed(rng_stream, seed);
  }
}

void platf_graph_init(int node_count, e_platf_placement placement) {
  int i;
  platform_graph = xbt_graph_new_graph(TRUE, NULL);
  if(rng_stream == NULL) {
    rng_stream = RngStream_CreateStream(NULL);
  }
  
  for(i=0 ; i<node_count ; i++) {
    context_node_t node_data = NULL;
    node_data = xbt_new(s_context_node_t, 1);
    switch(placement) {
      case UNIFORM:
        node_data->x = RngStream_RandU01(rng_stream);
        node_data->y = RngStream_RandU01(rng_stream);
        break;
      case HEAVY_TAILED:
        //Not implemented...
        THROW_UNIMPLEMENTED;
    }
    node_data->degree = 0;
    node_data->kind = ROUTER;
    xbt_graph_new_node(platform_graph, (void*) node_data);
  }
}
