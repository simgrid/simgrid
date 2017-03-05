/* Copyright (c) 2010, 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "smpi_group.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_group, smpi, "Logging specific to SMPI (group)");
simgrid::SMPI::Group mpi_MPI_GROUP_EMPTY;
MPI_Group MPI_GROUP_EMPTY=&mpi_MPI_GROUP_EMPTY;

namespace simgrid{
namespace SMPI{

Group::Group()
{
  m_size=0;                            /* size */
  m_rank_to_index_map=nullptr;                         /* m_rank_to_index_map */
  m_index_to_rank_map=nullptr;                         /* m_index_to_rank_map */
  m_refcount=1;                            /* m_refcount: start > 0 so that this group never gets freed */
}

Group::Group(int n)
{
  int i;

  m_size = n;
  m_rank_to_index_map = xbt_new(int, m_size);
  m_index_to_rank_map = xbt_dict_new_homogeneous(xbt_free_f);
  m_refcount = 1;
  for (i = 0; i < m_size; i++) {
    m_rank_to_index_map[i] = MPI_UNDEFINED;
  }
}

Group::Group(MPI_Group origin)
{
  char *key;
  char *ptr_rank;
  xbt_dict_cursor_t cursor = nullptr;
  
  int i;
  if(origin != MPI_GROUP_NULL
            && origin != MPI_GROUP_EMPTY)
    {
      m_size = origin->size();
      m_rank_to_index_map = xbt_new(int, m_size);
      m_index_to_rank_map = xbt_dict_new_homogeneous(xbt_free_f);
      m_refcount = 1;
      for (i = 0; i < m_size; i++) {
        m_rank_to_index_map[i] = origin->m_rank_to_index_map[i];
      }

      xbt_dict_foreach(origin->m_index_to_rank_map, cursor, key, ptr_rank) {
        int * cp = static_cast<int*>(xbt_malloc(sizeof(int)));
        *cp=*reinterpret_cast<int*>(ptr_rank);
        xbt_dict_set(m_index_to_rank_map, key, cp, nullptr);
      }
    }
}

Group::~Group()
{
  xbt_free(m_rank_to_index_map);
  xbt_dict_free(&m_index_to_rank_map);
}

void Group::destroy()
{
  if(this != smpi_comm_group(MPI_COMM_WORLD)
          && this != MPI_GROUP_NULL
          && this != MPI_GROUP_EMPTY)
  this->unuse();
}

void Group::set_mapping(int index, int rank)
{
  int * val_rank;

  if (rank < m_size) {
    m_rank_to_index_map[rank] = index;
    if (index!=MPI_UNDEFINED ) {
      val_rank = static_cast<int *>(xbt_malloc(sizeof(int)));
      *val_rank = rank;

      char * key = bprintf("%d", index);
      xbt_dict_set(m_index_to_rank_map, key, val_rank, nullptr);
      xbt_free(key);
    }
  }
}

int Group::index(int rank)
{
  int index = MPI_UNDEFINED;

  if (0 <= rank && rank < m_size) {
    index = m_rank_to_index_map[rank];
  }
  return index;
}

int Group::rank(int index)
{
  int * ptr_rank = nullptr;
  if (this==MPI_GROUP_EMPTY)
    return MPI_UNDEFINED;
  char * key = bprintf("%d", index);
  ptr_rank = static_cast<int*>(xbt_dict_get_or_null(m_index_to_rank_map, key));
  xbt_free(key);

  if (ptr_rank==nullptr)
    return MPI_UNDEFINED;
  return *ptr_rank;
}

int Group::use()
{
  m_refcount++;
  return m_refcount;
}

int Group::unuse()
{
  m_refcount--;
  if (m_refcount <= 0) {
    delete this;
    return 0;
  }
  return m_refcount;
}

int Group::size()
{
  return m_size;
}

int Group::compare(MPI_Group group2)
{
  int result;
  int i, index, rank, sz;

  result = MPI_IDENT;
  if (m_size != group2->size()) {
    result = MPI_UNEQUAL;
  } else {
    sz = group2->size();
    for (i = 0; i < sz; i++) {
      index = this->index(i);
      rank = group2->rank(index);
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
  int i=0, index=0;
  if (n == 0) {
    *newgroup = MPI_GROUP_EMPTY;
  } else if (n == m_size) {
    *newgroup = this;
    if(this!= smpi_comm_group(MPI_COMM_WORLD)
              && this != MPI_GROUP_NULL
              && this != smpi_comm_group(MPI_COMM_SELF)
              && this != MPI_GROUP_EMPTY)
    this->use();
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
  int size1 = m_size;
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
    *newgroup = new simgrid::SMPI::Group(size1);
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
    *newgroup = new simgrid::SMPI::Group(size2);
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
  int newsize = m_size;
  int size2 = m_size;
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
    *newgroup = new simgrid::SMPI::Group(newsize);
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
  int oldsize = m_size;
  int newsize = oldsize - n;
  *newgroup = new simgrid::SMPI::Group(newsize);
  int* to_exclude=xbt_new0(int, m_size);
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
         rank >= 0 && rank < m_size; /* Last */
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
  *newgroup = new simgrid::SMPI::Group(newsize);
  int j     = 0;
  for (int i = 0; i < n; i++) {
    for (int rank = ranges[i][0];                    /* First */
         rank >= 0 && rank < m_size; /* Last */
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
  int newsize = m_size;
  for (int i = 0; i < n; i++) {
    for (int rank = ranges[i][0];                    /* First */
         rank >= 0 && rank < m_size; /* Last */
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
    *newgroup = new simgrid::SMPI::Group(newsize);
    int newrank = 0;
    int oldrank = 0;
    while (newrank < newsize) {
      int add = 1;
      for (int i = 0; i < n; i++) {
        for (int rank = ranges[i][0]; rank >= 0 && rank < m_size;) {
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

}
}
