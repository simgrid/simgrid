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

int test_some_array[4][5][6];
struct some_struct { int first; int second[4][5]; } test_some_struct;

static mc_type_t find_type_by_name(mc_object_info_t info, const char* name)
{
  for (auto& entry : info->types)
    if(entry.second.name == name)
      return &entry.second;
  return nullptr;
}

static mc_frame_t find_function_by_name(
    mc_object_info_t info, const char* name)
{
  for (auto& entry : info->subprograms)
    if(entry.second.name == name)
      return &entry.second;
  return nullptr;
}

static mc_variable_t find_local_variable(
    mc_frame_t frame, const char* argument_name)
{
  for (simgrid::mc::Variable& variable : frame->variables)
    if(argument_name == variable.name)
      return &variable;

  for (simgrid::mc::Frame& scope : frame->scopes) {
    simgrid::mc::Variable* variable = find_local_variable(
      &scope, argument_name);
    if(variable)
      return variable;
  }

  return nullptr;
}

static void test_local_variable(mc_object_info_t info, const char* function, const char* variable, void* address, unw_cursor_t* cursor) {
  mc_frame_t subprogram = find_function_by_name(info, function);
  assert(subprogram);
  // TODO, Lookup frame by IP and test against name instead

  mc_variable_t var = find_local_variable(subprogram, variable);
  assert(var);

  void* frame_base = mc_find_frame_base(subprogram, info, cursor);
  s_mc_location_t location;

  mc_dwarf_resolve_locations(&location,
    &var->location_list, info, cursor, frame_base, NULL, -1);

  xbt_assert(mc_get_location_type(&location)==MC_LOCATION_TYPE_ADDRESS,
    "Unexpected location type for variable %s of %s", variable, function);

  xbt_assert(location.memory_location == address,
    "Bad resolution of local variable %s of %s", variable, function);

}

static mc_variable_t test_global_variable(mc_process_t process, mc_object_info_t info, const char* name, void* address, long byte_size) {

  mc_variable_t variable = info->find_variable(name);
  xbt_assert(variable, "Global variable %s was not found", name);
  xbt_assert(variable->name == name,
    "Name mismatch for %s", name);
  xbt_assert(variable->global, "Variable %s is not global", name);
  xbt_assert(variable->address == address,
      "Address mismatch for %s : %p expected but %p found",
      name, address, variable->address);

  auto i = process->binary_info->types.find(variable->type_id);
  xbt_assert(i != process->binary_info->types.end(), "Missing type for %s", name);
  mc_type_t type = &i->second;
  xbt_assert(type->byte_size = byte_size, "Byte size mismatch for %s", name);
  return variable;
}

static mc_type_t find_member(mc_object_info_t info, const char* name, mc_type_t type)
{
  for (simgrid::mc::Type& member : type->members)
    if(member.name == name)
      return &member;
  return nullptr;
}

int some_local_variable = 0;

typedef struct foo {int i;} s_foo;

static void test_type_by_name(mc_process_t process, s_foo my_foo)
{
  assert(
    process->binary_info->full_types_by_name.find("struct foo") !=
      process->binary_info->full_types_by_name.end());
}

int main(int argc, char** argv)
{
  SIMIX_global_init(&argc, argv);

  mc_variable_t var;
  mc_type_t type;

  s_mc_process_t p(getpid(), -1);
  mc_process_t process = &p;

  test_global_variable(process, process->binary_info.get(),
    "some_local_variable", &some_local_variable, sizeof(int));

  var = test_global_variable(process, process->binary_info.get(),
    "test_some_array", &test_some_array, sizeof(test_some_array));
  auto i = process->binary_info->types.find(var->type_id);
  xbt_assert(i != process->binary_info->types.end(), "Missing type");
  type = &i->second;
  xbt_assert(type->element_count == 6*5*4,
    "element_count mismatch in test_some_array : %i / %i",
    type->element_count, 6*5*4);

  var = test_global_variable(process, process->binary_info.get(),
    "test_some_struct", &test_some_struct, sizeof(test_some_struct));
  i = process->binary_info->types.find(var->type_id);
  xbt_assert(i != process->binary_info->types.end(), "Missing type");
  type = &i->second;
  assert(find_member(process->binary_info.get(), "first", type)->offset() == 0);
  assert(find_member(process->binary_info.get(), "second", type)->offset()
      == ((const char*)&test_some_struct.second) - (const char*)&test_some_struct);

  unw_context_t context;
  unw_cursor_t cursor;
  unw_getcontext(&context);
  unw_init_local(&cursor, &context);

  test_local_variable(process->binary_info.get(), "main", "argc", &argc, &cursor);

  {
    int lexical_block_variable = 50;
    test_local_variable(process->binary_info.get(), "main",
      "lexical_block_variable", &lexical_block_variable, &cursor);
  }

  s_foo my_foo;
  test_type_by_name(process, my_foo);

  _exit(0);
}
