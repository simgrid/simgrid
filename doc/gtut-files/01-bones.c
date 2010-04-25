/* Copyright (c) 2006, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include <gras.h>

int client(int argc, char *argv[]) {
  gras_init(&argc,argv);

  /* Your own code for the client process */
  
  gras_exit();
  return 0;
}

int server(int argc, char *argv[]) {
  gras_init(&argc,argv);

  /* Your own code for the server process */
  
  gras_exit();
  return 0;
}
