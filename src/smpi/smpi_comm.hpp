/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_COMM_HPP_INCLUDED
#define SMPI_COMM_HPP_INCLUDED

#include "private.h"

namespace simgrid{
namespace smpi{

class Comm {

  private:
    MPI_Group m_group;
    MPIR_Topo_type m_topoType; 
    MPI_Topology m_topo; // to be replaced by an union
    int m_refcount;
    MPI_Comm m_leaders_comm;//inter-node communicator
    MPI_Comm m_intra_comm;//intra-node communicator . For MPI_COMM_WORLD this can't be used, as var is global.
    //use an intracomm stored in the process data instead
    int* m_leaders_map; //who is the leader of each process
    int m_is_uniform;
    int* m_non_uniform_map; //set if smp nodes have a different number of processes allocated
    int m_is_blocked;// are ranks allocated on the same smp node contiguous ?
    xbt_dict_t m_attributes;

  public:
    Comm();
    Comm(MPI_Group group, MPI_Topology topo);

    void destroy();
    int dup(MPI_Comm* newcomm);
    MPI_Group group();
    MPI_Topology topo();
    int size();
    int rank();
    void get_name (char* name, int* len);
    void set_leaders_comm(MPI_Comm leaders);
    void set_intra_comm(MPI_Comm leaders);
    int* get_non_uniform_map();
    int* get_leaders_map();
    MPI_Comm get_leaders_comm();
    MPI_Comm get_intra_comm();
    int is_uniform();
    int is_blocked();
    MPI_Comm split(int color, int key);
    void use();
    void cleanup_attributes();
    void cleanup_smp();
    void unuse();
    void init_smp();
    int attr_delete(int keyval);
    int attr_get(int keyval, void* attr_value, int* flag);
    int attr_put(int keyval, void* attr_value);

};

}
}


#endif
