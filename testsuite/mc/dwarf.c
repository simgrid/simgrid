#ifdef NDEBUG
#undef NDEBUG
#endif

#include <string.h>
#include <assert.h>

#include <xbt.h>
#include <mc/mc.h>

#include "../../src/include/mc/datatypes.h"
#include "../../src/mc/mc_private.h"

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

static dw_variable_t find_global_variable_by_name(mc_object_info_t info, const char* name) {
  unsigned int cursor = 0;
  dw_variable_t variable;
  xbt_dynar_foreach(info->global_variables, cursor, variable){
    if(!strcmp(name, variable->name))
      return variable;
  }

  return NULL;
}

static dw_variable_t test_global_variable(mc_object_info_t info, const char* name, void* address, long byte_size) {
  dw_variable_t variable = find_global_variable_by_name(info, name);
  xbt_assert(variable, "Global variable %s was not found", name);
  xbt_assert(!strcmp(variable->name, name), "Name mismatch for %s", name);
  xbt_assert(variable->global, "Variable %s is not global", name);
  xbt_assert(variable->address == address,
      "Address mismatch for %s : %p expected but %p found", name, address, variable->address);

  dw_type_t type = xbt_dict_get_or_null(mc_binary_info->types, variable->type_origin);
  xbt_assert(type!=NULL, "Missing type for %s", name);
  xbt_assert(type->byte_size = byte_size, "Byte size mismatch for %s", name);
  return variable;
}

static dw_type_t find_type(mc_object_info_t info, const char* name, dw_type_t type) {
  unsigned int cursor = 0;
  dw_type_t member;
  xbt_dynar_foreach(type->members, cursor, member){
    if(!strcmp(name,member->name))
      return member;
  }
  return NULL;
}

int some_local_variable = 0;

int main(int argc, char** argv) {

  // xbt_init(&argc, argv);
  SIMIX_global_init(&argc, argv);
  MC_memory_init();
  MC_init();

  dw_variable_t var;
  dw_type_t type;

  var = test_global_variable(mc_binary_info, "some_local_variable", &some_local_variable, sizeof(int));

  var = test_global_variable(mc_binary_info, "test_some_array", &test_some_array, sizeof(test_some_array));
  type = xbt_dict_get_or_null(mc_binary_info->types, var->type_origin);
  xbt_assert(type->element_count == 6*5*4, "element_count mismatch in test_some_array : %i / %i", type->element_count, 6*5*4);

  var = test_global_variable(mc_binary_info, "test_some_struct", &test_some_struct, sizeof(test_some_struct));
  type = xbt_dict_get_or_null(mc_binary_info->types, var->type_origin);
  assert(find_type(mc_binary_info, "first", type)->offset == 0);
  assert(find_type(mc_binary_info, "second", type)->offset
      == ((const char*)&test_some_struct.second) - (const char*)&test_some_struct);

  _exit(0);
}
