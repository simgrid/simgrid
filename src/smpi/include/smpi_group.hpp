/* Copyright (c) 2010-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_GROUP_HPP_INCLUDED
#define SMPI_GROUP_HPP_INCLUDED

#include "smpi_f2c.hpp"
#include <smpi/smpi.h>
#include <map>
#include <vector>

namespace simgrid{
namespace smpi{

class Group : public F2C{
  int size_ = 0;
  /* This is actually a map from int to int. We could use std::map here, but looking up a value there costs O(log(n)).
   * For a vector, this costs O(1). We hence go with the vector.
   */
  std::vector<s4u::Actor*> rank_to_actor_map_;
  std::map<s4u::Actor*, int> actor_to_rank_map_;
  std::vector<int> index_to_rank_map_;

  int refcount_ = 1; /* refcount_: start > 0 so that this group never gets freed */

public:
  Group() = default;
  explicit Group(int size) : size_(size), rank_to_actor_map_(size, nullptr), index_to_rank_map_(size, MPI_UNDEFINED) {}
  explicit Group(const Group* origin);

  void set_mapping(s4u::Actor* actor, int rank);
  int rank(int index);
  s4u::Actor* actor(int rank);
  int rank(s4u::Actor* process);
  void ref();
  static void unref(MPI_Group group);
  int size() const { return size_; }
  int compare(MPI_Group group2);
  int incl(int n, const int* ranks, MPI_Group* newgroup);
  int excl(int n, const int* ranks, MPI_Group* newgroup);
  int group_union(MPI_Group group2, MPI_Group* newgroup);
  int intersection(MPI_Group group2, MPI_Group* newgroup);
  int difference(MPI_Group group2, MPI_Group* newgroup);
  int range_incl(int n, int ranges[][3], MPI_Group* newgroup);
  int range_excl(int n, int ranges[][3], MPI_Group* newgroup);

  static Group* f2c(int id);
};
}
}

#endif
