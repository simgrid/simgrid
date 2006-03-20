/*      $Id$      */

/* A few basic tests for the graphxml library                               */

/* Copyright (c) 2006 Darina Dimitrova, Arnaud Legrand. All rights reserved.*/

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <xbt/module.h>
#include "xbt/graph.h"
#include "xbt/graphxml.h"


void test(char *graph_file);
void test(char *graph_file)
{
  xbt_graph_t graph = xbt_graph_read(graph_file);
  xbt_graph_free_graph(graph, NULL, NULL, NULL);
}

int main(int argc, char** argv)
{
  xbt_init(&argc,argv);
  if(argc==1) {
     fprintf(stderr,"Usage : %s graph.xml\n",argv[0]);
     return 1;
  }
  test(argv[1]);

  return 0;
}
