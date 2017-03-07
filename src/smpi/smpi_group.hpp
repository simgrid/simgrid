/* Copyright (c) 2010, 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_GROUP_HPP_INCLUDED
#define SMPI_GROUP_HPP_INCLUDED

#include "private.h"

namespace simgrid{
namespace smpi{

class Group {
  private:
    int m_size;
    int *m_rank_to_index_map;
    xbt_dict_t m_index_to_rank_map;
    int m_refcount;
  public:

    Group();
    Group(int size);
    Group(Group* origin);
    ~Group();

    void destroy();
    void set_mapping(int index, int rank);
    int index(int rank);
    int rank(int index);
    int use();
    int unuse();
    int size();
    int compare(MPI_Group group2);
    int incl(int n, int* ranks, MPI_Group* newgroup);
    int excl(int n, int *ranks, MPI_Group * newgroup);
    int group_union(MPI_Group group2, MPI_Group* newgroup);
    int intersection(MPI_Group group2, MPI_Group* newgroup);
    int difference(MPI_Group group2, MPI_Group* newgroup);
    int range_incl(int n, int ranges[][3], MPI_Group * newgroup);
    int range_excl(int n, int ranges[][3], MPI_Group * newgroup);
};
}
}

#endif
