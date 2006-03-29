/*      $Id$      */

/* A few basic tests for the graphxml library                               */

/* Copyright (c) 2006 Darina Dimitrova, Arnaud Legrand. All rights reserved.*/

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <xbt/module.h>
#include "xbt/graph.h"
#include "xbt/graphxml.h"
#include "xbt/dynar.h"
#include "xbt/sysdep.h"
#include "../src/xbt/graph_private.h"
void* node_label_and_data(xbt_node_t node,const char*label ,const char* data);
void* node_label_and_data(xbt_node_t node,const char*label ,const char* data)
{
  char* lbl=xbt_strdup(label);
  return lbl;
}

static const char *node_name(xbt_node_t n) {
  return n->data;
}

void test(char *graph_file);
void test(char *graph_file)
{
  int i,j;
  unsigned long n;
  xbt_dynar_t dynar=NULL;
  xbt_dynar_t dynar1=NULL;
  xbt_node_t* sorted=NULL;
  xbt_node_t * route=NULL;

  xbt_graph_t graph = xbt_graph_read(graph_file,&node_label_and_data,NULL);
  n=xbt_dynar_length(xbt_graph_get_nodes( graph));

  double *adj=xbt_graph_get_length_matrix(graph);
 
  xbt_graph_export_graphviz(graph, "graph.dot", node_name, NULL);
 
 for(i=0;i<n;i++)
    {
      for(j=0;j<n;j++)
	{
	  fprintf(stderr,"%le\t",adj[i*n+j] );
	}
      fprintf(stderr,"\n" );
    }


route= xbt_graph_shortest_paths( graph);
 
 /*  for(i=0;i<n;i++) */
/*     { */
/*       for(j=0;j<n;j++) */
/* 	{ */
/* 	  if( route[i*n+j]) */
/* 	  fprintf(stderr,"%s\t",(char*)(route[i*n+j])->data) ); */
/* 	} */
/*       fprintf(stderr,"\n" ); */
/*     } */

  
sorted= xbt_graph_topo_sort(graph);

  for(i=0;i<n;i++)
    {
      if (sorted[i]){
/* fprintf(stderr,"sorted[%d] =%p\n",i,sorted[i] ); */
	  fprintf(stderr,"%s\t",
		  (char*)((sorted[i])->data)
		 ) ;
	
	  fprintf(stderr,"\n" );}
    }
 
 /*  while(xbt_dynar_length(dynar)) */
/*     xbt_graph_free_node(graph,*((xbt_node_t*)xbt_dynar_get_ptr(dynar,0)),NULL,NULL); */

  dynar = xbt_graph_get_edges(graph);
while(xbt_dynar_length(dynar))
    xbt_graph_free_edge(graph,*((xbt_edge_t*)xbt_dynar_get_ptr(dynar,0)),NULL);
 
  printf("%lu edges\n",xbt_dynar_length(dynar));
 dynar1 = xbt_graph_get_nodes(graph);
 printf("%lu nodes\n",xbt_dynar_length(dynar1));
 xbt_free(adj);
 xbt_free(route);
 xbt_graph_free_graph(graph, NULL, NULL, NULL);
}

int main(int argc, char** argv)
{
  xbt_init(&argc,argv);
  if(argc==1) 
    {
     fprintf(stderr,"Usage : %s graph.xml\n",argv[0]);
  
     return 1;
  }
  test(argv[1]);

  return 0;
}
