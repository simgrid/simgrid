/* 	$Id$	 */

/* a generic graph library.                                                 */

/* Copyright (c) 2006 Darina Dimitrova, Arnaud Legrand.                     */
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdlib.h>
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/graph.h"
#include "graph_private.h"
#include "xbt/graphxml_parse.h"
#include "xbt/dict.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(graph, xbt, "Graph");



/** Constructor
 * \return a new graph
 */
xbt_graph_t xbt_graph_new_graph(unsigned short int directed, void *data)
{
  xbt_graph_t graph = NULL;
  graph = xbt_new0(struct xbt_graph, 1);
  graph->directed = directed;
  graph->data = data;
  graph->nodes = xbt_dynar_new(sizeof(xbt_node_t), NULL);
  graph->edges = xbt_dynar_new(sizeof(xbt_edge_t), NULL);

  return graph;
}

xbt_node_t xbt_graph_new_node(xbt_graph_t g, void *data)
{
  xbt_node_t node = NULL;
  node = xbt_new0(struct xbt_node, 1);
  node->data = data;
  node->in = xbt_dynar_new(sizeof(xbt_node_t), NULL);
  node->out = xbt_dynar_new(sizeof(xbt_node_t), NULL);
  xbt_dynar_push(g->nodes, &node);

  return node;
}


xbt_edge_t xbt_graph_new_edge(xbt_graph_t g,
			      xbt_node_t src, xbt_node_t dst, void *data)
{
  xbt_edge_t edge = NULL;
 

  edge = xbt_new0(struct xbt_edge, 1);
  xbt_dynar_push(src->out, &edge);
  xbt_dynar_push(dst->in, &edge);

  edge->data = data;
  edge->src = src;
  edge->dst = dst;
  if (!g->directed) 
    {
    xbt_dynar_push(src->in, &edge);
    xbt_dynar_push(dst->out, &edge);
  }

  xbt_dynar_push(g->edges, &edge);

  return edge;
}


/** Destructor
 * \param l poor victim
 *
 * Free the graph structure. 
 */
void xbt_graph_free_graph(xbt_graph_t g,
			  void node_free_function(void *ptr),
			  void edge_free_function(void *ptr),
			  void graph_free_function(void *ptr))
{
  int cursor = 0;
  xbt_node_t node = NULL;
  xbt_edge_t edge = NULL;


  xbt_dynar_foreach(g->nodes, cursor, node)
    {
      xbt_dynar_free(&(node->out));
      xbt_dynar_free(&(node->in));
      if(node_free_function)
	node_free_function(node->data);
    }

  xbt_dynar_foreach(g->edges, cursor, edge) 
    {
      if(edge_free_function)
	edge_free_function(edge->data);
    }

  xbt_dynar_foreach(g->nodes, cursor, node)
    free(node);
  xbt_dynar_free(&(g->nodes));

  xbt_dynar_foreach(g->edges, cursor, edge)
    free(edge);
  xbt_dynar_free(&(g->edges));

  free(g);

  return;
}



void xbt_graph_free_node(xbt_graph_t g, xbt_node_t n,
			 void_f_pvoid_t * node_free_function,
			 void_f_pvoid_t * edge_free_function)
{
  unsigned long nbr;
  int i;
  int cursor = 0;
  xbt_node_t node = NULL;
  xbt_edge_t edge = NULL;

  nbr = xbt_dynar_length(g->edges);
  cursor=0;
  for (i = 0; i < nbr; i++)
    {
      xbt_dynar_cursor_get(g->edges, &cursor, &edge);
        
      if ((edge->dst == n) || (edge->src == n))
	{
	  xbt_graph_free_edge(g, edge, edge_free_function);
	} 
      else xbt_dynar_cursor_step( g->edges, &cursor);   
    }


 if ((node_free_function) && (n->data))
    node_free_function(n->data);

  cursor = 0;
  xbt_dynar_foreach(g->nodes, cursor, node)
    {
      if (node == n)
	xbt_dynar_cursor_rm(g->nodes, &cursor);

    }

  return;
}

void xbt_graph_free_edge(xbt_graph_t g, xbt_edge_t e,
			 void free_function(void *ptr))
{
  int idx;
  int cursor = 0;
  xbt_edge_t edge = NULL; 

  if ((free_function) && (e->data))
    free_function(e->data);

  xbt_dynar_foreach(g->edges, cursor, edge) 
    {
    if (edge == e)
      {
	idx = __xbt_find_in_dynar(edge->dst->in,edge); 
	xbt_dynar_remove_at(edge->dst->in, idx,NULL);
	idx = __xbt_find_in_dynar(edge->src->out,edge);
	xbt_dynar_remove_at(edge->src->out,idx,NULL);	
	if (!g->directed)
	  {
	    idx = __xbt_find_in_dynar(edge->src->in,edge); 
	    xbt_dynar_remove_at(edge->src->in,idx,NULL);
	    idx = __xbt_find_in_dynar(edge->dst->out,edge);
	    xbt_dynar_remove_at(edge->dst->out,idx,NULL);	   
	  }
        xbt_dynar_cursor_rm(g->edges, &cursor); 
	free(edge);
	break;
      }
    }
}

int __xbt_find_in_dynar(xbt_dynar_t dynar, void *p)
{

  int cursor = 0;
  void *tmp=NULL;

  xbt_dynar_foreach(dynar, cursor, tmp)
    {
      if (tmp == p)
	return cursor;
    }
  return -1;
}

xbt_dynar_t xbt_graph_get_nodes(xbt_graph_t g)
{
  return g->nodes;
}

xbt_dynar_t xbt_graph_get_edges(xbt_graph_t g)
{
  return g->edges;
}

xbt_node_t xbt_graph_edge_get_source(xbt_edge_t e)
{

  return e->src;
}

xbt_node_t xbt_graph_edge_get_target(xbt_edge_t e)
{
  return e->dst;
}


void xbt_graph_edge_set_length(xbt_edge_t e, double length)
{
  e->length = length;

}

double xbt_graph_edge_get_length(xbt_edge_t e)
{
  return e->length;
}


/*construct the adjacency matrix corresponding to a graph,
  the weights are the distances between nodes
 */
double *xbt_graph_get_length_matrix(xbt_graph_t g)
{
  int cursor = 0;
  int in_cursor = 0;
  int  idx,i;
  unsigned long n;
  xbt_edge_t edge = NULL;
  xbt_node_t node=NULL;
  double *d = NULL;

# define D(u,v) d[(u)*n+(v)]
  n = xbt_dynar_length(g->nodes);

  d = (double *) xbt_new0(double, n*n);

  for (i = 0; i < n * n; i++)
    {
      d[i] = -1.0;
    }
 
  xbt_dynar_foreach(g->nodes, cursor, node) 
    {
      in_cursor = 0;
      D(cursor, cursor) = 0;
    
      xbt_dynar_foreach(node->out, in_cursor, edge)
	{
	  if (edge->dst==node) 
	    idx= __xbt_find_in_dynar(g->nodes, edge->src);
	  else /*case of  undirected graphs*/ 
	     idx = __xbt_find_in_dynar(g->nodes, edge->dst);
	  D( cursor,idx) = edge->length;
	}
    }

# undef D

  return d;
}


void xbt_floyd_algorithm(xbt_graph_t g, double *adj, double *d,
			 xbt_node_t * p)
{
  int i, j, k;
  unsigned long n;
  n = xbt_dynar_length(g->nodes);

# define D(u,v) d[(u)*n+(v)]
# define P(u,v) p[(u)*n+(v)]

  for (i = 0; i < n * n; i++)
    {
      d[i] = adj[i];
    }


  for (i = 0; i < n; i++)
    {
      for (j = 0; j < n; j++)
	{
	  if (D(i, j) != -1)
	    {
	      P(i,j) =*((xbt_node_t*) xbt_dynar_get_ptr(g->nodes, i));	     
	    }
	}
    }
 
  for (k = 0; k < n; k++)
    {
      for (i = 0; i < n; i++)
	{
	  for (j = 0; j < n; j++)
	    {
	      if ((D(i, k) != -1) && (D(k, j) != -1))
		{
		  if ((D(i, j) == -1) || (D(i, j) > D(i, k) + D(k, j)))
		    {
		      D(i, j) = D(i, k) + D(k, j);
		      P(i, j) = P(k, j);
		    }
		}
	    }
	}
    }



# undef P
# undef D
}

/*computes all-pairs shortest paths*/
xbt_node_t *xbt_graph_shortest_paths(xbt_graph_t g)
{
  xbt_node_t *p;
  xbt_node_t *r;
  int i, j, k;
  unsigned long n;

  double *adj = NULL;
  double *d = NULL;

# define P(u,v) p[(u)*n+(v)]
# define R(u,v) r[(u)*n+(v)]

  n = xbt_dynar_length(g->nodes);
  adj = xbt_graph_get_length_matrix(g);
  d = xbt_new0(double,n*n);
  p = xbt_new0(xbt_node_t,n*n);
  r = xbt_new0(xbt_node_t,n*n);
 
  xbt_floyd_algorithm(g, adj, d, p);
 
  for (i = 0; i < n; i++)
    {
      for (j = 0; j < n; j++)
	{
	  k = j;

	  while ((P(i, k)) && (__xbt_find_in_dynar(g->nodes, P(i, k)) != i))
	    {
	      k = __xbt_find_in_dynar(g->nodes, P(i, k));
	    }

	  if (P(i, j))
	    {
	      R(i, j) = *((xbt_node_t*) xbt_dynar_get_ptr(g->nodes, k));
	    }
	}
    }
# undef R
# undef P

  free(d);
  free(p);
  free(adj);
  return r;
}

static xbt_graph_t parsed_graph = NULL;
static xbt_dict_t parsed_nodes = NULL;


static void __parse_graph_begin(void)
{
  DEBUG0("<graph>");
}
static void __parse_graph_end(void)
{
  DEBUG0("</graph>");
}

static void __parse_node(void)
{
  xbt_node_t node =
      xbt_graph_new_node(parsed_graph, NULL);

  xbt_dict_set(parsed_nodes, A_graphxml_node_name, (void *) node, NULL);

  DEBUG1("<node label=\"%s\"/>", (char *) (node->data));
}
static void __parse_edge(void)
{
  xbt_edge_t edge = 
    xbt_graph_new_edge(parsed_graph,
		       xbt_dict_get(parsed_nodes,A_graphxml_edge_source),
		       xbt_dict_get(parsed_nodes,A_graphxml_edge_target),
		       NULL);

  xbt_graph_edge_set_length(edge, atof(A_graphxml_edge_length));

  DEBUG4("<edge name=\"%s\"  source=\"%s\" target=\"%s\" length=\"%f\"/>",
	 (char *) edge->data,
	 (char *) (edge->src)->data,
	 (char *) (edge->dst)->data,
	 xbt_graph_edge_get_length(edge));
}

xbt_graph_t xbt_graph_read(const char *filename)
{
  xbt_graph_t graph = xbt_graph_new_graph(1, NULL);

  parsed_graph = graph;
  parsed_nodes = xbt_dict_new();


  xbt_graph_parse_reset_parser();

  STag_graphxml_graph_fun = __parse_graph_begin;
  ETag_graphxml_graph_fun = __parse_graph_end;
  ETag_graphxml_node_fun = __parse_node;
  ETag_graphxml_edge_fun = __parse_edge;


  xbt_graph_parse_open(filename);
  xbt_assert1((!xbt_graph_parse()), "Parse error in %s", filename);
  xbt_graph_parse_close();

  xbt_dict_free(&parsed_nodes);

  parsed_graph = NULL;
  return graph;
}

void xbt_graph_export_graphviz(xbt_graph_t g, const char *filename,
			       const char *(node_name) (xbt_node_t),
			       const char *(edge_name) (xbt_edge_t))
{
  int cursor = 0;
  xbt_node_t node = NULL;
  xbt_edge_t edge = NULL;
  FILE *file = NULL;
  const char *name=NULL;

  file=fopen(filename,"w");
  xbt_assert1(file, "Failed to open %s \n",filename);

  if(g->directed) fprintf(file,"digraph test {\n");
  else fprintf(file,"graph test {\n");

  fprintf(file,"  graph [overlap=scale]\n");

  fprintf(file,"  node [shape=box, style=filled]\n");
  fprintf(file,"  node [width=.3, height=.3, style=filled, color=skyblue]\n\n");
  
  xbt_dynar_foreach(g->nodes, cursor, node) {
    fprintf(file,"  \"%p\" ", node);
    if((node_name)&&((name=node_name(node)))) fprintf(file,"[label=\"%s\"]",name);
    fprintf(file,";\n");
  }
  xbt_dynar_foreach(g->edges, cursor, edge) {
    if(g->directed)
      fprintf(file,"  \"%p\" -> \"%p\"",edge->src, edge->dst);
    else
      fprintf(file,"  \"%p\" -- \"%p\"",edge->src, edge->dst);
    if((edge_name)&&((name=edge_name(edge)))) fprintf(file,"[label=\"%s\"]",name);
    fprintf(file,";\n");
  }
  fprintf(file,"}\n");
  fclose(file);
}

void xbt_graph_export_graphxml(xbt_graph_t g, const char *filename,
			       const char *(node_name)(xbt_node_t),
			       const char *(edge_name)(xbt_edge_t))
{
  int cursor = 0;
  xbt_node_t node = NULL;
  xbt_edge_t edge = NULL;
  FILE *file = NULL;
  const char *name = NULL;

  file=fopen(filename,"w");
  xbt_assert1(file, "Failed to open %s \n",filename);

  fprintf(file,"<?xml version='1.0'?>\n");
  fprintf(file,"<!DOCTYPE graph SYSTEM \"graphxml.dtd\">\n");
  fprintf(file,"<graph>\n");
  xbt_dynar_foreach(g->nodes, cursor, node) {
    fprintf(file,"  <node name=\"%p\" ", node);
    if((node_name)&&((name=node_name(node)))) fprintf(file,"label=\"%s\" ",name);
    fprintf(file,">\n");
  }
  xbt_dynar_foreach(g->edges, cursor, edge) {
    fprintf(file,"  <edge source=\"%p\" target =\"%p\" ",
	    edge->src, edge->dst );
    if((edge_name)&&((name=edge_name(edge)))) fprintf(file,"label=\"%s\" ",name);
    fprintf(file,">\n");
  }
  fprintf(file,"</graph>\n");
  fclose(file);
}

