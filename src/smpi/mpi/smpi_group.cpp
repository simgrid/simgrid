/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Actor.hpp"
#include "smpi_group.hpp"
#include "smpi_comm.hpp"
#include <string>

simgrid::smpi::Group smpi_MPI_GROUP_EMPTY;

namespace simgrid{
namespace smpi{

Group::Group(const Group* origin)
{
  if (origin != MPI_GROUP_NULL && origin != MPI_GROUP_EMPTY) {
    rank_to_pid_map_ = origin->rank_to_pid_map_;
    pid_to_rank_map_ = origin->pid_to_rank_map_;
  }
}

void Group::set_mapping(aid_t pid, int rank)
{
  if (0 <= rank && rank < size()) {
    if (static_cast<size_t>(pid) >= pid_to_rank_map_.size())
      pid_to_rank_map_.resize(pid + 1, MPI_UNDEFINED);
    rank_to_pid_map_[rank] = pid;
    pid_to_rank_map_[pid]  = rank;
  }
}

int Group::rank(aid_t pid) const
{
  int res = static_cast<size_t>(pid) < pid_to_rank_map_.size() ? pid_to_rank_map_[pid] : MPI_UNDEFINED;
  if (res == MPI_UNDEFINED) {
    // I'm not in the communicator ... but maybe my parent is?
    if (auto parent = s4u::Actor::by_pid(pid)) {
      aid_t ppid = parent->get_ppid();
      res        = static_cast<size_t>(ppid) < pid_to_rank_map_.size() ? pid_to_rank_map_[ppid] : MPI_UNDEFINED;
    }
  }
  return res;
}

aid_t Group::actor_pid(int rank) const
{
  return (0 <= rank && rank < size()) ? rank_to_pid_map_[rank] : -1;
}

void Group::ref()
{
  refcount_++;
}

void Group::unref(Group* group)
{
  group->refcount_--;
  if (group->refcount_ <= 0) {
    if (simgrid::smpi::F2C::lookup() != nullptr)
      F2C::free_f(group->f2c_id());
    delete group;
  }
}

int Group::compare(const Group* group2) const
{
  int result;

  result = MPI_IDENT;
  if (size() != group2->size()) {
    result = MPI_UNEQUAL;
  } else {
    for (int i = 0; i < size(); i++) {
      int rank = group2->rank(actor_pid(i));
      if (rank == MPI_UNDEFINED) {
        result = MPI_UNEQUAL;
        break;
      }
      if (rank != i) {
        result = MPI_SIMILAR;
      }
    }
  }
  return result;
}

int Group::incl(int n, const int* ranks, MPI_Group* newgroup) const
{
  if (n == 0) {
    *newgroup = MPI_GROUP_EMPTY;
    return MPI_SUCCESS;
  }

  *newgroup = new Group(n);
  for (int i = 0; i < n; i++) {
    aid_t actor = this->actor_pid(ranks[i]);
    (*newgroup)->set_mapping(actor, i);
  }
  (*newgroup)->add_f();
  return MPI_SUCCESS;
}

int Group::incl(const std::vector<int>& ranks, MPI_Group* newgroup) const
{
  return incl(static_cast<int>(ranks.size()), ranks.data(), newgroup);
}

int Group::excl(const std::vector<bool>& excl_map, MPI_Group* newgroup) const
{
  xbt_assert(static_cast<int>(excl_map.size()) == size());
  std::vector<int> ranks;
  for (int i = 0; i < static_cast<int>(excl_map.size()); i++)
    if (not excl_map[i])
      ranks.push_back(i);
  return incl(static_cast<int>(ranks.size()), ranks.data(), newgroup);
}

int Group::group_union(const Group* group2, MPI_Group* newgroup) const
{
  std::vector<int> ranks2;
  for (int i = 0; i < group2->size(); i++) {
    aid_t actor = group2->actor_pid(i);
    if (rank(actor) == MPI_UNDEFINED)
      ranks2.push_back(i);
  }

  int newsize = size() + static_cast<int>(ranks2.size());
  if (newsize == 0) {
    *newgroup = MPI_GROUP_EMPTY;
    return MPI_SUCCESS;
  }

  *newgroup = new Group(newsize);
  int i;
  for (i = 0; i < size(); i++) {
    aid_t actor1 = actor_pid(i);
    (*newgroup)->set_mapping(actor1, i);
  }
  for (int j : ranks2) {
    aid_t actor2 = group2->actor_pid(j);
    (*newgroup)->set_mapping(actor2, i);
    i++;
  }
  (*newgroup)->add_f();
  return MPI_SUCCESS;
}

int Group::intersection(const Group* group2, MPI_Group* newgroup) const
{
  std::vector<int> ranks2;
  for (int i = 0; i < group2->size(); i++) {
    aid_t actor = group2->actor_pid(i);
    if (rank(actor) != MPI_UNDEFINED)
      ranks2.push_back(i);
  }
  return group2->incl(ranks2, newgroup);
}

int Group::difference(const Group* group2, MPI_Group* newgroup) const
{
  std::vector<int> ranks;
  for (int i = 0; i < size(); i++) {
    aid_t actor = this->actor_pid(i);
    if (group2->rank(actor) == MPI_UNDEFINED)
      ranks.push_back(i);
  }
  return this->incl(ranks, newgroup);
}

int Group::excl(int n, const int* ranks, MPI_Group* newgroup) const
{
  std::vector<bool> to_excl(size(), false);
  for (int i = 0; i < n; i++)
    to_excl[ranks[i]] = true;
  return this->excl(to_excl, newgroup);
}

static bool is_rank_in_range(int rank, int first, int last)
{
  return (first <= rank && rank <= last) || (first >= rank && rank >= last);
}

int Group::range_incl(int n, const int ranges[][3], MPI_Group* newgroup) const
{
  std::vector<int> ranks;
  for (int i = 0; i < n; i++) {
    for (int j = ranges[i][0]; j >= 0 && j < size() && is_rank_in_range(j, ranges[i][0], ranges[i][1]);
         j += ranges[i][2])
      ranks.push_back(j);
  }
  return this->incl(ranks, newgroup);
}

int Group::range_excl(int n, const int ranges[][3], MPI_Group* newgroup) const
{
  std::vector<bool> to_excl(size(), false);
  for (int i = 0; i < n; i++) {
    for (int j = ranges[i][0]; j >= 0 && j < size() && is_rank_in_range(j, ranges[i][0], ranges[i][1]);
         j += ranges[i][2])
      to_excl[j] = true;
  }
  return this->excl(to_excl, newgroup);
}

MPI_Group Group::f2c(int id) {
  if(id == -2) {
    return MPI_GROUP_EMPTY;
  } else if (F2C::lookup() != nullptr && id >= 0) {
    return static_cast<MPI_Group>(F2C::lookup()->at(id));
  } else {
    return MPI_GROUP_NULL;
  }
}

}
}
