/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_PRIVATE_HPP
#define SMPI_PRIVATE_HPP

#include "simgrid/s4u/Barrier.hpp"
#include "smpi/smpi.h"
#include "smpi/smpi_helpers_internal.h"
#include "src/instr/instr_smpi.hpp"
#include <unordered_map>
#include <vector>

constexpr unsigned MPI_REQ_PERSISTENT     = 0x1;
constexpr unsigned MPI_REQ_NON_PERSISTENT = 0x2;
constexpr unsigned MPI_REQ_SEND           = 0x4;
constexpr unsigned MPI_REQ_RECV           = 0x8;
constexpr unsigned MPI_REQ_PROBE          = 0x10;
constexpr unsigned MPI_REQ_ISEND          = 0x20;
constexpr unsigned MPI_REQ_SSEND          = 0x40;
constexpr unsigned MPI_REQ_PREPARED       = 0x80;
constexpr unsigned MPI_REQ_FINISHED       = 0x100;
constexpr unsigned MPI_REQ_RMA            = 0x200;
constexpr unsigned MPI_REQ_ACCUMULATE     = 0x400;
constexpr unsigned MPI_REQ_GENERALIZED    = 0x800;
constexpr unsigned MPI_REQ_COMPLETE       = 0x1000;
constexpr unsigned MPI_REQ_BSEND          = 0x2000;
constexpr unsigned MPI_REQ_MATCHED        = 0x4000;
constexpr unsigned MPI_REQ_CANCELLED      = 0x8000;

enum class SmpiProcessState { UNINITIALIZED, INITIALIZING, INITIALIZED /*(=MPI_Init called)*/, FINALIZED };

constexpr int COLL_TAG_REDUCE         = -112;
constexpr int COLL_TAG_SCATTER        = -223;
constexpr int COLL_TAG_SCATTERV       = -334;
constexpr int COLL_TAG_GATHER         = -445;
constexpr int COLL_TAG_ALLGATHER      = -556;
constexpr int COLL_TAG_ALLGATHERV     = -667;
constexpr int COLL_TAG_BARRIER        = -778;
constexpr int COLL_TAG_REDUCE_SCATTER = -889;
constexpr int COLL_TAG_ALLTOALLV      = -1000;
constexpr int COLL_TAG_ALLTOALLW      = -1020;
constexpr int COLL_TAG_ALLTOALL       = -1112;
constexpr int COLL_TAG_GATHERV        = -2223;
constexpr int COLL_TAG_BCAST          = -3334;
constexpr int COLL_TAG_ALLREDUCE      = -4445;

// SMPI_RMA_TAG has to be the smallest one, as it will be decremented for accumulate ordering.
constexpr int SMPI_RMA_TAG            = -6666;

#define MPI_REQUEST_IGNORED ((MPI_Request*)-100)

/* Bindings for MPI special values */
extern XBT_PUBLIC int mpi_in_place_;
extern XBT_PUBLIC int mpi_bottom_;
extern XBT_PUBLIC int mpi_status_ignore_;
extern XBT_PUBLIC int mpi_statuses_ignore_; 
/* Convert between Fortran and C */
#define FORT_ADDR(addr, val, val2)                                         \
  (((void *)(addr) == (void*) &(val2))                  \
   ? (val) : (void *)(addr))
#define FORT_BOTTOM(addr) FORT_ADDR((addr), MPI_BOTTOM, mpi_bottom_)
#define FORT_IN_PLACE(addr) FORT_ADDR((addr), MPI_IN_PLACE, mpi_in_place_)
#define FORT_STATUS_IGNORE(addr) static_cast<MPI_Status*>(FORT_ADDR((addr), MPI_STATUS_IGNORE, mpi_status_ignore_))
#define FORT_STATUSES_IGNORE(addr)                                                                                     \
  static_cast<MPI_Status*>(FORT_ADDR((addr), MPI_STATUSES_IGNORE, mpi_statuses_ignore_))

extern XBT_PRIVATE MPI_Comm MPI_COMM_UNINITIALIZED;

using MPIR_Cart_Topology       = SMPI_Cart_topology*;
using MPIR_Graph_Topology      = SMPI_Graph_topology*;
using MPIR_Dist_Graph_Topology = SMPI_Dist_Graph_topology*;

XBT_PRIVATE simgrid::smpi::ActorExt* smpi_process();
XBT_PRIVATE simgrid::smpi::ActorExt* smpi_process_remote(simgrid::s4u::ActorPtr actor);
XBT_PRIVATE int smpi_get_universe_size();

XBT_PRIVATE void smpi_deployment_register_process(const std::string& instance_id, int rank, simgrid::s4u::Actor* actor);
XBT_PRIVATE void smpi_deployment_unregister_process(const std::string& instance_id);

XBT_PRIVATE MPI_Comm* smpi_deployment_comm_world(const std::string& instance_id);
XBT_PRIVATE void smpi_deployment_cleanup_instances();

XBT_PRIVATE void smpi_comm_copy_buffer_callback(simgrid::kernel::activity::CommImpl* comm, void* buff,
                                                size_t buff_size);

XBT_PRIVATE void smpi_comm_null_copy_buffer_callback(simgrid::kernel::activity::CommImpl* comm, void* buff,
                                                     size_t buff_size);

XBT_PRIVATE int smpi_enabled();
XBT_PRIVATE double smpi_mpi_wtime();
XBT_PRIVATE void smpi_mpi_init();

enum class SharedMallocType { NONE, LOCAL, GLOBAL };
enum class SmpiPrivStrategies { NONE = 0, MMAP = 1, DLOPEN = 2, DEFAULT = DLOPEN };

XBT_PRIVATE double smpi_cfg_host_speed();
XBT_PRIVATE bool smpi_cfg_simulate_computation();
XBT_PRIVATE SharedMallocType smpi_cfg_shared_malloc();
XBT_PRIVATE double smpi_cfg_cpu_thresh();
XBT_PRIVATE SmpiPrivStrategies smpi_cfg_privatization();
XBT_PRIVATE int smpi_cfg_async_small_thresh();
XBT_PRIVATE int smpi_cfg_detached_send_thresh();
XBT_PRIVATE bool smpi_cfg_grow_injected_times();
XBT_PRIVATE double smpi_cfg_iprobe_cpu_usage();
XBT_PRIVATE bool smpi_cfg_trace_call_location();
XBT_PRIVATE bool smpi_cfg_trace_call_use_absolute_path();
XBT_PRIVATE std::string smpi_cfg_comp_adjustment_file();
XBT_PRIVATE std::string smpi_cfg_papi_events_file();
XBT_PRIVATE double smpi_cfg_auto_shared_malloc_thresh();

// utilities
extern XBT_PRIVATE char* smpi_data_exe_start; // start of the data+bss segment of the executable
extern XBT_PRIVATE int smpi_data_exe_size;    // size of the data+bss segment of the executable

XBT_PRIVATE void smpi_switch_data_segment(simgrid::s4u::ActorPtr actor);

XBT_PRIVATE void smpi_prepare_global_memory_segment();
XBT_PRIVATE void smpi_backup_global_memory_segment();
XBT_PRIVATE void smpi_destroy_global_memory_segments();
XBT_PRIVATE void smpi_bench_destroy();
XBT_PRIVATE void smpi_bench_begin();
XBT_PRIVATE void smpi_bench_end();
XBT_PRIVATE void smpi_shared_destroy();
XBT_PRIVATE double smpi_adjust_comp_speed();

XBT_PRIVATE unsigned char* smpi_get_tmp_sendbuffer(size_t size);
XBT_PRIVATE unsigned char* smpi_get_tmp_recvbuffer(size_t size);
XBT_PRIVATE void smpi_free_tmp_buffer(const unsigned char* buf);
XBT_PRIVATE void smpi_free_replay_tmp_buffers();

extern "C" {
// f77 wrappers

void mpi_init_(int* ierr);
void mpi_finalize_(int* ierr);
void mpi_abort_(int* comm, int* errorcode, int* ierr);
void mpi_comm_rank_(int* comm, int* rank, int* ierr);
void mpi_comm_size_(int* comm, int* size, int* ierr);
double mpi_wtime_();
double mpi_wtick_();
void mpi_initialized_(int* flag, int* ierr);

void mpi_comm_dup_(int* comm, int* newcomm, int* ierr);
void mpi_comm_create_(int* comm, int* group, int* newcomm, int* ierr);
void mpi_comm_free_(int* comm, int* ierr);
void mpi_comm_split_(int* comm, int* color, int* key, int* comm_out, int* ierr);
void mpi_group_incl_(int* group, int* n, int* key, int* group_out, int* ierr);
void mpi_comm_group_(int* comm, int* group_out, int* ierr);
void mpi_comm_create_group_(int* comm, int* group, int, int* comm_out, int* ierr);
void mpi_send_init_(void* buf, int* count, int* datatype, int* dst, int* tag, int* comm, int* request, int* ierr);
void mpi_isend_(void* buf, int* count, int* datatype, int* dst, int* tag, int* comm, int* request, int* ierr);
void mpi_irsend_(void* buf, int* count, int* datatype, int* dst, int* tag, int* comm, int* request, int* ierr);
void mpi_send_(void* buf, int* count, int* datatype, int* dst, int* tag, int* comm, int* ierr);
void mpi_rsend_(void* buf, int* count, int* datatype, int* dst, int* tag, int* comm, int* ierr);
void mpi_recv_init_(void* buf, int* count, int* datatype, int* src, int* tag, int* comm, int* request, int* ierr);
void mpi_irecv_(void* buf, int* count, int* datatype, int* src, int* tag, int* comm, int* request, int* ierr);
void mpi_recv_(void* buf, int* count, int* datatype, int* src, int* tag, int* comm, MPI_Status* status, int* ierr);
void mpi_start_(int* request, int* ierr);
void mpi_startall_(int* count, int* requests, int* ierr);
void mpi_wait_(int* request, MPI_Status* status, int* ierr);
void mpi_waitany_(int* count, int* requests, int* index, MPI_Status* status, int* ierr);
void mpi_waitall_(int* count, int* requests, MPI_Status* status, int* ierr);

void mpi_barrier_(int* comm, int* ierr);
void mpi_bcast_(void* buf, int* count, int* datatype, int* root, int* comm, int* ierr);
void mpi_reduce_(void* sendbuf, void* recvbuf, int* count, int* datatype, int* op, int* root, int* comm, int* ierr);
void mpi_allreduce_(void* sendbuf, void* recvbuf, int* count, int* datatype, int* op, int* comm, int* ierr);
void mpi_reduce_scatter_(void* sendbuf, void* recvbuf, int* recvcounts, int* datatype, int* op, int* comm, int* ierr);
void mpi_reduce_scatter_block_(void* sendbuf, void* recvbuf, int* recvcount, int* datatype, int* op, int* comm,
                               int* ierr);
void mpi_scatter_(void* sendbuf, int* sendcount, int* sendtype, void* recvbuf, int* recvcount, int* recvtype, int* root,
                  int* comm, int* ierr);
void mpi_scatterv_(void* sendbuf, int* sendcounts, int* displs, int* sendtype, void* recvbuf, int* recvcount,
                   int* recvtype, int* root, int* comm, int* ierr);
void mpi_gather_(void* sendbuf, int* sendcount, int* sendtype, void* recvbuf, int* recvcount, int* recvtype, int* root,
                 int* comm, int* ierr);
void mpi_gatherv_(void* sendbuf, int* sendcount, int* sendtype, void* recvbuf, int* recvcounts, int* displs,
                  int* recvtype, int* root, int* comm, int* ierr);
void mpi_allgather_(void* sendbuf, int* sendcount, int* sendtype, void* recvbuf, int* recvcount, int* recvtype,
                    int* comm, int* ierr);
void mpi_allgatherv_(void* sendbuf, int* sendcount, int* sendtype, void* recvbuf, int* recvcount, int* displs,
                     int* recvtype, int* comm, int* ierr);
void mpi_scan_(void* sendbuf, void* recvbuf, int* count, int* datatype, int* op, int* comm, int* ierr);
void mpi_alltoall_(void* sendbuf, int* sendcount, int* sendtype, void* recvbuf, int* recvcount, int* recvtype,
                   int* comm, int* ierr);
void mpi_alltoallv_(void* sendbuf, int* sendcounts, int* senddisps, int* sendtype, void* recvbuf, int* recvcounts,
                    int* recvdisps, int* recvtype, int* comm, int* ierr);
void mpi_alltoallw_(void* sendbuf, int* sendcnts, int* sdispls, int* sendtypes, void* recvbuf, int* recvcnts,
                    int* rdispls, int* recvtypes, int* comm, int* ierr);
void mpi_exscan_(void* sendbuf, void* recvbuf, int* count, int* datatype, int* op, int* comm, int* ierr);

void mpi_ibarrier_(int* comm, int* request, int* ierr);
void mpi_ibcast_(void* buf, int* count, int* datatype, int* root, int* comm, int* request, int* ierr);
void mpi_ireduce_(void* sendbuf, void* recvbuf, int* count, int* datatype, int* op, int* root, int* comm, int* request, int* ierr);
void mpi_iallreduce_(void* sendbuf, void* recvbuf, int* count, int* datatype, int* op, int* comm, int* request, int* ierr);
void mpi_ireduce_scatter_(void* sendbuf, void* recvbuf, int* recvcounts, int* datatype, int* op, int* comm, int* request, int* ierr);
void mpi_ireduce_scatter_block_(void* sendbuf, void* recvbuf, int* recvcount, int* datatype, int* op, int* comm, int* request ,
                               int* ierr);
void mpi_iscatter_(void* sendbuf, int* sendcount, int* sendtype, void* recvbuf, int* recvcount, int* recvtype, int* root,
                  int* comm, int* request, int* ierr);
void mpi_iscatterv_(void* sendbuf, int* sendcounts, int* displs, int* sendtype, void* recvbuf, int* recvcount,
                   int* recvtype, int* root, int* comm, int* request, int* ierr);
void mpi_igather_(void* sendbuf, int* sendcount, int* sendtype, void* recvbuf, int* recvcount, int* recvtype, int* root,
                 int* comm, int* request, int* ierr);
void mpi_igatherv_(void* sendbuf, int* sendcount, int* sendtype, void* recvbuf, int* recvcounts, int* displs,
                  int* recvtype, int* root, int* comm, int* request, int* ierr);
void mpi_iallgather_(void* sendbuf, int* sendcount, int* sendtype, void* recvbuf, int* recvcount, int* recvtype,
                    int* comm, int* request, int* ierr);
void mpi_iallgatherv_(void* sendbuf, int* sendcount, int* sendtype, void* recvbuf, int* recvcount, int* displs,
                     int* recvtype, int* comm, int* request, int* ierr);
void mpi_iscan_(void* sendbuf, void* recvbuf, int* count, int* datatype, int* op, int* comm, int* request, int* ierr);
void mpi_ialltoall_(void* sendbuf, int* sendcount, int* sendtype, void* recvbuf, int* recvcount, int* recvtype,
                   int* comm, int* request, int* ierr);
void mpi_ialltoallv_(void* sendbuf, int* sendcounts, int* senddisps, int* sendtype, void* recvbuf, int* recvcounts,
                    int* recvdisps, int* recvtype, int* comm, int* request, int* ierr);
void mpi_ialltoallw_(void* sendbuf, int* sendcnts, int* sdispls, int* sendtypes, void* recvbuf, int* recvcnts,
                    int* rdispls, int* recvtypes, int* comm, int* request, int* ierr);
void mpi_iexscan_(void* sendbuf, void* recvbuf, int* count, int* datatype, int* op, int* comm, int* request, int* ierr);

void mpi_type_size_(int* datatype, int* size, int* ierr);
void mpi_get_processor_name_(char* name, int* resultlen, int* ierr);
void mpi_test_(int* request, int* flag, MPI_Status* status, int* ierr);
void mpi_testall_(int* count, int* requests, int* flag, MPI_Status* statuses, int* ierr);
void mpi_get_count_(MPI_Status* status, int* datatype, int* count, int* ierr);
void mpi_type_extent_(int* datatype, MPI_Aint* extent, int* ierr);
void mpi_attr_get_(int* comm, int* keyval, int* attr_value, int* flag, int* ierr);
void mpi_type_commit_(int* datatype, int* ierr);
void mpi_type_vector_(int* count, int* blocklen, int* stride, int* old_type, int* newtype, int* ierr);
void mpi_type_hvector_(int* count, int* blocklen, MPI_Aint* stride, int* old_type, int* newtype, int* ierr);
void mpi_type_create_hvector_(int* count, int* blocklen, MPI_Aint* stride, int* old_type, int* newtype, int* ierr);
void mpi_type_free_(int* datatype, int* ierr);
void mpi_type_lb_(int* datatype, MPI_Aint* extent, int* ierr);
void mpi_type_ub_(int* datatype, MPI_Aint* extent, int* ierr);
void mpi_win_fence_(int* assert, int* win, int* ierr);
void mpi_win_free_(int* win, int* ierr);
void mpi_win_create_(int* base, MPI_Aint* size, int* disp_unit, int* info, int* comm, int* win, int* ierr);
void mpi_win_set_name_(int* win, char* name, int* ierr, int size);
void mpi_win_get_name_(int* win, char* name, int* len, int* ierr);
void mpi_win_post_(int* group, int assert, int* win, int* ierr);
void mpi_win_start_(int* group, int assert, int* win, int* ierr);
void mpi_win_complete_(int* win, int* ierr);
void mpi_win_wait_(int* win, int* ierr);
void mpi_win_allocate_(MPI_Aint* size, int* disp_unit, int* info, int* comm, void* base, int* win, int* ierr);
void mpi_win_attach_(int* win, int* base, MPI_Aint* size, int* ierr);
void mpi_win_create_dynamic_(int* info, int* comm, int* win, int* ierr);
void mpi_win_detach_(int* win, int* base, int* ierr);
void mpi_win_set_info_(int* win, int* info, int* ierr);
void mpi_win_get_info_(int* win, int* info, int* ierr);
void mpi_win_get_group_(int* win, int* group, int* ierr);
void mpi_win_get_attr_(int* win, int* type_keyval, MPI_Aint* attribute_val, int* flag, int* ierr);
void mpi_win_set_attr_(int* win, int* type_keyval, MPI_Aint* att, int* ierr);
void mpi_win_delete_attr_(int* win, int* comm_keyval, int* ierr);
void mpi_win_create_keyval_(void* copy_fn, void* delete_fn, int* keyval, void* extra_state, int* ierr);
void mpi_win_free_keyval_(int* keyval, int* ierr);
void mpi_win_lock_(int* lock_type, int* rank, int* assert, int* win, int* ierr);
void mpi_win_lock_all_(int* assert, int* win, int* ierr);
void mpi_win_unlock_(int* rank, int* win, int* ierr);
void mpi_win_unlock_all_(int* win, int* ierr);
void mpi_win_flush_(int* rank, int* win, int* ierr);
void mpi_win_flush_local_(int* rank, int* win, int* ierr);
void mpi_win_flush_all_(int* win, int* ierr);
void mpi_win_flush_local_all_(int* win, int* ierr);
void mpi_win_dup_fn_( int* win, int* keyval, int* extrastate, MPI_Aint* valin, MPI_Aint* valout, int* flag, int* ierr );
void mpi_win_null_copy_fn_( int* win, int* keyval, int* extrastate, MPI_Aint* valin, MPI_Aint* valout, int* flag, int* ierr );
void mpi_info_create_(int* info, int* ierr);
void mpi_info_set_(int* info, char* key, char* value, int* ierr, unsigned int keylen, unsigned int valuelen);
void mpi_info_free_(int* info, int* ierr);
void mpi_get_(int* origin_addr, int* origin_count, int* origin_datatype, int* target_rank, MPI_Aint* target_disp,
              int* target_count, int* target_datatype, int* win, int* ierr);
void mpi_put_(int* origin_addr, int* origin_count, int* origin_datatype, int* target_rank, MPI_Aint* target_disp,
              int* target_count, int* target_datatype, int* win, int* ierr);
void mpi_rget_(int* origin_addr, int* origin_count, int* origin_datatype, int* target_rank, MPI_Aint* target_disp,
               int* target_count, int* target_datatype, int* win, int* request, int* ierr);
void mpi_rput_(int* origin_addr, int* origin_count, int* origin_datatype, int* target_rank, MPI_Aint* target_disp,
               int* target_count, int* target_datatype, int* win, int* request, int* ierr);
void mpi_fetch_and_op_(int* origin_addr, int* result_addr, int* datatype, int* target_rank, MPI_Aint* target_disp,
                       int* op, int* win, int* ierr);
void mpi_compare_and_swap_(int* origin_addr, int* compare_addr, int* result_addr, int* datatype, int* target_rank,
                           MPI_Aint* target_disp, int* win, int* ierr);
void mpi_get_accumulate_(int* origin_addr, int* origin_count, int* origin_datatype, int* result_addr, int* result_count,
                         int* result_datatype, int* target_rank, MPI_Aint* target_disp, int* target_count,
                         int* target_datatype, int* op, int* win, int* ierr);
void mpi_rget_accumulate_(int* origin_addr, int* origin_count, int* origin_datatype, int* result_addr,
                          int* result_count, int* result_datatype, int* target_rank, MPI_Aint* target_disp,
                          int* target_count, int* target_datatype, int* op, int* win, int* request, int* ierr);
void mpi_accumulate_(int* origin_addr, int* origin_count, int* origin_datatype, int* target_rank, MPI_Aint* target_disp,
                     int* target_count, int* target_datatype, int* op, int* win, int* ierr);
void mpi_raccumulate_(int* origin_addr, int* origin_count, int* origin_datatype, int* target_rank,
                      MPI_Aint* target_disp, int* target_count, int* target_datatype, int* op, int* win, int* request,
                      int* ierr);
void mpi_error_string_(int* errorcode, char* string, int* resultlen, int* ierr);
void mpi_sendrecv_(void* sendbuf, int* sendcount, int* sendtype, int* dst, int* sendtag, void* recvbuf, int* recvcount,
                   int* recvtype, int* src, int* recvtag, int* comm, MPI_Status* status, int* ierr);

void mpi_finalized_(int* flag, int* ierr);
void mpi_init_thread_(int* required, int* provided, int* ierr);
void mpi_query_thread_(int* provided, int* ierr);
void mpi_is_thread_main_(int* flag, int* ierr);
void mpi_address_(void* location, MPI_Aint* address, int* ierr);
void mpi_get_address_(void* location, MPI_Aint* address, int* ierr);
void mpi_type_dup_(int* datatype, int* newdatatype, int* ierr);
void mpi_type_set_name_(int* datatype, char* name, int* ierr, int size);
void mpi_type_get_name_(int* datatype, char* name, int* len, int* ierr);
void mpi_type_get_attr_(int* type, int* type_keyval, int* attribute_val, int* flag, int* ierr);
void mpi_type_set_attr_(int* type, int* type_keyval, int* attribute_val, int* ierr);
void mpi_type_delete_attr_(int* type, int* type_keyval, int* ierr);
void mpi_type_create_keyval_(void* copy_fn, void* delete_fn, int* keyval, void* extra_state, int* ierr);
void mpi_type_free_keyval_(int* keyval, int* ierr);
void mpi_pcontrol_(int* level, int* ierr);
void mpi_type_get_extent_(int* datatype, MPI_Aint* lb, MPI_Aint* extent, int* ierr);
void mpi_type_get_true_extent_(int* datatype, MPI_Aint* lb, MPI_Aint* extent, int* ierr);
void mpi_op_create_(void* function, int* commute, int* op, int* ierr);
void mpi_op_free_(int* op, int* ierr);
void mpi_op_commutative_(int* op, int* commute, int* ierr);
void mpi_group_free_(int* group, int* ierr);
void mpi_group_size_(int* group, int* size, int* ierr);
void mpi_group_rank_(int* group, int* rank, int* ierr);
void mpi_group_translate_ranks_(int* group1, int* n, int* ranks1, int* group2, int* ranks2, int* ierr);
void mpi_group_compare_(int* group1, int* group2, int* result, int* ierr);
void mpi_group_union_(int* group1, int* group2, int* newgroup, int* ierr);
void mpi_group_intersection_(int* group1, int* group2, int* newgroup, int* ierr);
void mpi_group_difference_(int* group1, int* group2, int* newgroup, int* ierr);
void mpi_group_excl_(int* group, int* n, int* ranks, int* newgroup, int* ierr);
void mpi_group_range_incl_(int* group, int* n, int ranges[][3], int* newgroup, int* ierr);
void mpi_group_range_excl_(int* group, int* n, int ranges[][3], int* newgroup, int* ierr);
void mpi_comm_get_attr_(int* comm, int* comm_keyval, int* attribute_val, int* flag, int* ierr);
void mpi_comm_set_attr_(int* comm, int* comm_keyval, int* attribute_val, int* ierr);
void mpi_comm_delete_attr_(int* comm, int* comm_keyval, int* ierr);
void mpi_comm_create_keyval_(void* copy_fn, void* delete_fn, int* keyval, void* extra_state, int* ierr);
void mpi_comm_free_keyval_(int* keyval, int* ierr);
void mpi_comm_get_name_(int* comm, char* name, int* len, int* ierr);
void mpi_comm_compare_(int* comm1, int* comm2, int* result, int* ierr);
void mpi_comm_disconnect_(int* comm, int* ierr);
void mpi_request_free_(int* request, int* ierr);
void mpi_sendrecv_replace_(void* buf, int* count, int* datatype, int* dst, int* sendtag, int* src, int* recvtag,
                           int* comm, MPI_Status* status, int* ierr);
void mpi_testany_(int* count, int* requests, int* index, int* flag, MPI_Status* status, int* ierr);
void mpi_waitsome_(int* incount, int* requests, int* outcount, int* indices, MPI_Status* status, int* ierr);
void mpi_reduce_local_(void* inbuf, void* inoutbuf, int* count, int* datatype, int* op, int* ierr);
void mpi_pack_size_(int* incount, int* datatype, int* comm, int* size, int* ierr);
void mpi_cart_coords_(int* comm, int* rank, int* maxdims, int* coords, int* ierr);
void mpi_cart_create_(int* comm_old, int* ndims, int* dims, int* periods, int* reorder, int* comm_cart, int* ierr);
void mpi_cart_get_(int* comm, int* maxdims, int* dims, int* periods, int* coords, int* ierr);
void mpi_cart_map_(int* comm_old, int* ndims, int* dims, int* periods, int* newrank, int* ierr);
void mpi_cart_rank_(int* comm, int* coords, int* rank, int* ierr);
void mpi_cart_shift_(int* comm, int* direction, int* displ, int* source, int* dest, int* ierr);
void mpi_cart_sub_(int* comm, int* remain_dims, int* comm_new, int* ierr);
void mpi_cartdim_get_(int* comm, int* ndims, int* ierr);
void mpi_graph_create_(int* comm_old, int* nnodes, int* index, int* edges, int* reorder, int* comm_graph, int* ierr);
void mpi_graph_get_(int* comm, int* maxindex, int* maxedges, int* index, int* edges, int* ierr);
void mpi_graph_map_(int* comm_old, int* nnodes, int* index, int* edges, int* newrank, int* ierr);
void mpi_graph_neighbors_(int* comm, int* rank, int* maxneighbors, int* neighbors, int* ierr);
void mpi_graph_neighbors_count_(int* comm, int* rank, int* nneighbors, int* ierr);
void mpi_graphdims_get_(int* comm, int* nnodes, int* nedges, int* ierr);
void mpi_topo_test_(int* comm, int* top_type, int* ierr);
void mpi_error_class_(int* errorcode, int* errorclass, int* ierr);
void mpi_errhandler_create_(void* function, int* errhandler, int* ierr);
void mpi_errhandler_free_(int* errhandler, int* ierr);
void mpi_errhandler_get_(int* comm, int* errhandler, int* ierr);
void mpi_errhandler_set_(int* comm, int* errhandler, int* ierr);
void mpi_comm_set_errhandler_(int* comm, int* errhandler, int* ierr);
void mpi_comm_get_errhandler_(int* comm, int* errhandler, int* ierr);
void mpi_type_contiguous_(int* count, int* old_type, int* newtype, int* ierr);
void mpi_cancel_(int* request, int* ierr);
void mpi_buffer_attach_(void* buffer, int* size, int* ierr);
void mpi_buffer_detach_(void* buffer, int* size, int* ierr);
void mpi_testsome_(int* incount, int* requests, int* outcount, int* indices, MPI_Status* statuses, int* ierr);
void mpi_comm_test_inter_(int* comm, int* flag, int* ierr);
void mpi_unpack_(void* inbuf, int* insize, int* position, void* outbuf, int* outcount, int* type, int* comm, int* ierr);
void mpi_pack_external_size_(char* datarep, int* incount, int* datatype, MPI_Aint* size, int* ierr);
void mpi_pack_external_(char* datarep, void* inbuf, int* incount, int* datatype, void* outbuf, MPI_Aint* outcount,
                        MPI_Aint* position, int* ierr);
void mpi_unpack_external_(char* datarep, void* inbuf, MPI_Aint* insize, MPI_Aint* position, void* outbuf, int* outcount,
                          int* datatype, int* ierr);
void mpi_type_hindexed_(int* count, int* blocklens, int* indices, int* old_type, int* newtype, int* ierr);
void mpi_type_create_hindexed_(int* count, int* blocklens, MPI_Aint* indices, int* old_type, int* newtype, int* ierr);
void mpi_type_create_hindexed_block_(int* count, int* blocklength, MPI_Aint* indices, int* old_type, int* newtype,
                                     int* ierr);
void mpi_type_indexed_(int* count, int* blocklens, int* indices, int* old_type, int* newtype, int* ierr);
void mpi_type_create_indexed_(int* count, int* blocklens, int* indices, int* old_type, int* newtype, int* ierr);
void mpi_type_create_indexed_block_(int* count, int* blocklength, int* indices, int* old_type, int* newtype, int* ierr);
void mpi_type_struct_(int* count, int* blocklens, int* indices, int* old_types, int* newtype, int* ierr);
void mpi_type_create_struct_(int* count, int* blocklens, MPI_Aint* indices, int* old_types, int* newtype, int* ierr);
void mpi_ssend_(void* buf, int* count, int* datatype, int* dest, int* tag, int* comm, int* ierr);
void mpi_ssend_init_(void* buf, int* count, int* datatype, int* dest, int* tag, int* comm, int* request, int* ierr);
void mpi_intercomm_create_(int* local_comm, int* local_leader, int* peer_comm, int* remote_leader, int* tag,
                           int* comm_out, int* ierr);
void mpi_intercomm_merge_(int* comm, int* high, int* comm_out, int* ierr);
void mpi_bsend_(void* buf, int* count, int* datatype, int* dest, int* tag, int* comm, int* ierr);
void mpi_bsend_init_(void* buf, int* count, int* datatype, int* dest, int* tag, int* comm, int* request, int* ierr);
void mpi_ibsend_(void* buf, int* count, int* datatype, int* dest, int* tag, int* comm, int* request, int* ierr);
void mpi_comm_remote_group_(int* comm, int* group, int* ierr);
void mpi_comm_remote_size_(int* comm, int* size, int* ierr);
void mpi_issend_(void* buf, int* count, int* datatype, int* dest, int* tag, int* comm, int* request, int* ierr);
void mpi_probe_(int* source, int* tag, int* comm, MPI_Status* status, int* ierr);
void mpi_attr_delete_(int* comm, int* keyval, int* ierr);
void mpi_attr_put_(int* comm, int* keyval, int* attr_value, int* ierr);
void mpi_rsend_init_(void* buf, int* count, int* datatype, int* dest, int* tag, int* comm, int* request, int* ierr);
void mpi_keyval_create_(void* copy_fn, void* delete_fn, int* keyval, void* extra_state, int* ierr);
void mpi_keyval_free_(int* keyval, int* ierr);
void mpi_test_cancelled_(MPI_Status* status, int* flag, int* ierr);
void mpi_pack_(void* inbuf, int* incount, int* type, void* outbuf, int* outcount, int* position, int* comm, int* ierr);
void mpi_get_elements_(MPI_Status* status, int* datatype, int* elements, int* ierr);
void mpi_dims_create_(int* nnodes, int* ndims, int* dims, int* ierr);
void mpi_iprobe_(int* source, int* tag, int* comm, int* flag, MPI_Status* status, int* ierr);
void mpi_type_get_envelope_(int* datatype, int* num_integers, int* num_addresses, int* num_datatypes, int* combiner,
                            int* ierr);
void mpi_type_get_contents_(int* datatype, int* max_integers, int* max_addresses, int* max_datatypes,
                            int* array_of_integers, MPI_Aint* array_of_addresses, int* array_of_datatypes, int* ierr);
void mpi_type_create_darray_(int* size, int* rank, int* ndims, int* array_of_gsizes, int* array_of_distribs,
                             int* array_of_dargs, int* array_of_psizes, int* order, int* oldtype, int* newtype,
                             int* ierr);
void mpi_type_create_resized_(int* oldtype, MPI_Aint* lb, MPI_Aint* extent, int* newtype, int* ierr);
void mpi_type_create_subarray_(int* ndims, int* array_of_sizes, int* array_of_subsizes, int* array_of_starts,
                               int* order, int* oldtype, int* newtype, int* ierr);
void mpi_type_match_size_(int* typeclass, int* size, int* datatype, int* ierr);
void mpi_comm_set_name_(int* comm, char* name, int* ierr);
void mpi_comm_dup_with_info_(int* comm, int* info, int* newcomm, int* ierr);
void mpi_comm_split_type_(int* comm, int* split_type, int* key, int* info, int* newcomm, int* ierr);
void mpi_comm_set_info_(int* comm, int* info, int* ierr);
void mpi_comm_get_info_(int* comm, int* info, int* ierr);
void mpi_info_get_(int* info, char* key, int* valuelen, char* value, int* flag, int* ierr, unsigned int keylen);
void mpi_comm_create_errhandler_(void* function, int* errhandler, int* ierr);
void mpi_add_error_class_(int* errorclass, int* ierr);
void mpi_add_error_code_(int* errorclass, int* errorcode, int* ierr);
void mpi_add_error_string_(int* errorcode, char* string, int* ierr);
void mpi_comm_call_errhandler_(int* comm, int* errorcode, int* ierr);
void mpi_info_dup_(int* info, int* newinfo, int* ierr);
void mpi_info_get_valuelen_(int* info, char* key, int* valuelen, int* flag, int* ierr, unsigned int keylen);
void mpi_info_delete_(int* info, char* key, int* ierr, unsigned int keylen);
void mpi_info_get_nkeys_(int* info, int* nkeys, int* ierr);
void mpi_info_get_nthkey_(int* info, int* n, char* key, int* ierr, unsigned int keylen);
void mpi_get_version_(int* version, int* subversion, int* ierr);
void mpi_get_library_version_(char* version, int* len, int* ierr);
void mpi_request_get_status_(int* request, int* flag, MPI_Status* status, int* ierr);
void mpi_grequest_start_(void* query_fn, void* free_fn, void* cancel_fn, void* extra_state, int* request, int* ierr);
void mpi_grequest_complete_(int* request, int* ierr);
void mpi_status_set_cancelled_(MPI_Status* status, int* flag, int* ierr);
void mpi_status_set_elements_(MPI_Status* status, int* datatype, int* count, int* ierr);
void mpi_comm_connect_(char* port_name, int* info, int* root, int* comm, int* newcomm, int* ierr);
void mpi_publish_name_(char* service_name, int* info, char* port_name, int* ierr);
void mpi_unpublish_name_(char* service_name, int* info, char* port_name, int* ierr);
void mpi_lookup_name_(char* service_name, int* info, char* port_name, int* ierr);
void mpi_comm_join_(int* fd, int* intercomm, int* ierr);
void mpi_open_port_(int* info, char* port_name, int* ierr);
void mpi_close_port_(char* port_name, int* ierr);
void mpi_comm_accept_(char* port_name, int* info, int* root, int* comm, int* newcomm, int* ierr);
void mpi_comm_spawn_(char* command, char* argv, int* maxprocs, int* info, int* root, int* comm, int* intercomm,
                     int* array_of_errcodes, int* ierr);
void mpi_comm_spawn_multiple_(int* count, char* array_of_commands, char** array_of_argv, int* array_of_maxprocs,
                              int* array_of_info, int* root, int* comm, int* intercomm, int* array_of_errcodes,
                              int* ierr);
void mpi_comm_get_parent_(int* parent, int* ierr);
void mpi_file_close_(int* file, int* ierr);
void mpi_file_delete_(char* filename, int* info, int* ierr);
void mpi_file_open_(int* comm, char* filename, int* amode, int* info, int* fh, int* ierr);
void mpi_file_seek_(int* fh, MPI_Offset* offset, int* whence, int* ierr);
void mpi_file_seek_shared_(int* fh, MPI_Offset* offset, int* whence, int* ierr);
void mpi_file_get_position_(int* fh, MPI_Offset* offset, int* ierr);
void mpi_file_get_position_shared_(int* fh, MPI_Offset* offset, int* ierr);
void mpi_file_set_view_(int* fh, MPI_Offset* offset, int* etype, int* filetype, char* datarep, int* info, int* ierr);
void mpi_file_get_view_(int* fh, MPI_Offset* disp, int* etype, int* filetype, char *datarep, int* ierr);
void mpi_file_read_(int* fh, void* buf, int* count, int* datatype, MPI_Status* status, int* ierr);
void mpi_file_read_shared_(int* fh, void *buf, int* count, int* datatype, MPI_Status* status, int* ierr);
void mpi_file_read_all_(int* fh, void *buf, int* count, int* datatype, MPI_Status* status, int* ierr);
void mpi_file_read_ordered_(int* fh, void *buf, int* count, int* datatype, MPI_Status* status, int* ierr);
void mpi_file_read_at_(int* fh, MPI_Offset* offset, void *buf, int* count, int* datatype, MPI_Status* status, int* ierr);
void mpi_file_read_at_all_(int* fh, MPI_Offset* offset, void *buf, int* count, int* datatype, MPI_Status* status, int* ierr);
void mpi_file_write_(int* fh, void* buf, int* count, int* datatype, MPI_Status* status, int* ierr);
void mpi_file_write_all_(int* fh, void* buf, int* count, int* datatype, MPI_Status* status, int* ierr);
void mpi_file_write_shared_(int* fh, void *buf, int* count, int* datatype, MPI_Status* status, int* ierr);
void mpi_file_write_ordered_(int* fh, void *buf, int* count, int* datatype, MPI_Status* status, int* ierr);
void mpi_file_write_at_(int* fh, MPI_Offset* offset, void *buf, int* count, int* datatype, MPI_Status* status, int* ierr);
void mpi_file_write_at_all_(int* fh, MPI_Offset* offset, void *buf, int* count, int* datatype, MPI_Status* status, int* ierr);
void smpi_init_fortran_types();
void smpi_execute_flops_(double* flops);
void smpi_execute_flops_benched_(double* flops);
void smpi_execute_(double* duration);
void smpi_execute_benched_(double* duration);
} // extern "C"

XBT_PRIVATE int smpi_temp_shm_get();
XBT_PRIVATE void* smpi_temp_shm_mmap(int fd, size_t size);

struct s_smpi_privatization_region_t {
  void* address;
  int file_descriptor;
};
using smpi_privatization_region_t = s_smpi_privatization_region_t*;

extern XBT_PRIVATE int smpi_loaded_page;
XBT_PRIVATE smpi_privatization_region_t smpi_init_global_memory_segment_process();

/**
 * Get the address of the beginning of the memory page where addr is located.
 * Note that we use an integer division here, so (a/b)*b is not a, unless a%b == 0
 *
 * This is used when privatizing.
 */
#define TOPAGE(addr) (void*)(((unsigned long)(addr) / xbt_pagesize) * xbt_pagesize)

/** Used only if PAPI is compiled in, but integrated anyway so that this file does not depend on internal_config.h (to speed builds) */
using papi_counter_t = std::vector<std::pair</* counter name */ std::string, /* counter value */ long long>>;
struct papi_process_data {
  papi_counter_t counter_data;
  int event_set;
};
extern std::map</* computation unit name */ std::string, papi_process_data, std::less<>> units2papi_setup;

extern std::unordered_map<std::string, double> location2speedup;

/** @brief Returns the last call location (filename, linenumber). Process-specific. */
XBT_PUBLIC smpi_trace_call_location_t* smpi_trace_get_call_location();

XBT_PRIVATE void private_execute_flops(double flops);

#define CHECK_ARGS(test, errcode, ...)                                                                                 \
  if (test) {                                                                                                          \
    int error_code_ = (errcode);                                                                                       \
    if (error_code_ != MPI_SUCCESS)                                                                                    \
      XBT_WARN(__VA_ARGS__);                                                                                           \
    return error_code_;                                                                                                \
  }

#define CHECK_MPI_NULL(num, val, err, ptr)                                                                             \
  CHECK_ARGS((ptr) == (val), (err),                                                                                    \
             "%s: param %d %s cannot be %s", __func__, (num), _XBT_STRINGIFY(ptr), _XBT_STRINGIFY(val));
#define CHECK_NULL(num,err,buf)                                                                                        \
  CHECK_ARGS((buf) == nullptr, (err),                                                                                  \
             "%s: param %d %s cannot be NULL", __func__, (num), _XBT_STRINGIFY(buf));
#define CHECK_NEGATIVE(num, err, val)                                                                                  \
  CHECK_ARGS((val) < 0, (err),                                                                                         \
             "%s: param %d %s cannot be negative", __func__, (num), _XBT_STRINGIFY(val));
#define CHECK_NEGATIVE_OR_ZERO(num, err, val)                                                                          \
  CHECK_ARGS((val) <= 0, (err),                                                                                        \
             "%s: param %d %s cannot be negative or 0", __func__, (num), _XBT_STRINGIFY(val));
#define CHECK_COMM2(num, comm)                                                                                         \
  CHECK_MPI_NULL((num), MPI_COMM_NULL, MPI_ERR_COMM, (comm))
#define CHECK_DELETED(num, err, obj)                                                                                   \
  CHECK_ARGS((obj)->deleted(), (err), "%s: param %d %s has already been freed", __func__, (num), _XBT_STRINGIFY(obj));
#define CHECK_COMM(num)                                                                                                \
  CHECK_COMM2((num), comm)                                                                                             \
  CHECK_DELETED((num), MPI_ERR_COMM, comm)
#define CHECK_REQUEST(num)                                                                                             \
  CHECK_ARGS(request == nullptr, MPI_ERR_REQUEST,                                                                      \
             "%s: param %d request cannot be NULL",__func__, (num));
#define CHECK_BUFFER(num,buf,count)                                                                                    \
  CHECK_ARGS((buf) == nullptr && (count) > 0, MPI_ERR_BUFFER,                                                          \
             "%s: param %d %s cannot be NULL if %s > 0",__func__, (num), _XBT_STRINGIFY(buf), _XBT_STRINGIFY(count));
#define CHECK_COUNT(num, count)                                                                                        \
  CHECK_NEGATIVE((num), MPI_ERR_COUNT, (count))
#define CHECK_TYPE(num, datatype)                                                                                      \
  CHECK_ARGS(((datatype) == MPI_DATATYPE_NULL|| not (datatype)->is_valid()), MPI_ERR_TYPE,                             \
             "%s: param %d %s cannot be MPI_DATATYPE_NULL or invalid", __func__, (num), _XBT_STRINGIFY(datatype));
#define CHECK_OP(num, op, type)                                                                                        \
  CHECK_MPI_NULL((num), MPI_OP_NULL, MPI_ERR_OP, (op))                                                                 \
  CHECK_ARGS(((op)->allowed_types() && (((op)->allowed_types() & (type)->flags()) == 0)), MPI_ERR_OP,                \
             "%s: param %d op %s can't be applied to type %s", __func__, (num), _XBT_STRINGIFY(op), type->name());
#define CHECK_ROOT(num)\
  CHECK_ARGS((root < 0 || root >= comm->size()), MPI_ERR_ROOT,                                                         \
             "%s: param %d root (=%d) cannot be negative or larger than communicator size (=%d)", __func__, (num),     \
             root, comm->size());
#define CHECK_INFO(num,info)                                                                                           \
  CHECK_MPI_NULL((num), MPI_INFO_NULL, MPI_ERR_INFO, (info))
#define CHECK_TAG(num,tag)                                                                                             \
  CHECK_ARGS(((tag) < 0 && (tag) !=  MPI_ANY_TAG), MPI_ERR_TAG,                                                        \
             "%s: param %d %s (=%d) cannot be negative", __func__, (num), _XBT_STRINGIFY(tag), (tag));
#define CHECK_FILE(num, fh)                                                                                            \
  CHECK_MPI_NULL((num), MPI_FILE_NULL, MPI_ERR_FILE, (fh))
#define CHECK_OFFSET(num, offset)                                                                                      \
  CHECK_NEGATIVE((num), MPI_ERR_DISP, (offset))
#define CHECK_GROUP(num, group)                                                                                        \
  CHECK_MPI_NULL((num), MPI_GROUP_NULL, MPI_ERR_GROUP, (group))
#define CHECK_WIN(num, win)                                                                                            \
  CHECK_MPI_NULL((num), MPI_WIN_NULL, MPI_ERR_WIN, (win))
#define CHECK_RANK(num, rank, comm)                                                                                    \
  CHECK_ARGS(((rank) >= (comm)->size() || (rank) <0), MPI_ERR_RANK,                                                    \
             "%s: param %d %s (=%d) cannot be < 0 or > %d", __func__, (num), _XBT_STRINGIFY(rank),                     \
             (rank), (comm)->size() );
#define CHECK_PROC_RMA(num,proc,win)                                                                                   \
  CHECK_MPI_NULL((num), MPI_PROC_NULL, MPI_SUCCESS, (proc))                                                            \
  CHECK_RANK(num, proc, win->comm())
#define CHECK_NOT_IN_PLACE_ROOT(num, buf)                                                                                   \
  CHECK_ARGS((buf == MPI_IN_PLACE), MPI_ERR_BUFFER,                                                                    \
             "%s: param %d %s cannot be MPI_IN_PLACE for rank %d with root %d", __func__, (num), _XBT_STRINGIFY(buf),  \
             rank, root);
#define CHECK_NOT_IN_PLACE(num, buf)                                                                                   \
  CHECK_ARGS((buf == MPI_IN_PLACE), MPI_ERR_BUFFER,                                                                    \
             "%s: param %d %s cannot be MPI_IN_PLACE for rank %d", __func__, (num), _XBT_STRINGIFY(buf),  \
             rank);
#endif
