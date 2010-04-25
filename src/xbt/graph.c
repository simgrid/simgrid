/* a generic graph library.                                                 */

/* Copyright (c) 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
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
#include "xbt/heap.h"




XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_graph, xbt, "Graph");



/** @brief Constructor
 *  @return a new graph
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

/** @brief add a node to the given graph */
xbt_node_t xbt_graph_new_node(xbt_graph_t g, void *data)
{
  xbt_node_t node = NULL;
  node = xbt_new0(struct xbt_node, 1);
  node->data = data;
  if (g->directed)
    /* only the "out" field is used */
    node->in = xbt_dynar_new(sizeof(xbt_edge_t), NULL);

  node->out = xbt_dynar_new(sizeof(xbt_edge_t), NULL);
  node->position_x = -1.0;
  node->position_y = -1.0;

  xbt_dynar_push(g->nodes, &node);

  return node;
}

/** @brief add an edge to the given graph */
xbt_edge_t xbt_graph_new_edge(xbt_graph_t g,
                              xbt_node_t src, xbt_node_t dst, void *data)
{
  xbt_edge_t edge = NULL;

  edge = xbt_new0(struct xbt_edge, 1);
  xbt_dynar_push(src->out, &edge);
  if (g->directed)
    xbt_dynar_push(dst->in, &edge);
  else                          /* only the "out" field is used */
    xbt_dynar_push(dst->out, &edge);

  edge->data = data;
  edge->src = src;
  edge->dst = dst;

  xbt_dynar_push(g->edges, &edge);

  return edge;
}

xbt_edge_t xbt_graph_get_edge(xbt_graph_t g, xbt_node_t src, xbt_node_t dst)
{
  xbt_edge_t edge;
  unsigned int cursor;

  xbt_dynar_foreach(src->out, cursor, edge) {
    DEBUG3("%p = %p--%p", edge, edge->src, edge->dst);
    if ((edge->src == src) && (edge->dst == dst))
      return edge;
  }
  if (!g->directed) {
    xbt_dynar_foreach(src->out, cursor, edge) {
      DEBUG3("%p = %p--%p", edge, edge->src, edge->dst);
      if ((edge->dst == src) && (edge->src == dst))
        return edge;
    }
  }
  return NULL;
}

void *xbt_graph_node_get_data(xbt_node_t node)
{
  return node->data;
}

void xbt_graph_node_set_data(xbt_node_t node, void *data)
{
  node->data = data;
}

void *xbt_graph_edge_get_data(xbt_edge_t edge)
{
  return edge->data;
}

void xbt_graph_edge_set_data(xbt_edge_t edge, void *data)
{
  edge->data = data;
}

/** @brief Destructor
 *  @param g: poor victim
 *  @param node_free_function: function to use to free data associated to each node
 *  @param edge_free_function: function to use to free data associated to each edge
 *  @param graph_free_function: function to use to free data associated to g
 *
 * Free the graph structure.
 */
void xbt_graph_free_graph(xbt_graph_t g,
                          void_f_pvoid_t node_free_function,
                          void_f_pvoid_t edge_free_function,
                          void_f_pvoid_t graph_free_function)
{
  unsigned int cursor = 0;
  xbt_node_t node = NULL;
  xbt_edge_t edge = NULL;


  xbt_dynar_foreach(g->nodes, cursor, node) {
    xbt_dynar_free(&(node->out));
    xbt_dynar_free(&(node->in));
    if (node_free_function)
      (*node_free_function) (node->data);
  }

  xbt_dynar_foreach(g->edges, cursor, edge) {
    if (edge_free_function)
      (*edge_free_function) (edge->data);
  }

  xbt_dynar_foreach(g->nodes, cursor, node)
    free(node);
  xbt_dynar_free(&(g->nodes));

  xbt_dynar_foreach(g->edges, cursor, edge)
    free(edge);
  xbt_dynar_free(&(g->edges));
  if (graph_free_function)
    (*graph_free_function) (g->data);
  free(g);

  return;
}


/** @brief remove the given node from the given graph */
void xbt_graph_free_node(xbt_graph_t g, xbt_node_t n,
                         void_f_pvoid_t node_free_function,
                         void_f_pvoid_t edge_free_function)
{
  unsigned long nbr;
  unsigned long i;
  unsigned int cursor = 0;
  xbt_node_t node = NULL;
  xbt_edge_t edge = NULL;

  nbr = xbt_dynar_length(g->edges);
  cursor = 0;
  for (i = 0; i < nbr; i++) {
    xbt_dynar_get_cpy(g->edges, cursor, &edge);

    if ((edge->dst == n) || (edge->src == n)) {
      xbt_graph_free_edge(g, edge, edge_free_function);
    } else
      cursor++;
  }

  if ((node_free_function) && (n->data))
    (*node_free_function) (n->data);

  cursor = 0;
  xbt_dynar_foreach(g->nodes, cursor, node)
    if (node == n)
    xbt_dynar_cursor_rm(g->nodes, &cursor);

  xbt_dynar_free(&(n->in));
  xbt_dynar_free(&(n->out));

  free(n);

  return;
}

/** @brief remove the given edge from the given graph */
void xbt_graph_free_edge(xbt_graph_t g, xbt_edge_t e,
                         void_f_pvoid_t free_function)
{
  int idx;
  unsigned int cursor = 0;
  xbt_edge_t edge = NULL;

  if ((free_function) && (e->data))
    (*free_function) (e->data);

  xbt_dynar_foreach(g->edges, cursor, edge) {
    if (edge == e) {
      if (g->directed) {
        idx = __xbt_find_in_dynar(edge->dst->in, edge);
        xbt_dynar_remove_at(edge->dst->in, idx, NULL);
      } else {                  /* only the out field is used */
        idx = __xbt_find_in_dynar(edge->dst->out, edge);
        xbt_dynar_remove_at(edge->dst->out, idx, NULL);
      }

      idx = __xbt_find_in_dynar(edge->src->out, edge);
      xbt_dynar_remove_at(edge->src->out, idx, NULL);

      xbt_dynar_cursor_rm(g->edges, &cursor);
      free(edge);
      break;
    }
  }
}

int __xbt_find_in_dynar(xbt_dynar_t dynar, void *p)
{

  unsigned int cursor = 0;
  void *tmp = NULL;

  xbt_dynar_foreach(dynar, cursor, tmp) {
    if (tmp == p)
      return cursor;
  }
  return -1;
}

/** @brief Retrieve the graph's nodes as a dynar */
xbt_dynar_t xbt_graph_get_nodes(xbt_graph_t g)
{
  return g->nodes;
}

/** @brief Retrieve the graph's edges as a dynar */
xbt_dynar_t xbt_graph_get_edges(xbt_graph_t g)
{
  return g->edges;
}

/** @brief Retrieve the node at the source of the given edge */
xbt_node_t xbt_graph_edge_get_source(xbt_edge_t e)
{
  return e->src;
}

/** @brief Retrieve the node being the target of the given edge */
xbt_node_t xbt_graph_edge_get_target(xbt_edge_t e)
{
  return e->dst;
}

/** @brief Retrieve the outgoing edges of the given node */
xbt_dynar_t xbt_graph_node_get_outedges(xbt_node_t n)
{
  return n->out;
}

/** @brief Set the weight of the given edge */
void xbt_graph_edge_set_length(xbt_edge_t e, double length)
{
  e->length = length;

}

double xbt_graph_edge_get_length(xbt_edge_t e)
{
  return e->length;
}


/** @brief construct the adjacency matrix corresponding to the given graph
 *
 * The weights are the distances between nodes
 */
double *xbt_graph_get_length_matrix(xbt_graph_t g)
{
  unsigned int cursor = 0;
  unsigned int in_cursor = 0;
  unsigned long idx, i;
  unsigned long n;
  xbt_edge_t edge = NULL;
  xbt_node_t node = NULL;
  double *d = NULL;

# define D(u,v) d[(u)*n+(v)]
  n = xbt_dynar_length(g->nodes);

  d = (double *) xbt_new0(double, n * n);

  for (i = 0; i < n * n; i++) {
    d[i] = -1.0;
  }

  xbt_dynar_foreach(g->nodes, cursor, node) {
    in_cursor = 0;
    D(cursor, cursor) = 0;

    xbt_dynar_foreach(node->out, in_cursor, edge) {
      if (edge->dst == node)
        idx = __xbt_find_in_dynar(g->nodes, edge->src);
      else                      /*case of  undirected graphs */
        idx = __xbt_find_in_dynar(g->nodes, edge->dst);
      D(cursor, idx) = edge->length;
    }
  }

# undef D

  return d;
}

/** @brief Floyd-Warshall algorithm for shortest path finding
 *
 * From wikipedia:
 *
 * The Floyd–Warshall algorithm takes as input an adjacency matrix
 * representation of a weighted, directed graph (V, E). The weight of a
 * path between two vertices is the sum of the weights of the edges along
 * that path. The edges E of the graph may have negative weights, but the
 * graph must not have any negative weight cycles. The algorithm computes,
 * for each pair of vertices, the minimum weight among all paths between
 * the two vertices. The running time complexity is Θ(|V|3).
 */
void xbt_floyd_algorithm(xbt_graph_t g, double *adj, double *d,
                         xbt_node_t * p)
{
  unsigned long i, j, k;
  unsigned long n;
  n = xbt_dynar_length(g->nodes);

# define D(u,v) d[(u)*n+(v)]
# define P(u,v) p[(u)*n+(v)]

  for (i = 0; i < n * n; i++) {
    d[i] = adj[i];
  }


  for (i = 0; i < n; i++) {
    for (j = 0; j < n; j++) {
      if (D(i, j) != -1) {
        P(i, j) = *((xbt_node_t *) xbt_dynar_get_ptr(g->nodes, i));
      }
    }
  }

  for (k = 0; k < n; k++) {
    for (i = 0; i < n; i++) {
      for (j = 0; j < n; j++) {
        if ((D(i, k) != -1) && (D(k, j) != -1)) {
          if ((D(i, j) == -1) || (D(i, j) > D(i, k) + D(k, j))) {
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

/** @brief computes all-pairs shortest paths */
xbt_node_t *xbt_graph_shortest_paths(xbt_graph_t g)
{
  xbt_node_t *p;
  xbt_node_t *r;
  unsigned long i, j, k;
  unsigned long n;

  double *adj = NULL;
  double *d = NULL;

# define P(u,v) p[(u)*n+(v)]
# define R(u,v) r[(u)*n+(v)]

  n = xbt_dynar_length(g->nodes);
  adj = xbt_graph_get_length_matrix(g);
  d = xbt_new0(double, n * n);
  p = xbt_new0(xbt_node_t, n * n);
  r = xbt_new0(xbt_node_t, n * n);

  xbt_floyd_algorithm(g, adj, d, p);

  for (i = 0; i < n; i++) {
    for (j = 0; j < n; j++) {
      k = j;

      while ((P(i, k)) && (__xbt_find_in_dynar(g->nodes, P(i, k)) != i)) {
        k = __xbt_find_in_dynar(g->nodes, P(i, k));
      }

      if (P(i, j)) {
        R(i, j) = *((xbt_node_t *) xbt_dynar_get_ptr(g->nodes, k));
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

/** @brief Extract a spanning tree of the given graph */
xbt_edge_t *xbt_graph_spanning_tree_prim(xbt_graph_t g)
{
  int tree_size = 0;
  int tree_size_max = xbt_dynar_length(g->nodes) - 1;
  xbt_edge_t *tree = xbt_new0(xbt_edge_t, tree_size_max);
  xbt_edge_t e, edge;
  xbt_node_t node = NULL;
  xbt_dynar_t edge_list = NULL;
  xbt_heap_t heap = xbt_heap_new(10, NULL);
  unsigned int cursor;

  xbt_assert0(!(g->directed),
              "Spanning trees do not make sense on directed graphs");

  xbt_dynar_foreach(g->nodes, cursor, node) {
    node->xbtdata = NULL;
  }

  node = xbt_dynar_getfirst_as(g->nodes, xbt_node_t);
  node->xbtdata = (void *) 1;
  edge_list = node->out;
  xbt_dynar_foreach(edge_list, cursor, e)
    xbt_heap_push(heap, e, -(e->length));

  while ((edge = xbt_heap_pop(heap))) {
    if ((edge->src->xbtdata) && (edge->dst->xbtdata))
      continue;
    tree[tree_size++] = edge;
    if (!(edge->src->xbtdata)) {
      edge->src->xbtdata = (void *) 1;
      edge_list = edge->src->out;
      xbt_dynar_foreach(edge_list, cursor, e) {
        xbt_heap_push(heap, e, -(e->length));
      }
    } else {
      edge->dst->xbtdata = (void *) 1;
      edge_list = edge->dst->out;
      xbt_dynar_foreach(edge_list, cursor, e) {
        xbt_heap_push(heap, e, -(e->length));
      }
    }
    if (tree_size == tree_size_max)
      break;
  }

  xbt_heap_free(heap);

  return tree;
}

/** @brief Topological sort on the given graph
 *
 *  From wikipedia:
 *
 * In graph theory, a topological sort of a directed acyclic graph (DAG) is
 * a linear ordering of its nodes which is compatible with the partial
 * order R induced on the nodes where x comes before y (xRy) if there's a
 * directed path from x to y in the DAG. An equivalent definition is that
 * each node comes before all nodes to which it has edges. Every DAG has at
 * least one topological sort, and may have many.
 */
xbt_node_t *xbt_graph_topo_sort(xbt_graph_t g)
{

  xbt_node_t *sorted;
  unsigned int cursor;
  int idx;
  xbt_node_t node;
  unsigned long n;

  n = xbt_dynar_length(g->nodes);
  idx = n - 1;

  sorted = xbt_malloc(n * sizeof(xbt_node_t));

  xbt_dynar_foreach(g->nodes, cursor, node)
    node->xbtdata = xbt_new0(int, 1);

  xbt_dynar_foreach(g->nodes, cursor, node)
    xbt_graph_depth_visit(g, node, sorted, &idx);

  xbt_dynar_foreach(g->nodes, cursor, node) {
    free(node->xbtdata);
    node->xbtdata = NULL;
  }

  return sorted;
}

/** @brief First-depth graph traversal */
void xbt_graph_depth_visit(xbt_graph_t g, xbt_node_t n,
                           xbt_node_t * sorted, int *idx)
{
  unsigned int cursor;
  xbt_edge_t edge;

  if (*((int *) (n->xbtdata)) == ALREADY_EXPLORED)
    return;
  else if (*((int *) (n->xbtdata)) == CURRENTLY_EXPLORING)
    THROW0(0, 0, "There is a cycle");
  else {
    *((int *) (n->xbtdata)) = CURRENTLY_EXPLORING;

    xbt_dynar_foreach(n->out, cursor, edge) {
      xbt_graph_depth_visit(g, edge->dst, sorted, idx);
    }

    *((int *) (n->xbtdata)) = ALREADY_EXPLORED;
    sorted[(*idx)--] = n;
  }
}

/********************* Import and Export ******************/
static xbt_graph_t parsed_graph = NULL;
static xbt_dict_t parsed_nodes = NULL;

static void *(*__parse_node_label_and_data) (xbt_node_t, const char *,
                                             const char *) = NULL;
static void *(*__parse_edge_label_and_data) (xbt_edge_t, const char *,
                                             const char *) = NULL;

static void __parse_graph_begin(void)
{
  DEBUG0("<graph>");
  if (A_graphxml_graph_isDirected == A_graphxml_graph_isDirected_true)
    parsed_graph = xbt_graph_new_graph(1, NULL);
  else
    parsed_graph = xbt_graph_new_graph(0, NULL);

  parsed_nodes = xbt_dict_new();
}

static void __parse_graph_end(void)
{
  xbt_dict_free(&parsed_nodes);
  DEBUG0("</graph>");
}

static void __parse_node(void)
{
  xbt_node_t node = xbt_graph_new_node(parsed_graph, NULL);

  DEBUG1("<node name=\"%s\"/>", A_graphxml_node_name);
  if (__parse_node_label_and_data)
    node->data = __parse_node_label_and_data(node, A_graphxml_node_label,
                                             A_graphxml_node_data);
  xbt_graph_parse_get_double(&(node->position_x), A_graphxml_node_position_x);
  xbt_graph_parse_get_double(&(node->position_y), A_graphxml_node_position_y);

  xbt_dict_set(parsed_nodes, A_graphxml_node_name, (void *) node, NULL);
}

static void __parse_edge(void)
{
  xbt_edge_t edge = xbt_graph_new_edge(parsed_graph,
                                       xbt_dict_get(parsed_nodes,
                                                    A_graphxml_edge_source),
                                       xbt_dict_get(parsed_nodes,
                                                    A_graphxml_edge_target),
                                       NULL);

  if (__parse_edge_label_and_data)
    edge->data = __parse_edge_label_and_data(edge, A_graphxml_edge_label,
                                             A_graphxml_edge_data);

  xbt_graph_parse_get_double(&(edge->length), A_graphxml_edge_length);

  DEBUG3("<edge  source=\"%s\" target=\"%s\" length=\"%f\"/>",
         (char *) (edge->src)->data,
         (char *) (edge->dst)->data, xbt_graph_edge_get_length(edge));
}

/** @brief Import a graph from a file following the GraphXML format */
xbt_graph_t xbt_graph_read(const char *filename,
                           void *(*node_label_and_data) (xbt_node_t,
                                                         const char *,
                                                         const char *),
                           void *(*edge_label_and_data) (xbt_edge_t,
                                                         const char *,
                                                         const char *))
{

  xbt_graph_t graph = NULL;

  __parse_node_label_and_data = node_label_and_data;
  __parse_edge_label_and_data = edge_label_and_data;

  xbt_graph_parse_reset_parser();

  STag_graphxml_graph_fun = __parse_graph_begin;
  ETag_graphxml_graph_fun = __parse_graph_end;
  ETag_graphxml_node_fun = __parse_node;
  ETag_graphxml_edge_fun = __parse_edge;

  xbt_graph_parse_open(filename);
  xbt_assert1((!(*xbt_graph_parse) ()), "Parse error in %s", filename);
  xbt_graph_parse_close();

  graph = parsed_graph;
  parsed_graph = NULL;

  return graph;
}

/** @brief Export the given graph in the GraphViz formatting for visualization */
void xbt_graph_export_graphviz(xbt_graph_t g, const char *filename,
                               const char *(node_name) (xbt_node_t),
                               const char *(edge_name) (xbt_edge_t))
{
  unsigned int cursor = 0;
  xbt_node_t node = NULL;
  xbt_edge_t edge = NULL;
  FILE *file = NULL;
  const char *name = NULL;

  file = fopen(filename, "w");
  xbt_assert1(file, "Failed to open %s \n", filename);

  if (g->directed)
    fprintf(file, "digraph test {\n");
  else
    fprintf(file, "graph test {\n");

  fprintf(file, "  graph [overlap=scale]\n");

  fprintf(file, "  node [shape=box, style=filled]\n");
  fprintf(file,
          "  node [width=.3, height=.3, style=filled, color=skyblue]\n\n");

  xbt_dynar_foreach(g->nodes, cursor, node) {
    fprintf(file, "  \"%p\" ", node);
    if ((node_name) && ((name = node_name(node))))
      fprintf(file, "[label=\"%s\"]", name);
    fprintf(file, ";\n");
  }
  xbt_dynar_foreach(g->edges, cursor, edge) {
    if (g->directed)
      fprintf(file, "  \"%p\" -> \"%p\"", edge->src, edge->dst);
    else
      fprintf(file, "  \"%p\" -- \"%p\"", edge->src, edge->dst);
    if ((edge_name) && ((name = edge_name(edge))))
      fprintf(file, "[label=\"%s\"]", name);
    fprintf(file, ";\n");
  }
  fprintf(file, "}\n");
  fclose(file);
}

/** @brief Export the given graph in the GraphXML format */
void xbt_graph_export_graphxml(xbt_graph_t g, const char *filename,
                               const char *(node_name) (xbt_node_t),
                               const char *(edge_name) (xbt_edge_t),
                               const char *(node_data_print) (void *),
                               const char *(edge_data_print) (void *))
{
  unsigned int cursor = 0;
  xbt_node_t node = NULL;
  xbt_edge_t edge = NULL;
  FILE *file = NULL;
  const char *name = NULL;

  file = fopen(filename, "w");
  xbt_assert1(file, "Failed to open %s \n", filename);

  fprintf(file, "<?xml version='1.0'?>\n");
  fprintf(file, "<!DOCTYPE graph SYSTEM \"graphxml.dtd\">\n");
  if (g->directed)
    fprintf(file, "<graph isDirected=\"true\">\n");
  else
    fprintf(file, "<graph isDirected=\"false\">\n");
  xbt_dynar_foreach(g->nodes, cursor, node) {
    fprintf(file, "  <node name=\"%p\" ", node);
    if ((node_name) && ((name = node_name(node))))
      fprintf(file, "label=\"%s\" ", name);
    if ((node_data_print) && ((name = node_data_print(node->data))))
      fprintf(file, "data=\"%s\" ", name);
    fprintf(file, ">\n");
  }
  xbt_dynar_foreach(g->edges, cursor, edge) {
    fprintf(file, "  <edge source=\"%p\" target =\"%p\" ",
            edge->src, edge->dst);
    if ((edge_name) && ((name = edge_name(edge))))
      fprintf(file, "label=\"%s\" ", name);
    if (edge->length >= 0.0)
      fprintf(file, "length=\"%g\" ", edge->length);
    if ((edge_data_print) && ((name = edge_data_print(edge->data))))
      fprintf(file, "data=\"%s\" ", name);
    fprintf(file, ">\n");
  }
  fprintf(file, "</graph>\n");
  fclose(file);
}
