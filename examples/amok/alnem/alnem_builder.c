/* ALNeM builder. Take an interference matrix as argument,                  */
/*  and reconstruct the corresponding graph itself                          */

/* Copyright (c) 2003 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include <stdio.h>
#include <stdlib.h>

#include <tbx_graph.h> /* alvin's graph toolbox (+ reconstruction algorithm) */

int main(int argc,char *argv[]) {
  TBX_Graph_t graph; /* a dummy graph containing all hosts */
  TBX_FIFO_t host_fifo;
  TBX_InterfTable_t interf; /* the measured interferences */
  TBX_Graph_t builded_graph; /* the graph builded from the interferences */

  if (argc != 2) {
    fprintf(stderr,"alnem_builder: USAGE:\n");
    fprintf(stderr,"  alnem_builder interference_file\n");
    exit (1);
  }

  if (TBX_Graph_interferenceTableRead (argv[1],&graph,&interf,&host_fifo)) {
    fprintf(stderr,"Can't read the interference data, aborting\n");
    exit (1);
  }

  builded_graph = TBX_Graph_exploreInterference(interf);
  TBX_Graph_exportToGraphViz(builded_graph, "toto.dot");
  return 0;
}
