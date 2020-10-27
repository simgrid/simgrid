/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Actor.hpp"
#include "smpi_group.hpp"
#include "smpi_comm.hpp"
#include <string>

simgrid::smpi::Group mpi_MPI_GROUP_EMPTY;
MPI_Group MPI_GROUP_EMPTY=&mpi_MPI_GROUP_EMPTY;

namespace simgrid{
namespace smpi{

Group::Group(const Group* origin)
{
  if (origin != MPI_GROUP_NULL && origin != MPI_GROUP_EMPTY) {
    size_              = origin->size();
    // FIXME: cheinrich: There is no such thing as an index any more; the two maps should be removed
    index_to_rank_map_ = origin->index_to_rank_map_;
    rank_to_actor_map_ = origin->rank_to_actor_map_;
    actor_to_rank_map_ = origin->actor_to_rank_map_;
  }
}

void Group::set_mapping(s4u::Actor* actor, int rank)
{
  if (0 <= rank && rank < size_) {
    int index                = actor->get_pid();
    if (index != MPI_UNDEFINED) {
      if ((unsigned)index >= index_to_rank_map_.size())
        index_to_rank_map_.resize(index + 1, MPI_UNDEFINED);
      index_to_rank_map_[index] = rank;
    }

    rank_to_actor_map_[rank] = actor;
    actor_to_rank_map_.insert({actor, rank});
  }
}

int Group::rank(int index)
{
  int rank;
  if (0 <= index && (unsigned)index < index_to_rank_map_.size())
    rank = index_to_rank_map_[index];
  else
    rank = MPI_UNDEFINED;

  return rank;
}

s4u::Actor* Group::actor(int rank)
{
  if (0 <= rank && rank < size_)
    return rank_to_actor_map_[rank];
  else
    return nullptr;
}

int Group::rank(s4u::Actor* actor)
{
  auto iterator = actor_to_rank_map_.find(actor);
  //I'm not in the communicator ... but maybe my parent is ?
  if (iterator == actor_to_rank_map_.end())
    iterator = actor_to_rank_map_.find(s4u::Actor::by_pid(actor->get_ppid()).get());
  return (iterator == actor_to_rank_map_.end()) ? MPI_UNDEFINED : (*iterator).second;
}

void Group::ref()
{
  refcount_++;
}

void Group::unref(Group* group)
{
  group->refcount_--;
  if (group->refcount_ <= 0) {
    delete group;
  }
}

int Group::compare(MPI_Group group2)
{
  int result;

  result = MPI_IDENT;
  if (size_ != group2->size()) {
    result = MPI_UNEQUAL;
  } else {
    for (int i = 0; i < size_; i++) {
      s4u::Actor* actor = this->actor(i);
      int rank = group2->rank(actor);
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

int Group::incl(int n, const int* ranks, MPI_Group* newgroup)
{
  if (n == 0) {
    *newgroup = MPI_GROUP_EMPTY;
  } else if (n == size_) {
    *newgroup = this;
    if (this != MPI_COMM_WORLD->group() && this != MPI_COMM_SELF->group() && this != MPI_GROUP_EMPTY)
      this->ref();
  } else {
    *newgroup = new Group(n);
    for (int i = 0; i < n; i++) {
      s4u::Actor* actor = this->actor(ranks[i]); // ranks[] was passed as a param!
      (*newgroup)->set_mapping(actor, i);
    }
  }
  return MPI_SUCCESS;
}

int Group::group_union(MPI_Group group2, MPI_Group* newgroup)
{
  int size1 = size_;
  int size2 = group2->size();
  for (int i = 0; i < size2; i++) {
    s4u::Actor* actor = group2->actor(i);
    int proc1 = this->rank(actor);
    if (proc1 == MPI_UNDEFINED) {
      size1++;
    }
  }
  if (size1 == 0) {
    *newgroup = MPI_GROUP_EMPTY;
  } else {
    *newgroup = new  Group(size1);
    size2 = this->size();
    for (int i = 0; i < size2; i++) {
      s4u::Actor* actor1 = this->actor(i);
      (*newgroup)->set_mapping(actor1, i);
    }
    for (int i = size2; i < size1; i++) {
      s4u::Actor* actor = group2->actor(i - size2);
      (*newgroup)->set_mapping(actor, i);
    }
  }
  return MPI_SUCCESS;
}

int Group::intersection(MPI_Group group2, MPI_Group* newgroup)
{
  int size2 = group2->size();
  for (int i = 0; i < size2; i++) {
    s4u::Actor* actor = group2->actor(i);
    int proc1 = this->rank(actor);
    if (proc1 == MPI_UNDEFINED) {
      size2--;
    }
  }
  if (size2 == 0) {
    *newgroup = MPI_GROUP_EMPTY;
  } else {
    *newgroup = new  Group(size2);
    int j=0;
    for (int i = 0; i < group2->size(); i++) {
      s4u::Actor* actor = group2->actor(i);
      int proc1 = this->rank(actor);
      if (proc1 != MPI_UNDEFINED) {
        (*newgroup)->set_mapping(actor, j);
        j++;
      }
    }
  }
  return MPI_SUCCESS;
}

int Group::difference(MPI_Group group2, MPI_Group* newgroup)
{
  int newsize = size_;
  int size2 = size_;
  for (int i = 0; i < size2; i++) {
    s4u::Actor* actor = this->actor(i);
    int proc2 = group2->rank(actor);
    if (proc2 != MPI_UNDEFINED) {
      newsize--;
    }
  }
  if (newsize == 0) {
    *newgroup = MPI_GROUP_EMPTY;
  } else {
    *newgroup = new  Group(newsize);
    for (int i = 0; i < size2; i++) {
      s4u::Actor* actor = this->actor(i);
      int proc2 = group2->rank(actor);
      if (proc2 == MPI_UNDEFINED) {
        (*newgroup)->set_mapping(actor, i);
      }
    }
  }
  return MPI_SUCCESS;
}

int Group::excl(int n, const int *ranks, MPI_Group * newgroup){
  int oldsize = size_;
  int newsize = oldsize - n;
  *newgroup = new  Group(newsize);
  auto* to_exclude = new int[size_];
  for (int i     = 0; i < oldsize; i++)
    to_exclude[i]=0;
  for (int i            = 0; i < n; i++)
    to_exclude[ranks[i]]=1;
  int j = 0;
  for (int i = 0; i < oldsize; i++) {
    if(to_exclude[i]==0){
      s4u::Actor* actor = this->actor(i);
      (*newgroup)->set_mapping(actor, j);
      j++;
    }
  }
  delete[] to_exclude;
  return MPI_SUCCESS;
}

static bool is_rank_in_range(int rank, int first, int last)
{
  if (first < last)
    return rank <= last;
  else
    return rank >= last;
}

int Group::range_incl(int n, int ranges[][3], MPI_Group * newgroup){
  int newsize = 0;
  for (int i = 0; i < n; i++) {
    for (int rank = ranges[i][0];                    /* First */
         rank >= 0 && rank < size_; /* Last */
         ) {
      newsize++;
      if(rank == ranges[i][1]){/*already last ?*/
        break;
      }
      rank += ranges[i][2]; /* Stride */
      if (not is_rank_in_range(rank, ranges[i][0], ranges[i][1]))
        break;
    }
  }
  *newgroup = new  Group(newsize);
  int j     = 0;
  for (int i = 0; i < n; i++) {
    for (int rank = ranges[i][0];                    /* First */
         rank >= 0 && rank < size_; /* Last */
         ) {
      s4u::Actor* actor = this->actor(rank);
      (*newgroup)->set_mapping(actor, j);
      j++;
      if(rank == ranges[i][1]){/*already last ?*/
        break;
      }
      rank += ranges[i][2]; /* Stride */
      if (not is_rank_in_range(rank, ranges[i][0], ranges[i][1]))
        break;
    }
  }
  return MPI_SUCCESS;
}

int Group::range_excl(int n, int ranges[][3], MPI_Group * newgroup){
  int newsize = size_;
  for (int i = 0; i < n; i++) {
    for (int rank = ranges[i][0];                    /* First */
         rank >= 0 && rank < size_; /* Last */
         ) {
      newsize--;
      if(rank == ranges[i][1]){/*already last ?*/
        break;
      }
      rank += ranges[i][2]; /* Stride */
      if (not is_rank_in_range(rank, ranges[i][0], ranges[i][1]))
        break;
    }
  }
  if (newsize == 0) {
    *newgroup = MPI_GROUP_EMPTY;
  } else {
    *newgroup = new  Group(newsize);
    int newrank = 0;
    int oldrank = 0;
    while (newrank < newsize) {
      int add = 1;
      for (int i = 0; i < n; i++) {
        for (int rank = ranges[i][0]; rank >= 0 && rank < size_;) {
          if(rank==oldrank){
            add = 0;
            break;
          }
          if(rank == ranges[i][1]){/*already last ?*/
            break;
          }
          rank += ranges[i][2]; /* Stride */
          if (not is_rank_in_range(rank, ranges[i][0], ranges[i][1]))
            break;
        }
      }
      if(add==1){
        s4u::Actor* actor = this->actor(oldrank);
        (*newgroup)->set_mapping(actor, newrank);
        newrank++;
      }
      oldrank++;
    }
  }
  return MPI_SUCCESS;
}

MPI_Group Group::f2c(int id) {
  if(id == -2) {
    return MPI_GROUP_EMPTY;
  } else if(F2C::f2c_lookup() != nullptr && id >= 0) {
    return static_cast<MPI_Group>(F2C::f2c_lookup()->at(get_key(id)));
  } else {
    return MPI_GROUP_NULL;
  }
}

}
}
