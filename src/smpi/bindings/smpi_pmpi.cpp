/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.hpp"
#include "simgrid/instr.h"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/version.h"
#include "smpi_comm.hpp"
#include "smpi_datatype_derived.hpp"
#include "smpi_status.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/smpi/include/smpi_actor.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_pmpi, smpi, "Logging specific to SMPI (pmpi)");

//this function need to be here because of the calls to smpi_bench
void TRACE_smpi_set_category(const char *category)
{
  //need to end bench otherwise categories for execution tasks are wrong
  smpi_bench_end();
  if (category != nullptr) {
    // declare category
    TRACE_category(category);
    smpi_process()->set_tracing_category(category);
  }
  //begin bench after changing process's category
  smpi_bench_begin();
}

/* PMPI User level calls */

int PMPI_Init(int*, char***)
{
  xbt_assert(simgrid::s4u::Engine::is_initialized(),
             "Your MPI program was not properly initialized. The easiest is to use smpirun to start it.");

  xbt_assert(not smpi_process()->initializing());
  xbt_assert(not smpi_process()->initialized());

  simgrid::smpi::ActorExt::init();
  int rank_traced = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_init(rank_traced, __func__);
  smpi_bench_begin();
  smpi_process()->mark_as_initialized();

  smpi_mpi_init();

  return MPI_SUCCESS;
}

int PMPI_Finalize()
{
  smpi_bench_end();
  int rank_traced = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_comm_in(rank_traced, __func__, new simgrid::instr::NoOpTIData("finalize"));

  smpi_process()->finalize();

  TRACE_smpi_comm_out(rank_traced);
  return MPI_SUCCESS;
}

int PMPI_Finalized(int* flag)
{
  *flag=smpi_process()!=nullptr ? smpi_process()->finalized() : 0;
  return MPI_SUCCESS;
}

int PMPI_Get_version (int *version,int *subversion){
  *version = MPI_VERSION;
  *subversion= MPI_SUBVERSION;
  return MPI_SUCCESS;
}

int PMPI_Get_library_version (char *version,int *len){
  smpi_bench_end();
  snprintf(version, MPI_MAX_LIBRARY_VERSION_STRING, "SMPI Version %d.%d. Copyright The SimGrid Team 2007-2020",
           SIMGRID_VERSION_MAJOR, SIMGRID_VERSION_MINOR);
  *len = strlen(version) > MPI_MAX_LIBRARY_VERSION_STRING ? MPI_MAX_LIBRARY_VERSION_STRING : strlen(version);
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Init_thread(int* argc, char*** argv, int /*required*/, int* provided)
{
  if (provided != nullptr) {
    *provided = MPI_THREAD_SINGLE;
  }
  return MPI_Init(argc, argv);
}

int PMPI_Query_thread(int *provided)
{
  if (provided == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *provided = MPI_THREAD_SINGLE;
    return MPI_SUCCESS;
  }
}

int PMPI_Is_thread_main(int *flag)
{
  // FIXME: The MPI standard seems to say that fatal errors need to be triggered
  // if MPI has been finalized or not yet been initialized
  if (flag == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *flag = simgrid::s4u::this_actor::get_pid() ==
            1; // FIXME: I don't think this is correct: This just returns true if the process ID is 1,
               // regardless of whether this process called MPI_Thread_Init() or not.
    return MPI_SUCCESS;
  }
}

int PMPI_Abort(MPI_Comm /*comm*/, int /*errorcode*/)
{
  smpi_bench_end();
  // FIXME: should kill all processes in comm instead
  XBT_WARN("MPI_Abort was called, something went probably wrong in this simulation ! Killing this process");
  smx_actor_t actor = SIMIX_process_self();
  simgrid::kernel::actor::simcall([actor] { actor->exit(); });
  return MPI_SUCCESS;
}

double PMPI_Wtime()
{
  return smpi_mpi_wtime();
}

extern double sg_maxmin_precision;
double PMPI_Wtick()
{
  return sg_maxmin_precision;
}

int PMPI_Address(const void* location, MPI_Aint* address)
{
  if (address==nullptr) {
    return MPI_ERR_ARG;
  } else {
    *address = reinterpret_cast<MPI_Aint>(location);
    return MPI_SUCCESS;
  }
}

int PMPI_Get_address(const void *location, MPI_Aint * address)
{
  return PMPI_Address(location, address);
}

int PMPI_Get_processor_name(char *name, int *resultlen)
{
  strncpy(name, sg_host_self()->get_cname(),
          strlen(sg_host_self()->get_cname()) < MPI_MAX_PROCESSOR_NAME - 1 ? strlen(sg_host_self()->get_cname()) + 1
                                                                           : MPI_MAX_PROCESSOR_NAME - 1);
  *resultlen = strlen(name) > MPI_MAX_PROCESSOR_NAME ? MPI_MAX_PROCESSOR_NAME : strlen(name);

  return MPI_SUCCESS;
}

int PMPI_Get_count(const MPI_Status * status, MPI_Datatype datatype, int *count)
{
  if (status == nullptr || count == nullptr) {
    return MPI_ERR_ARG;
  } else if (not datatype->is_valid()) {
    return MPI_ERR_TYPE;
  } else {
    size_t size = datatype->size();
    if (size == 0) {
      *count = 0;
      return MPI_SUCCESS;
    } else if (status->count % size != 0) {
      return MPI_UNDEFINED;
    } else {
      *count = simgrid::smpi::Status::get_count(status, datatype);
      return MPI_SUCCESS;
    }
  }
}

int PMPI_Initialized(int* flag) {
   *flag=(smpi_process()!=nullptr && smpi_process()->initialized());
   return MPI_SUCCESS;
}

int PMPI_Alloc_mem(MPI_Aint size, MPI_Info /*info*/, void* baseptr)
{
  void *ptr = xbt_malloc(size);
  if(ptr==nullptr)
    return MPI_ERR_NO_MEM;
  else {
    *static_cast<void**>(baseptr) = ptr;
    return MPI_SUCCESS;
  }
}

int PMPI_Free_mem(void *baseptr){
  xbt_free(baseptr);
  return MPI_SUCCESS;
}

int PMPI_Error_class(int errorcode, int* errorclass) {
  // assume smpi uses only standard mpi error codes
  *errorclass=errorcode;
  return MPI_SUCCESS;
}

int PMPI_Error_string(int errorcode, char* string, int* resultlen)
{
  static const char* smpi_error_string[] = {FOREACH_ERROR(GENERATE_STRING)};
  constexpr int nerrors                  = (sizeof smpi_error_string) / (sizeof smpi_error_string[0]);
  if (errorcode < 0 || errorcode >= nerrors || string == nullptr)
    return MPI_ERR_ARG;

  int len    = snprintf(string, MPI_MAX_ERROR_STRING, "%s", smpi_error_string[errorcode]);
  *resultlen = std::min(len, MPI_MAX_ERROR_STRING - 1);
  return MPI_SUCCESS;
}

int PMPI_Keyval_create(MPI_Copy_function* copy_fn, MPI_Delete_function* delete_fn, int* keyval, void* extra_state) {
  smpi_copy_fn _copy_fn={copy_fn,nullptr,nullptr,nullptr,nullptr,nullptr};
  smpi_delete_fn _delete_fn={delete_fn,nullptr,nullptr,nullptr,nullptr,nullptr};
  return simgrid::smpi::Keyval::keyval_create<simgrid::smpi::Comm>(_copy_fn, _delete_fn, keyval, extra_state);
}

int PMPI_Keyval_free(int* keyval) {
  return simgrid::smpi::Keyval::keyval_free<simgrid::smpi::Comm>(keyval);
}

MPI_Errhandler PMPI_Errhandler_f2c(MPI_Fint errhan){
  if(errhan==-1)
    return MPI_ERRHANDLER_NULL;
  return static_cast<MPI_Errhandler>(simgrid::smpi::Errhandler::f2c(errhan));
}

MPI_Fint PMPI_Errhandler_c2f(MPI_Errhandler errhan){
  if(errhan==MPI_ERRHANDLER_NULL)
    return -1;
  return errhan->c2f();
}

int PMPI_Buffer_attach(void *buf, int size){
  if(buf==nullptr)
    return MPI_ERR_BUFFER;
  if(size<0)
    return MPI_ERR_ARG;
  smpi_process()->set_bsend_buffer(buf, size);
  return MPI_SUCCESS;
}

int PMPI_Buffer_detach(void* buffer, int* size){
  smpi_process()->bsend_buffer((void**)buffer, size);
  smpi_process()->set_bsend_buffer(nullptr, 0);
  return MPI_SUCCESS;
}
