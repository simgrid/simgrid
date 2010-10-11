/* Copyright (c) 2006, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include <gras.h>
#include <stdio.h>

/* We must set this to make logging mecanism happy */
extern const char *_gras_procname;

/* This is a private data of gras/Datadesc we want to explore */
xbt_set_t gras_datadesc_set_local = NULL;

int main(int argc, char *argv[])
{
  xbt_set_elm_t elm;
  xbt_set_cursor_t cursor;
  gras_init(&argc, argv);

  xbt_set_foreach(gras_datadesc_set_local, cursor, elm) {
    printf("%s\n", elm->name);
  }


  gras_exit();
  return 0;
}
