/* Copyright (c) 2014-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <mc/mc.h>
#include <simgrid/s4u/Engine.hpp>

#include "mc/datatypes.h"
#include "src/mc/mc_private.hpp"

#include "src/mc/inspect/ObjectInformation.hpp"
#include "src/mc/inspect/Type.hpp"
#include "src/mc/inspect/Variable.hpp"
#include "src/mc/remote/RemoteProcess.hpp"

#include <cassert>
#include <cstring>

#include <elfutils/version.h>
#if _ELFUTILS_VERSION < 171
// Elder elfutils/libdw broken with multi-dimensional arrays. See https://sourceware.org/bugzilla/show_bug.cgi?id=22546
int test_some_array[4 * 5 * 6];
#else
int test_some_array[4][5][6];
#endif

struct some_struct {
  int first;
  int second[4][5];
};
some_struct test_some_struct;

static simgrid::mc::Frame* find_function_by_name(
    simgrid::mc::ObjectInformation* info, const char* name)
{
  for (auto& entry : info->subprograms)
    if(entry.second.name == name)
      return &entry.second;
  return nullptr;
}

static simgrid::mc::Variable* find_local_variable(
    simgrid::mc::Frame* frame, const char* argument_name)
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

static void test_local_variable(simgrid::mc::ObjectInformation* info, const char* function, const char* variable,
                                const void* address, unw_cursor_t* cursor)
{
  simgrid::mc::Frame* subprogram = find_function_by_name(info, function);
  assert(subprogram);
  // TODO, Lookup frame by IP and test against name instead

  const simgrid::mc::Variable* var = find_local_variable(subprogram, variable);
  assert(var);

  void* frame_base = subprogram->frame_base(*cursor);
  simgrid::dwarf::Location location = simgrid::dwarf::resolve(var->location_list, info, cursor, frame_base, nullptr);

  xbt_assert(location.in_memory(), "Expected the variable %s of function %s to be in memory", variable, function);
  xbt_assert(location.address() == address, "Bad resolution of local variable %s of %s", variable, function);
}

static const simgrid::mc::Variable* test_global_variable(const simgrid::mc::RemoteProcess& process,
                                                         simgrid::mc::ObjectInformation* info, const char* name,
                                                         void* address, long byte_size)
{
  const simgrid::mc::Variable* variable = info->find_variable(name);
  xbt_assert(variable, "Global variable %s was not found", name);
  xbt_assert(variable->name == name, "Name mismatch for %s", name);
  xbt_assert(variable->global, "Variable %s is not global", name);
  xbt_assert(variable->address == address, "Address mismatch for %s : %p expected but %p found", name, address,
             variable->address);

  auto i = process.binary_info->types.find(variable->type_id);
  xbt_assert(i != process.binary_info->types.end(), "Missing type for %s", name);
  const simgrid::mc::Type* type = &i->second;
  xbt_assert(type->byte_size == byte_size, "Byte size mismatch for %s", name);
  return variable;
}

static simgrid::mc::Member* find_member(simgrid::mc::Type& type, const char* name)
{
  for (simgrid::mc::Member& member : type.members)
    if(member.name == name)
      return &member;
  return nullptr;
}

int some_local_variable = 0;

struct s_foo {
  int i;
};

static void test_type_by_name(const simgrid::mc::RemoteProcess& process, s_foo /*my_foo*/)
{
  assert(process.binary_info->full_types_by_name.find("struct s_foo") != process.binary_info->full_types_by_name.end());
}

int main(int argc, char** argv)
{
  simgrid::s4u::Engine::get_instance(&argc, argv);

  const simgrid::mc::Variable* var;
  simgrid::mc::Type* type;

  simgrid::mc::RemoteProcess process(getpid());
  process.init(nullptr, nullptr, nullptr);

  test_global_variable(process, process.binary_info.get(), "some_local_variable", &some_local_variable, sizeof(int));

  var = test_global_variable(process, process.binary_info.get(), "test_some_array", &test_some_array,
                             sizeof(test_some_array));
  auto i = process.binary_info->types.find(var->type_id);
  xbt_assert(i != process.binary_info->types.end(), "Missing type");
  type = &i->second;
  xbt_assert(type->element_count == 6 * 5 * 4, "element_count mismatch in test_some_array : %i / %i",
             type->element_count, 6 * 5 * 4);

  var = test_global_variable(process, process.binary_info.get(), "test_some_struct", &test_some_struct,
                             sizeof(test_some_struct));
  i = process.binary_info->types.find(var->type_id);
  xbt_assert(i != process.binary_info->types.end(), "Missing type");
  type = &i->second;

  assert(type);
  assert(find_member(*type, "first")->offset() == 0);
  assert(find_member(*type, "second")->offset() ==
         ((const char*)&test_some_struct.second) - (const char*)&test_some_struct);

  unw_context_t context;
  unw_cursor_t cursor;
  unw_getcontext(&context);
  unw_init_local(&cursor, &context);

  test_local_variable(process.binary_info.get(), "main", "argc", &argc, &cursor);

  int lexical_block_variable = 50;
  test_local_variable(process.binary_info.get(), "main", "lexical_block_variable", &lexical_block_variable, &cursor);

  s_foo my_foo = {0};
  test_type_by_name(process, my_foo);

  return 0;
}
