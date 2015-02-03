/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <string.h>
#include <assert.h>

#include <xbt.h>
#include <mc/mc.h>

#include "../../src/include/mc/datatypes.h"
#include "../../src/mc/mc_object_info.h"
#include "../../src/mc/mc_private.h"
#include "../../src/mc/mc_model_checker.h"

int test_some_array[4][5][6];
struct some_struct { int first; int second[4][5]; } test_some_struct;

static dw_type_t find_type_by_name(mc_object_info_t info, const char* name) {
  xbt_dict_cursor_t cursor = NULL;
  char *key;
  dw_type_t type;
  xbt_dict_foreach(info->types, cursor, key, type) {
    if(!strcmp(name, type->name))
      return type;
  }

  return NULL;
}

static dw_frame_t find_function_by_name(mc_object_info_t info, const char* name) {
  xbt_dict_cursor_t cursor = 0;
  dw_frame_t subprogram;
  char* key;
  xbt_dict_foreach(info->subprograms, cursor, key, subprogram){
    if(!strcmp(name, subprogram->name))
      return subprogram;
  }

  return NULL;
}

static dw_variable_t find_local_variable(dw_frame_t frame, const char* argument_name) {
  unsigned int cursor = 0;
  dw_variable_t variable;
  xbt_dynar_foreach(frame->variables, cursor, variable){
    if(!strcmp(argument_name, variable->name))
      return variable;
  }

  dw_frame_t scope = NULL;
  xbt_dynar_foreach(frame->scopes, cursor, scope) {
    variable = find_local_variable(scope, argument_name);
    if(variable)
      return variable;
  }

  return NULL;
}

static void test_local_variable(mc_object_info_t info, const char* function, const char* variable, void* address, unw_cursor_t* cursor) {
  dw_frame_t subprogram = find_function_by_name(info, function);
  assert(subprogram);
  // TODO, Lookup frame by IP and test against name instead

  dw_variable_t var = find_local_variable(subprogram, variable);
  assert(var);

  void* frame_base = mc_find_frame_base(subprogram, info, cursor);
  s_mc_location_t location;

  mc_dwarf_resolve_locations(&location,
    &var->locations, info, cursor, frame_base, NULL, -1);

  xbt_assert(mc_get_location_type(&location)==MC_LOCATION_TYPE_ADDRESS,
    "Unexpected location type for variable %s of %s", variable, function);

  xbt_assert(location.memory_location == address,
    "Bad resolution of local variable %s of %s", variable, function);

}

static dw_variable_t test_global_variable(mc_process_t process, mc_object_info_t info, const char* name, void* address, long byte_size) {
  
  dw_variable_t variable = MC_file_object_info_find_variable_by_name(info, name);
  xbt_assert(variable, "Global variable %s was not found", name);
  xbt_assert(!strcmp(variable->name, name), "Name mismatch for %s", name);
  xbt_assert(variable->global, "Variable %s is not global", name);
  xbt_assert(variable->address == address,
      "Address mismatch for %s : %p expected but %p found", name, address, variable->address);

  dw_type_t type = xbt_dict_get_or_null(process->binary_info->types, variable->type_origin);
  xbt_assert(type!=NULL, "Missing type for %s", name);
  xbt_assert(type->byte_size = byte_size, "Byte size mismatch for %s", name);
  return variable;
}

static dw_type_t find_member(mc_object_info_t info, const char* name, dw_type_t type) {
  unsigned int cursor = 0;
  dw_type_t member;
  xbt_dynar_foreach(type->members, cursor, member){
    if(!strcmp(name,member->name))
      return member;
  }
  return NULL;
}

int some_local_variable = 0;

typedef struct foo {int i;} s_foo;

static void test_type_by_name(mc_process_t process, s_foo my_foo) {
  assert(xbt_dict_get_or_null(process->binary_info->full_types_by_name, "struct foo"));
}

int main(int argc, char** argv)
{
  SIMIX_global_init(&argc, argv);

  dw_variable_t var;
  dw_type_t type;
  
  s_mc_process_t p;
  mc_process_t process = &p;
  MC_process_init(&p, getpid(), -1);

  test_global_variable(process, process->binary_info, "some_local_variable", &some_local_variable, sizeof(int));

  var = test_global_variable(process, process->binary_info, "test_some_array", &test_some_array, sizeof(test_some_array));
  type = xbt_dict_get_or_null(process->binary_info->types, var->type_origin);
  xbt_assert(type->element_count == 6*5*4, "element_count mismatch in test_some_array : %i / %i", type->element_count, 6*5*4);

  var = test_global_variable(process, process->binary_info, "test_some_struct", &test_some_struct, sizeof(test_some_struct));
  type = xbt_dict_get_or_null(process->binary_info->types, var->type_origin);
  assert(find_member(process->binary_info, "first", type)->offset == 0);
  assert(find_member(process->binary_info, "second", type)->offset
      == ((const char*)&test_some_struct.second) - (const char*)&test_some_struct);

  unw_context_t context;
  unw_cursor_t cursor;
  unw_getcontext(&context);
  unw_init_local(&cursor, &context);

  test_local_variable(process->binary_info, "main", "argc", &argc, &cursor);

  {
    int lexical_block_variable = 50;
    test_local_variable(process->binary_info, "main", "lexical_block_variable", &lexical_block_variable, &cursor);
  }

  s_foo my_foo;
  test_type_by_name(process, my_foo);

  _exit(0);
}
