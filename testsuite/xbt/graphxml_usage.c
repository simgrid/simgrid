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
#include <malloc.h>



void test(char *graph_file);
void test(char *graph_file)
{
  int i,j;

  unsigned long n;
  xbt_dynar_t dynar=NULL;
  xbt_dynar_t dynar1=NULL;
  xbt_graph_t graph = xbt_graph_read(graph_file);
  n=xbt_dynar_length(xbt_graph_get_nodes( graph));


  double *d=xbt_graph_get_length_matrix(graph);
  
  for(i=0;i<n;i++)
    {
      for(j=0;j<n;j++)
	{
	  fprintf(stderr,"%le\t",d[i*n+j] );
	}
      fprintf(stderr,"\n" );
    }
  dynar = xbt_graph_get_nodes(graph);

 
  while(xbt_dynar_length(dynar))
    xbt_graph_free_node(graph,*((xbt_node_t*)xbt_dynar_get_ptr(dynar,0)),NULL,NULL);

  dynar = xbt_graph_get_edges(graph);
  printf("%lu edges\n",xbt_dynar_length(dynar));
 dynar1 = xbt_graph_get_nodes(graph);
 printf("%lu nodes\n",xbt_dynar_length(dynar1));
/*  free(d); */
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
