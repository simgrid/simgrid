/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smpi_comm.hpp"
#include "smpi_group.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_group, smpi, "Logging specific to SMPI (group)");

simgrid::smpi::Group mpi_MPI_GROUP_EMPTY;
MPI_Group MPI_GROUP_EMPTY=&mpi_MPI_GROUP_EMPTY;

namespace simgrid{
namespace smpi{

Group::Group()
{
  size_              = 0;       /* size */
  rank_to_index_map_ = nullptr; /* rank_to_index_map_ */
  refcount_          = 1;       /* refcount_: start > 0 so that this group never gets freed */
}

Group::Group(int n) : size_(n)
{
  rank_to_index_map_ = new int[size_];
  refcount_ = 1;
  for (int i              = 0; i < size_; i++)
    rank_to_index_map_[i] = MPI_UNDEFINED;
}

Group::Group(MPI_Group origin)
{
  if (origin != MPI_GROUP_NULL && origin != MPI_GROUP_EMPTY) {
    size_              = origin->size();
    rank_to_index_map_ = new int[size_];
    refcount_          = 1;
    for (int i = 0; i < size_; i++) {
      rank_to_index_map_[i] = origin->rank_to_index_map_[i];
    }

    for (auto const& elm : origin->index_to_rank_map_) {
      index_to_rank_map_.insert({elm.first, elm.second});
    }
  }
}

Group::~Group()
{
  delete[] rank_to_index_map_;
}

void Group::set_mapping(int index, int rank)
{
  if (rank < size_) {
    rank_to_index_map_[rank] = index;
    if (index != MPI_UNDEFINED)
      index_to_rank_map_.insert({index, rank});
  }
}

int Group::index(int rank)
{
  int index = MPI_UNDEFINED;

  if (0 <= rank && rank < size_) {
    index = rank_to_index_map_[rank];
  }
  return index;
}

int Group::rank(int index)
{
  if (this == MPI_GROUP_EMPTY)
    return MPI_UNDEFINED;
  auto rank = index_to_rank_map_.find(index);
  return rank == index_to_rank_map_.end() ? MPI_UNDEFINED : rank->second;
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

int Group::size()
{
  return size_;
}

int Group::compare(MPI_Group group2)
{
  int result;

  result = MPI_IDENT;
  if (size_ != group2->size()) {
    result = MPI_UNEQUAL;
  } else {
    int sz = group2->size();
    for (int i = 0; i < sz; i++) {
      int index = this->index(i);
      int rank = group2->rank(index);
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

int Group::incl(int n, int* ranks, MPI_Group* newgroup)
{
  int i=0;
  int index=0;
  if (n == 0) {
    *newgroup = MPI_GROUP_EMPTY;
  } else if (n == size_) {
    *newgroup = this;
    if (this != MPI_COMM_WORLD->group() && this != MPI_COMM_SELF->group() && this != MPI_GROUP_EMPTY)
      this->ref();
  } else {
    *newgroup = new Group(n);
    for (i = 0; i < n; i++) {
      index = this->index(ranks[i]);
      (*newgroup)->set_mapping(index, i);
    }
  }
  return MPI_SUCCESS;
}

int Group::group_union(MPI_Group group2, MPI_Group* newgroup)
{
  int size1 = size_;
  int size2 = group2->size();
  for (int i = 0; i < size2; i++) {
    int proc2 = group2->index(i);
    int proc1 = this->rank(proc2);
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
      int proc1 = this->index(i);
      (*newgroup)->set_mapping(proc1, i);
    }
    for (int i = size2; i < size1; i++) {
      int proc2 = group2->index(i - size2);
      (*newgroup)->set_mapping(proc2, i);
    }
  }
  return MPI_SUCCESS;
}

int Group::intersection(MPI_Group group2, MPI_Group* newgroup)
{
  int size2 = group2->size();
  for (int i = 0; i < size2; i++) {
    int proc2 = group2->index(i);
    int proc1 = this->rank(proc2);
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
      int proc2 = group2->index(i);
      int proc1 = this->rank(proc2);
      if (proc1 != MPI_UNDEFINED) {
        (*newgroup)->set_mapping(proc2, j);
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
    int proc1 = this->index(i);
    int proc2 = group2->rank(proc1);
    if (proc2 != MPI_UNDEFINED) {
      newsize--;
    }
  }
  if (newsize == 0) {
    *newgroup = MPI_GROUP_EMPTY;
  } else {
    *newgroup = new  Group(newsize);
    for (int i = 0; i < size2; i++) {
      int proc1 = this->index(i);
      int proc2 = group2->rank(proc1);
      if (proc2 == MPI_UNDEFINED) {
        (*newgroup)->set_mapping(proc1, i);
      }
    }
  }
  return MPI_SUCCESS;
}

int Group::excl(int n, int *ranks, MPI_Group * newgroup){
  int oldsize = size_;
  int newsize = oldsize - n;
  *newgroup = new  Group(newsize);
  int* to_exclude=xbt_new0(int, size_);
  for (int i     = 0; i < oldsize; i++)
    to_exclude[i]=0;
  for (int i            = 0; i < n; i++)
    to_exclude[ranks[i]]=1;
  int j = 0;
  for (int i = 0; i < oldsize; i++) {
    if(to_exclude[i]==0){
      int index = this->index(i);
      (*newgroup)->set_mapping(index, j);
      j++;
    }
  }
  xbt_free(to_exclude);
  return MPI_SUCCESS;

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
      if (ranges[i][0] < ranges[i][1]) {
        if (rank > ranges[i][1])
          break;
      } else {
        if (rank < ranges[i][1])
          break;
      }
    }
  }
  *newgroup = new  Group(newsize);
  int j     = 0;
  for (int i = 0; i < n; i++) {
    for (int rank = ranges[i][0];                    /* First */
         rank >= 0 && rank < size_; /* Last */
         ) {
      int index = this->index(rank);
      (*newgroup)->set_mapping(index, j);
      j++;
      if(rank == ranges[i][1]){/*already last ?*/
        break;
      }
      rank += ranges[i][2]; /* Stride */
      if (ranges[i][0] < ranges[i][1]) {
        if (rank > ranges[i][1])
          break;
      } else {
        if (rank < ranges[i][1])
          break;
      }
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
      if (ranges[i][0] < ranges[i][1]) {
        if (rank > ranges[i][1])
          break;
      } else {
        if (rank < ranges[i][1])
          break;
      }
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
          if (ranges[i][0]<ranges[i][1]){
            if (rank > ranges[i][1])
              break;
          }else{
            if (rank < ranges[i][1])
              break;
          }
        }
      }
      if(add==1){
        int index = this->index(oldrank);
        (*newgroup)->set_mapping(index, newrank);
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
    char key[KEY_SIZE];
    return static_cast<MPI_Group>(F2C::f2c_lookup()->at(get_key(key, id)));
  } else {
    return static_cast<MPI_Group>(MPI_GROUP_NULL);
  }
}

}
}
