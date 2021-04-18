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
  /* This is actually a map from int to aid_t. We could use std::map here, but looking up a value there costs
   * O(log(n)). For a vector, this costs O(1). We hence go with the vector.
   */
  std::vector<aid_t> rank_to_pid_map_;
  std::vector<int> pid_to_rank_map_;

  int refcount_ = 1; /* refcount_: start > 0 so that this group never gets freed */

  int incl(const std::vector<int>& ranks, MPI_Group* newgroup) const;
  int excl(const std::vector<bool>& excl_map, MPI_Group* newgroup) const;

public:
  Group() = default;
  explicit Group(int size) : rank_to_pid_map_(size, -1), pid_to_rank_map_(size, MPI_UNDEFINED) {}
  explicit Group(const Group* origin);

  void set_mapping(aid_t pid, int rank);
  int rank(aid_t pid) const;
  aid_t actor_pid(int rank) const;
  std::string name() const override {return std::string("MPI_Group");}
  void ref();
  static void unref(MPI_Group group);
  int size() const { return static_cast<int>(rank_to_pid_map_.size()); }
  int compare(const Group* group2) const;
  int incl(int n, const int* ranks, MPI_Group* newgroup) const;
  int excl(int n, const int* ranks, MPI_Group* newgroup) const;
  int group_union(const Group* group2, MPI_Group* newgroup) const;
  int intersection(const Group* group2, MPI_Group* newgroup) const;
  int difference(const Group* group2, MPI_Group* newgroup) const;
  int range_incl(int n, const int ranges[][3], MPI_Group* newgroup) const;
  int range_excl(int n, const int ranges[][3], MPI_Group* newgroup) const;

  static Group* f2c(int id);
};
}
}

#endif
