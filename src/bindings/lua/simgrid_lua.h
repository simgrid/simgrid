/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_LUA_H
#define SIMGRID_LUA_H

#include "xbt/dict.h"
#include "xbt/dynar.h"
#include <lua.h>

extern xbt_dynar_t process_list;
extern xbt_dict_t machine_set;

typedef struct s_process_t {
  int argc;
  char **argv;
  char *host;
} s_process_t;

void s_process_free(void *process);

/* UNIX files */
void generate_sim(const char *project);
void generate_rl(const char *project);
void generate_makefile_am(const char *project);
void generate_makefile_local(const char *project);

/* ********************************************************************************* */
/*                           Plaftorm functions                                      */
/* ********************************************************************************* */

int console_open(lua_State *L);
int console_close(lua_State *L);

int console_add_host(lua_State*);
int console_add_link(lua_State*);
int console_add_router(lua_State* L);
int console_add_route(lua_State*);
int console_AS_open(lua_State*);
int console_AS_close(lua_State *L);
int console_set_function(lua_State*);
int console_host_set_property(lua_State*);

#endif  /* SIMGRID_LUA_H */
