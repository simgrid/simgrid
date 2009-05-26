#include "private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_base, smpi,
                                "Logging specific to SMPI (base)");
XBT_LOG_EXTERNAL_CATEGORY(smpi_base);
XBT_LOG_EXTERNAL_CATEGORY(smpi_bench);
XBT_LOG_EXTERNAL_CATEGORY(smpi_kernel);
XBT_LOG_EXTERNAL_CATEGORY(smpi_mpi);
XBT_LOG_EXTERNAL_CATEGORY(smpi_receiver);
XBT_LOG_EXTERNAL_CATEGORY(smpi_sender);
XBT_LOG_EXTERNAL_CATEGORY(smpi_util);

smpi_mpi_global_t smpi_mpi_global = NULL;

void smpi_mpi_land_func(void *a, void *b, int *length,
                        MPI_Datatype * datatype);

void smpi_mpi_land_func(void *a, void *b, int *length,
                        MPI_Datatype * datatype)
{
  int i;
  if (*datatype == smpi_mpi_global->mpi_int) {
    int *x = a, *y = b;
    for (i = 0; i < *length; i++) {
      y[i] = x[i] && y[i];
    }
  }
}

void smpi_mpi_sum_func(void *a, void *b, int *length,
                       MPI_Datatype * datatype);

void smpi_mpi_sum_func(void *a, void *b, int *length, MPI_Datatype * datatype)
{
  int i;
  if (*datatype == smpi_mpi_global->mpi_int) {
    int *x = a, *y = b;
    for (i = 0; i < *length; i++) {
      y[i] = x[i] + y[i];
    }
  }
}

int smpi_mpi_comm_rank(smpi_mpi_communicator_t comm)
{
  return comm->index_to_rank_map[smpi_host_index()];
}

void smpi_mpi_init()
{
  smx_host_t host;
  smx_host_t *hosts;
  int host_count;
  int i;
  smpi_host_data_t hdata;

  SIMIX_mutex_lock(smpi_global->running_hosts_count_mutex);
  smpi_global->running_hosts_count++;
  SIMIX_mutex_unlock(smpi_global->running_hosts_count_mutex);

  // initialize some local variables
  host = SIMIX_host_self();
  hosts = SIMIX_host_get_table();
  host_count = SIMIX_host_get_number();

  hdata = xbt_new(s_smpi_host_data_t, 1);

  for (i = 0; i < host_count && host != hosts[i]; i++);

  hdata->index = i;
  hdata->mutex = SIMIX_mutex_init();
  hdata->cond = SIMIX_cond_init();

  SIMIX_host_set_data(host, hdata);

  // node 0 sets the globals
  if (0 == i) {

    smpi_global->hosts = hosts;
    smpi_global->host_count = host_count;

    smpi_mpi_global = xbt_new(s_smpi_mpi_global_t, 1);

    // global communicator
    smpi_mpi_global->mpi_comm_world = xbt_new(s_smpi_mpi_communicator_t, 1);
    smpi_mpi_global->mpi_comm_world->size = host_count;
    smpi_mpi_global->mpi_comm_world->barrier_count = 0;
    smpi_mpi_global->mpi_comm_world->barrier_mutex = SIMIX_mutex_init();
    smpi_mpi_global->mpi_comm_world->barrier_cond = SIMIX_cond_init();
    smpi_mpi_global->mpi_comm_world->rank_to_index_map =
      xbt_new(int, host_count);
    smpi_mpi_global->mpi_comm_world->index_to_rank_map =
      xbt_new(int, host_count);
    for (i = 0; i < host_count; i++) {
      smpi_mpi_global->mpi_comm_world->rank_to_index_map[i] = i;
      smpi_mpi_global->mpi_comm_world->index_to_rank_map[i] = i;
    }

    // mpi datatypes
    smpi_mpi_global->mpi_byte = xbt_new(s_smpi_mpi_datatype_t, 1);
    smpi_mpi_global->mpi_byte->size = (size_t) 1;
    smpi_mpi_global->mpi_int = xbt_new(s_smpi_mpi_datatype_t, 1);
    smpi_mpi_global->mpi_int->size = sizeof(int);
    smpi_mpi_global->mpi_double = xbt_new(s_smpi_mpi_datatype_t, 1);
    smpi_mpi_global->mpi_double->size = sizeof(double);

    // mpi operations
    smpi_mpi_global->mpi_land = xbt_new(s_smpi_mpi_op_t, 1);
    smpi_mpi_global->mpi_land->func = smpi_mpi_land_func;
    smpi_mpi_global->mpi_sum = xbt_new(s_smpi_mpi_op_t, 1);
    smpi_mpi_global->mpi_sum->func = smpi_mpi_sum_func;

    // signal all nodes to perform initialization
    SIMIX_mutex_lock(smpi_global->start_stop_mutex);
    smpi_global->root_ready = 1;
    SIMIX_cond_broadcast(smpi_global->start_stop_cond);
    SIMIX_mutex_unlock(smpi_global->start_stop_mutex);

  } else {

    // make sure root is done before own initialization
    SIMIX_mutex_lock(smpi_global->start_stop_mutex);
    while (!smpi_global->root_ready) {
      SIMIX_cond_wait(smpi_global->start_stop_cond,
                      smpi_global->start_stop_mutex);
    }
    SIMIX_mutex_unlock(smpi_global->start_stop_mutex);

  }

  // wait for all nodes to signal initializatin complete
  SIMIX_mutex_lock(smpi_global->start_stop_mutex);
  smpi_global->ready_process_count++;
  if (smpi_global->ready_process_count >= 3 * host_count) {
    SIMIX_cond_broadcast(smpi_global->start_stop_cond);
  }
  while (smpi_global->ready_process_count < 3 * host_count) {
    SIMIX_cond_wait(smpi_global->start_stop_cond,
                    smpi_global->start_stop_mutex);
  }
  SIMIX_mutex_unlock(smpi_global->start_stop_mutex);

  return;
}

void smpi_mpi_finalize()
{
  int i;

  SIMIX_mutex_lock(smpi_global->running_hosts_count_mutex);
  i = --smpi_global->running_hosts_count;
  SIMIX_mutex_unlock(smpi_global->running_hosts_count_mutex);

  SIMIX_mutex_lock(smpi_global->start_stop_mutex);
  smpi_global->ready_process_count--;
  SIMIX_mutex_unlock(smpi_global->start_stop_mutex);

  SIMIX_mutex_destroy(smpi_host_mutex());
  SIMIX_cond_destroy(smpi_host_cond());

  if (0 >= i) {

    // wake up senders/receivers
    for (i = 0; i < smpi_global->host_count; i++) {
      if (SIMIX_process_is_suspended(smpi_global->sender_processes[i])) {
        SIMIX_process_resume(smpi_global->sender_processes[i]);
      }
      if (SIMIX_process_is_suspended(smpi_global->receiver_processes[i])) {
        SIMIX_process_resume(smpi_global->receiver_processes[i]);
      }
    }

    SIMIX_mutex_destroy(smpi_mpi_global->mpi_comm_world->barrier_mutex);
    SIMIX_cond_destroy(smpi_mpi_global->mpi_comm_world->barrier_cond);
    xbt_free(smpi_mpi_global->mpi_comm_world);

    xbt_free(smpi_mpi_global->mpi_byte);
    xbt_free(smpi_mpi_global->mpi_int);
    xbt_free(smpi_mpi_global->mpi_double);

    xbt_free(smpi_mpi_global->mpi_land);
    xbt_free(smpi_mpi_global->mpi_sum);

    xbt_free(smpi_mpi_global);

  }

}

int smpi_mpi_barrier(smpi_mpi_communicator_t comm)
{

  SIMIX_mutex_lock(comm->barrier_mutex);
  ++comm->barrier_count;
  if (comm->barrier_count > comm->size) {       // only happens on second barrier...
    comm->barrier_count = 0;
  } else if (comm->barrier_count == comm->size) {
    SIMIX_cond_broadcast(comm->barrier_cond);
  }
  while (comm->barrier_count < comm->size) {
    SIMIX_cond_wait(comm->barrier_cond, comm->barrier_mutex);
  }
  SIMIX_mutex_unlock(comm->barrier_mutex);

  return MPI_SUCCESS;
}

int smpi_mpi_isend(smpi_mpi_request_t request)
{
  int retval = MPI_SUCCESS;
  int index = smpi_host_index();

  if (NULL == request) {
    retval = MPI_ERR_INTERN;
  } else {
    SIMIX_mutex_lock(smpi_global->pending_send_request_queues_mutexes[index]);
    xbt_fifo_push(smpi_global->pending_send_request_queues[index], request);
    SIMIX_mutex_unlock(smpi_global->pending_send_request_queues_mutexes
                       [index]);

    if (SIMIX_process_is_suspended(smpi_global->sender_processes[index])) {
      SIMIX_process_resume(smpi_global->sender_processes[index]);
    }
  }

  return retval;
}

int smpi_mpi_irecv(smpi_mpi_request_t request)
{
  int retval = MPI_SUCCESS;
  int index = smpi_host_index();

  if (NULL == request) {
    retval = MPI_ERR_INTERN;
  } else {
    SIMIX_mutex_lock(smpi_global->pending_recv_request_queues_mutexes[index]);
    xbt_fifo_push(smpi_global->pending_recv_request_queues[index], request);
    SIMIX_mutex_unlock(smpi_global->pending_recv_request_queues_mutexes
                       [index]);

    if (SIMIX_process_is_suspended(smpi_global->receiver_processes[index])) {
      SIMIX_process_resume(smpi_global->receiver_processes[index]);
    }
  }

  return retval;
}

int smpi_mpi_wait(smpi_mpi_request_t request, smpi_mpi_status_t * status)
{
  int retval = MPI_SUCCESS;

  if (NULL == request) {
    retval = MPI_ERR_INTERN;
  } else {
    SIMIX_mutex_lock(request->mutex);
    while (!request->completed) {
      SIMIX_cond_wait(request->cond, request->mutex);
    }
    if (NULL != status) {
      status->MPI_SOURCE = request->src;
      status->MPI_TAG = request->tag;
      status->MPI_ERROR = MPI_SUCCESS;
    }
    SIMIX_mutex_unlock(request->mutex);
  }

  return retval;
}
