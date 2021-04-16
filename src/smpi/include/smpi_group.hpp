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
  /* This is actually a map from int to s4u::Actor*. We could use std::map here, but looking up a value there costs
   * O(log(n)). For a vector, this costs O(1). We hence go with the vector.
   */
  std::vector<s4u::Actor*> rank_to_actor_map_;
  std::map<s4u::Actor*, int> actor_to_rank_map_;
  std::vector<int> index_to_rank_map_;

  int refcount_ = 1; /* refcount_: start > 0 so that this group never gets freed */

public:
  Group() = default;
  explicit Group(int size) : rank_to_actor_map_(size, nullptr), index_to_rank_map_(size, MPI_UNDEFINED) {}
  explicit Group(const Group* origin);

  void set_mapping(s4u::Actor* actor, int rank);
  int rank(int index) const;
  s4u::Actor* actor(int rank) const;
  std::string name() const override {return std::string("MPI_Group");}
  int rank(s4u::Actor* process) const;
  void ref();
  static void unref(MPI_Group group);
  int size() const { return static_cast<int>(rank_to_actor_map_.size()); }
  int compare(MPI_Group group2) const;
  int incl(int n, const int* ranks, MPI_Group* newgroup) const;
  int excl(int n, const int* ranks, MPI_Group* newgroup) const;
  int group_union(MPI_Group group2, MPI_Group* newgroup) const;
  int intersection(MPI_Group group2, MPI_Group* newgroup) const;
  int difference(MPI_Group group2, MPI_Group* newgroup) const;
  int range_incl(int n, int ranges[][3], MPI_Group* newgroup) const;
  int range_excl(int n, int ranges[][3], MPI_Group* newgroup) const;

  static Group* f2c(int id);
};
}
}

#endif
