/* Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_WIN_HPP_INCLUDED
#define SMPI_WIN_HPP_INCLUDED

#include "simgrid/s4u/Barrier.hpp"
#include "smpi_errhandler.hpp"
#include "smpi_f2c.hpp"
#include "smpi_keyvals.hpp"
#include "smpi_config.hpp"

#include <vector>
#include <list>

namespace simgrid::smpi {

class Win : public F2C, public Keyval {
  void* base_;
  MPI_Aint size_;
  int disp_unit_;
  int assert_ = 0;
  MPI_Info info_;
  MPI_Comm comm_;
  std::vector<MPI_Request> requests_;
  s4u::MutexPtr mut_ = s4u::Mutex::create();
  s4u::BarrierPtr bar_;
  std::vector<MPI_Win> connected_wins_;
  std::string name_;
  int opened_               = 0;
  MPI_Group src_group_      = MPI_GROUP_NULL; // for post/wait
  MPI_Group dst_group_      = MPI_GROUP_NULL; // for start/complete
  int count_                = 0; // for ordering the accs
  s4u::MutexPtr lock_mut_   = s4u::Mutex::create();
  s4u::MutexPtr atomic_mut_ = s4u::Mutex::create();
  std::list<int> lockers_;
  int rank_; // to identify owner for barriers in MPI_COMM_WORLD
  int mode_ = 0; // exclusive or shared lock
  bool allocated_;
  bool dynamic_;
  MPI_Errhandler errhandler_ = _smpi_cfg_default_errhandler_is_error ? MPI_ERRORS_ARE_FATAL : MPI_ERRORS_RETURN;

public:
  static std::unordered_map<int, smpi_key_elem> keyvals_;
  static int keyval_id_;

  Win(void* base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, bool allocated = false,
      bool dynamic = false);
  Win(MPI_Info info, MPI_Comm comm) : Win(MPI_BOTTOM, 0, 1, info, comm, false, true){};
  Win(const Win&) = delete;
  Win& operator=(const Win&) = delete;
  static int del(Win* win);
  int attach (void *base, MPI_Aint size);
  int detach (const void *base);
  void get_name(char* name, int* length) const;
  std::string name() const override { return name_.empty() ? "MPI_Win" : name_; }
  void get_group( MPI_Group* group);
  void set_name(const char* name);
  int rank() const;
  MPI_Comm comm() const;
  bool dynamic() const;
  int start(MPI_Group group, int assert);
  int post(MPI_Group group, int assert);
  int complete();
  MPI_Info info();
  void set_info( MPI_Info info);
  int wait();
  MPI_Aint size() const;
  void* base() const;
  int disp_unit() const;
  int fence(int assert);
  int opened() const {return opened_;}
  int put(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Request* request=nullptr);
  int get( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Request* request=nullptr);
  int accumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Request* request=nullptr);
  int get_accumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, void *result_addr,
              int result_count, MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp, int target_count,
              MPI_Datatype target_datatype, MPI_Op op, MPI_Request* request=nullptr);
  int compare_and_swap(const void* origin_addr, const void* compare_addr, void* result_addr, MPI_Datatype datatype,
                       int target_rank, MPI_Aint target_disp);
  static Win* f2c(int id);

  int lock(int lock_type, int rank, int assert);
  int unlock(int rank);
  int lock_all(int assert);
  int unlock_all();
  int flush(int rank);
  int flush_local(int rank);
  int flush_all();
  int flush_local_all();
  int finish_comms();
  int finish_comms(int rank);
  int shared_query(int rank, MPI_Aint* size, int* disp_unit, void* baseptr) const;
  MPI_Errhandler errhandler();
  void set_errhandler( MPI_Errhandler errhandler);
};

} // namespace simgrid::smpi

#endif
