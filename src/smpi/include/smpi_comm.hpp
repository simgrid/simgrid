/* Copyright (c) 2010-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_COMM_HPP_INCLUDED
#define SMPI_COMM_HPP_INCLUDED

#include <list>
#include <string>
#include "smpi_errhandler.hpp"
#include "smpi_keyvals.hpp"
#include "smpi_group.hpp"
#include "smpi_topo.hpp"

namespace simgrid{
namespace smpi{

class Comm : public F2C, public Keyval{
  friend Topo;
  MPI_Group group_;
  SMPI_Topo_type topoType_;
  MPI_Topology topo_; // to be replaced by an union
  int refcount_;
  MPI_Comm leaders_comm_; // inter-node communicator
  MPI_Comm intra_comm_;   // intra-node communicator . For MPI_COMM_WORLD this can't be used, as var is global.
  // use an intracomm stored in the process data instead
  int* leaders_map_; // who is the leader of each process
  int is_uniform_;
  int* non_uniform_map_;        // set if smp nodes have a different number of processes allocated
  int is_blocked_;              // are ranks allocated on the same smp node contiguous ?
  int is_smp_comm_;             // set to 0 in case this is already an intra-comm or a leader-comm to avoid recursion
  std::list<MPI_Win> rma_wins_; // attached windows for synchronization.
  std::string name_;
  MPI_Info info_;
  int id_;
  MPI_Errhandler errhandler_;

public:
  static std::unordered_map<int, smpi_key_elem> keyvals_;
  static int keyval_id_;

  Comm() = default;
  Comm(MPI_Group group, MPI_Topology topo, int smp = 0, int id=MPI_UNDEFINED);
  int dup(MPI_Comm* newcomm);
  int dup_with_info(MPI_Info info, MPI_Comm* newcomm);
  MPI_Group group();
  MPI_Topology topo() { return topo_; }
  int size();
  int rank();
  int id();
  void get_name(char* name, int* len);
  void set_name(const char* name);
  MPI_Info info();
  void set_info( MPI_Info info);
  MPI_Errhandler errhandler();
  void set_errhandler( MPI_Errhandler errhandler);
  void set_leaders_comm(MPI_Comm leaders);
  void set_intra_comm(MPI_Comm leaders) { intra_comm_ = leaders; };
  int* get_non_uniform_map();
  int* get_leaders_map();
  MPI_Comm get_leaders_comm();
  MPI_Comm get_intra_comm();
  MPI_Comm find_intra_comm(int* leader);
  int is_uniform();
  int is_blocked();
  int is_smp_comm();
  MPI_Comm split(int color, int key);
  void cleanup_smp();
  void ref();
  static void unref(MPI_Comm comm);
  static void destroy(MPI_Comm comm);
  void init_smp();

  static void free_f(int id);
  static Comm* f2c(int);

  static int keyval_create(MPI_Comm_copy_attr_function* copy_fn, MPI_Comm_delete_attr_function* delete_fn, int* keyval,
                           void* extra_state);
  static int keyval_free(int* keyval);
  static void keyval_cleanup();

  void add_rma_win(MPI_Win win);
  void remove_rma_win(MPI_Win win);
  void finish_rma_calls();
  MPI_Comm split_type(int type, int key, MPI_Info info);
};

} // namespace smpi
} // namespace simgrid

#endif
