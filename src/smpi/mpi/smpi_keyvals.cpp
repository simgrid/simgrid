/* Copyright (c) 2007-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

//#include "private.hpp"
#include "smpi_keyvals.hpp"

namespace simgrid{
namespace smpi{

template <> int Keyval::call_deleter<Comm>(Comm* obj, const smpi_key_elem& elem, int keyval, void* value, int* /*flag*/)
{
  int ret = MPI_SUCCESS;
  if (elem.delete_fn.comm_delete_fn != MPI_NULL_DELETE_FN)
    ret = elem.delete_fn.comm_delete_fn(obj, keyval, value, elem.extra_state);
  else if (elem.delete_fn.comm_delete_fn_fort != MPI_NULL_DELETE_FN)
    elem.delete_fn.comm_delete_fn_fort(obj, keyval, value, elem.extra_state, &ret);
  return ret;
}

template <> int Keyval::call_deleter<Win>(Win* obj, const smpi_key_elem& elem, int keyval, void* value, int* /*flag*/)
{
  int ret = MPI_SUCCESS;
  if (elem.delete_fn.win_delete_fn != MPI_NULL_DELETE_FN)
    ret = elem.delete_fn.win_delete_fn(obj, keyval, value, elem.extra_state);
  else if (elem.delete_fn.win_delete_fn_fort != MPI_NULL_DELETE_FN)
    elem.delete_fn.win_delete_fn_fort(obj, keyval, value, elem.extra_state, &ret);
  return ret;
}

template <>
int Keyval::call_deleter<Datatype>(Datatype* obj, const smpi_key_elem& elem, int keyval, void* value, int* /*flag*/)
{
  int ret = MPI_SUCCESS;
  if (elem.delete_fn.type_delete_fn != MPI_NULL_DELETE_FN)
    ret = elem.delete_fn.type_delete_fn(obj, keyval, value, elem.extra_state);
  else if (elem.delete_fn.type_delete_fn_fort != MPI_NULL_DELETE_FN)
    elem.delete_fn.type_delete_fn_fort(obj, keyval, value, elem.extra_state, &ret);
  return ret;
}

}
}
