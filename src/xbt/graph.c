/* 	$Id$	 */

/* a generic graph library.                                                 */

/* Copyright (c) 2006 Darina Dimitrova, Arnaud Legrand. 
   All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/graph.h"
#include "graph_private.h"

/* XBT_LOG_NEW_DEFAULT_SUBCATEGORY(graph,xbt,"Graph"); */

/** Constructor
 * \return a new graph
 */
xbt_graph_t xbt_graph_new_graph(const char *name, unsigned short int directed,
			       void *data)
{
  xbt_graph_t graph=NULL;
  graph=xbt_new0(struct xbt_graph,1);
  graph->directed=directed;
  graph->data=data;
  graph->nodes= xbt_dynar_new(sizeof(xbt_node_t), free);
  graph->edges= xbt_dynar_new(sizeof(xbt_edge_t), free);

  return graph;
}

xbt_node_t xbt_graph_new_node(xbt_graph_t g,const char *name, void *data)
{
  xbt_node_t node=NULL;
  node=xbt_new0(struct xbt_node,1);
  node->data=data;
  node->in=xbt_dynar_new(sizeof(xbt_node_t), free);
  node->out=xbt_dynar_new(sizeof(xbt_node_t), free);
  xbt_dynar_push(g->nodes,node);  

  return node;
}


xbt_edge_t xbt_graph_new_edge(xbt_graph_t g,const char *name,
			     xbt_node_t src, xbt_node_t dst, void *data)
{
  xbt_edge_t edge=NULL;

  edge=xbt_new0(struct xbt_edge,1);
  xbt_dynar_push(src->out,edge); 
  xbt_dynar_push(dst->in,edge); 
  edge->data=data;
  edge->src=src;
  edge->dst=dst;
  if(!g->directed) 
   {
     xbt_dynar_push(src->in,edge); 
     xbt_dynar_push(dst->out,edge); 
   }
 xbt_dynar_push(g->edges,edge);

 return edge; 
}


/** Destructor
 * \param l poor victim
 *
 * Free the graph structure. 
 */
void xbt_graph_free_graph(xbt_graph_t g,
			  void node_free_function(void * ptr),
			  void edge_free_function(void * ptr),
			  void graph_free_function(void * ptr))
{
  int cursor; 
  xbt_node_t node=NULL;
  xbt_edge_t edge=NULL; 

  xbt_dynar_foreach(g->nodes,cursor,node)
    xbt_graph_remove_node(g,node,node_free_function);

  xbt_assert0(!xbt_dynar_length(g->edges),
	      "Damnit, there are some remaining edges!");

  xbt_dynar_foreach(g->edges,cursor,edge)
      xbt_graph_remove_edge(g,edge,edge_free_function); 

  xbt_dynar_free(&(g->nodes));
  xbt_dynar_free(&(g->edges));
      
/*  void xbt_dynar_free(g->edges); */


  return;
}

void xbt_graph_remove_node(xbt_graph_t g, xbt_node_t n, void free_function(void * ptr))
{
  int cursor; 
  xbt_node_t node=NULL;

  if ((free_function)&&(n->data))
   free_function(n->data);
  xbt_dynar_free_container(&(n->in));
  xbt_dynar_free_container(&(n->out));
  xbt_dynar_foreach(g->nodes,cursor,node)
    {
      if (node==n)
	xbt_dynar_cursor_rm(g->nodes,&cursor);   
      
    }
  return;
   
}
void xbt_graph_remove_edge(xbt_graph_t g, xbt_edge_t e, void free_function(void * ptr))
{
   int cursor=0; 
   xbt_edge_t edge=NULL;
   xbt_node_t node=NULL;
   xbt_node_t temp=NULL;
   
   if ((free_function)&&(e->data))
     free_function(e->data);
   xbt_dynar_foreach(g->nodes,cursor,node)
     {
       if (node==e->src)
	 xbt_dynar_pop(node->out,temp);
       if (g->directed)
	 xbt_dynar_pop(node->in,temp);
        
     }
   node=NULL;
   cursor=0;
   xbt_dynar_foreach(g->nodes,cursor,node)
     {
        if (node==e->dst)
	  xbt_dynar_pop(node->in,temp);
	if (g->directed)
	  xbt_dynar_pop(node->out,temp);
        
     }
   cursor=0;
   xbt_dynar_foreach(g->edges,cursor,edge)
     if (edge==e) 
       {
	 xbt_dynar_cursor_rm(g->edges,&cursor);
	 break;
       }
   
}
