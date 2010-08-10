/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_STUB_GENERATOR_H
#define GRAS_STUB_GENERATOR_H

#include "xbt/dynar.h"
#include "xbt/dict.h"
xbt_dict_t process_function_set;
xbt_dynar_t process_list;
xbt_dict_t machine_set;

typedef struct s_process_t {
  int argc;
  char **argv;
  char *host;
} s_process_t;

char *warning;

void s_process_free(void *process);

/* UNIX files */
void generate_sim(const char *project);
void generate_rl(const char *project);
void generate_makefile_am(const char *project);
void generate_makefile_local(const char *project);

#endif
