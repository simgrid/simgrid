#ifndef _XBT_GRAPH_H
#define _XBT_GRAPH_H
#include "xbt/misc.h" /* SG_BEGIN_DECL */
#include "xbt/fifo.h" 
#include "xbt/dict.h" 

SG_BEGIN_DECL()

typedef struct xbt_node *xbt_node_t;
typedef struct xbt_edge  *xbt_edge_t;
typedef struct xbt_graph  *xbt_graph_t;

/* API */
xbt_graph_t xbt_graph_new_graph(const char *name, unsigned short int directed, void *data);
xbt_node_t xbt_graph_new_node(xbt_graph_t g,const char *name, void *data);
xbt_edge_t xbt_graph_new_edge(xbt_graph_t g,const char *name,
			     xbt_node_t src, xbt_node_t dst, void *data);

void xbt_graph_remove_node(xbt_graph_t g, xbt_node_t n, void free_function(void * ptr));
void xbt_graph_remove_edge(xbt_graph_t g, xbt_edge_t e, void free_function(void * ptr));
void xbt_graph_free_graph(xbt_graph_t g, 
			 void node_free_function(void * ptr),
			 void edge_free_function(void * ptr),
			 void graph_free_function(void * ptr));

void xbt_export_graphviz(xbt_graph_t g, const char *filename,
			 const char *(node_name)(xbt_node_t),
			 const char *(edge_name)(xbt_edge_t)
			);

			void xbt_graph_export_surfxml(xbt_graph_t g,
						      const char *filename,
						      const char *(node_name)(xbt_node_t),
						      const char *(edge_name)(xbt_edge_t)
						     );

void *xbt_graph_to_array(xbt_graph_t g); 
void xbt_graph_shortest_paths(xbt_graph_t g);
void xbt_graph_topological_sort(xbt_graph_t g);




/** Convenient for loop : g is a graph, n a node, e an edge, b a bucket and i an item **/

/* #define xbt_graph_foreachInNeighbor(v,n,i)            \ */
/*    for(i=xbt_fifo_get_first_item((v)->in);              \ */
/*      ((i)?(n=((xbt_edge_t)((xbt_fifo_get_item_content(i)) */
/* )->src):(NULL));\ */
/*        i=xbt_fifo_get_next_item(i)) */
/* #define xbt_graph_foreachOutNeighbor(v,n,i)           \ */
/*    for(i=xbt_fifo_get_first_item((v)->out);             \ */
/*      ((i)?(n=((xbt_edge_t)(xbt_fifo_get_item_content(i)))->dst):(NULL));\ */
/*        i=xbt_fifo_get_next_item(i)) */



SG_END_DECL()

#endif				/* _XBT_GRAPH_H */






