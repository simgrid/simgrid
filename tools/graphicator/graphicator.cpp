/* Copyright (c) 2008-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/instr.h"
#include "simgrid/s4u.hpp"

int main(int argc, char** argv)
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc == 3, "Usage: %s <platform_file.xml> <graphviz_file.dot>", argv[0]);

  e.load_platform(argv[1]);
  TRACE_platform_graph_export_graphviz(argv[2]);
  return 0;
}
