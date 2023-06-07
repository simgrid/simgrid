/* Copyright (c) 2008-2023. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/instr.h"
#include "simgrid/s4u.hpp"

int main(int argc, char** argv)
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc == 3, "Usage: %s <platform_file.xml> <graphviz_file.dot|graphviz_file.csv>", argv[0]);

  e.load_platform(argv[1]);
  e.seal_platform();

  const std::string outputfile(argv[2]);
  const std::string extension = outputfile.substr(outputfile.find_last_of(".") + 1);
  if(extension == "csv") {
    printf("Dumping to CSV file\n");
    simgrid::instr::platform_graph_export_csv(outputfile);
  }
  else if(extension == "dot") {
    printf("Dumping to DOT file\n");
    simgrid::instr::platform_graph_export_graphviz(outputfile);
  }
  else {
    xbt_die("Unknown output file format, please use '.dot' or .csv' extension");
  }
  return 0;
}
