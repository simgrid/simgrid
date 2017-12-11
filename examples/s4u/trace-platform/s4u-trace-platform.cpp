/* Copyright (c) 2010, 2012-2016. The SimGrid Team. All rights reserved.    */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.loadPlatform(argv[1]);
  e.run();
  return 0;
}
