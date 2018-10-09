/* Copyright (c) 2007-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "smpi_comm.hpp"
#include "smpi_datatype_derived.hpp"
#include "smpi_status.hpp"
#include "src/simix/ActorImpl.hpp"
#include "src/smpi/include/smpi_actor.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_pmpi, smpi, "Logging specific to SMPI (pmpi)");

//this function need to be here because of the calls to smpi_bench
void TRACE_smpi_set_category(const char *category)
{
  //need to end bench otherwise categories for execution tasks are wrong
  smpi_bench_end();
  if (category != nullptr)
    TRACE_internal_smpi_set_category(category);
  //begin bench after changing process's category
  smpi_bench_begin();
}

/* PMPI User level calls */

int PMPI_Init(int *argc, char ***argv)
{
  xbt_assert(simgrid::s4u::Engine::is_initialized(),
             "Your MPI program was not properly initialized. The easiest is to use smpirun to start it.");
  // Init is called only once per SMPI process
  if (not smpi_process()->initializing()){
    simgrid::smpi::ActorExt::init(argc, argv);
  }
  if (not smpi_process()->initialized()){
    int rank = simgrid::s4u::this_actor::get_pid();
    TRACE_smpi_init(rank);
    TRACE_smpi_comm_in(rank, __func__, new simgrid::instr::NoOpTIData("init"));
    TRACE_smpi_comm_out(rank);
    TRACE_smpi_computing_init(rank);
    TRACE_smpi_sleeping_init(rank);
    smpi_bench_begin();
    smpi_process()->mark_as_initialized();
  }

  smpi_mpi_init();

  return MPI_SUCCESS;
}

int PMPI_Finalize()
{
  smpi_bench_end();
  int rank = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_comm_in(rank, __func__, new simgrid::instr::NoOpTIData("finalize"));

  smpi_process()->finalize();

  TRACE_smpi_comm_out(rank);
  TRACE_smpi_finalize(rank);
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
  snprintf(version, MPI_MAX_LIBRARY_VERSION_STRING, "SMPI Version %d.%d. Copyright The Simgrid Team 2007-2018",
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
  smx_actor_t process = SIMIX_process_self();
  simgrid::simix::simcall([process] { SIMIX_process_kill(process, process); });
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

int PMPI_Address(void *location, MPI_Aint * address)
{
  if (address==nullptr) {
    return MPI_ERR_ARG;
  } else {
    *address = reinterpret_cast<MPI_Aint>(location);
    return MPI_SUCCESS;
  }
}

int PMPI_Get_address(void *location, MPI_Aint * address)
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

int PMPI_Get_count(MPI_Status * status, MPI_Datatype datatype, int *count)
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

int PMPI_Error_string(int errorcode, char* string, int* resultlen){
  if (errorcode<0 || string ==nullptr){
    return MPI_ERR_ARG;
  } else {
    static const char *smpi_error_string[] = {
      FOREACH_ERROR(GENERATE_STRING)
    };
    *resultlen = strlen(smpi_error_string[errorcode]);
    strncpy(string, smpi_error_string[errorcode], *resultlen);
    return MPI_SUCCESS;  
  }
}

int PMPI_Keyval_create(MPI_Copy_function* copy_fn, MPI_Delete_function* delete_fn, int* keyval, void* extra_state) {
  smpi_copy_fn _copy_fn={copy_fn,nullptr,nullptr,nullptr,nullptr,nullptr};
  smpi_delete_fn _delete_fn={delete_fn,nullptr,nullptr,nullptr,nullptr,nullptr};
  return simgrid::smpi::Keyval::keyval_create<simgrid::smpi::Comm>(_copy_fn, _delete_fn, keyval, extra_state);
}

int PMPI_Keyval_free(int* keyval) {
  return simgrid::smpi::Keyval::keyval_free<simgrid::smpi::Comm>(keyval);
}
