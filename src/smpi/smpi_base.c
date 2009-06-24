#include "private.h"
#include "xbt/time.h"

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

void smpi_process_init()
{
  smx_host_t host;
  int i;
  smpi_host_data_t hdata;

  smpi_global->running_hosts_count++;

  // initialize some local variables
  host = SIMIX_host_self();

  hdata = xbt_new(s_smpi_host_data_t, 1);
  SIMIX_host_set_data(host, hdata);

  for (i = 0; i < smpi_global->host_count && host != smpi_global->hosts[i]; i++);

  hdata->index = i;
  hdata->mutex = SIMIX_mutex_init();
  hdata->cond = SIMIX_cond_init();

  hdata->pending_recv_request_queue = xbt_fifo_new();

  hdata->main = SIMIX_process_self();
  hdata->sender = SIMIX_process_create("smpi_sender",
          smpi_sender, hdata,
          SIMIX_host_get_name(SIMIX_host_self()), 0, NULL,
          /*props */ NULL);
  hdata->receiver = SIMIX_process_create("smpi_receiver",
          smpi_receiver, hdata,
          SIMIX_host_get_name(SIMIX_host_self()), 0, NULL,
          /*props */ NULL);

  smpi_global->main_processes[hdata->index] = SIMIX_process_self();
  return;
}

void smpi_process_finalize()
{
  int i;
  smpi_host_data_t hdata =  SIMIX_host_get_data(SIMIX_host_self());

  i = --smpi_global->running_hosts_count;

  SIMIX_mutex_destroy(smpi_host_mutex());
  SIMIX_cond_destroy(smpi_host_cond());
  xbt_fifo_free(hdata->pending_recv_request_queue);

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
    xbt_fifo_push(smpi_global->pending_send_request_queues[index], request);

    if (SIMIX_process_is_suspended(smpi_global->sender_processes[index])) {
      SIMIX_process_resume(smpi_global->sender_processes[index]);
    }
  }

  return retval;
}

int smpi_mpi_irecv(smpi_mpi_request_t request)
{
  int retval = MPI_SUCCESS;
  smpi_host_data_t hdata =  SIMIX_host_get_data(SIMIX_host_self());

  if (NULL == request) {
    retval = MPI_ERR_INTERN;
  } else {
    xbt_fifo_push(hdata->pending_recv_request_queue, request);

    if (SIMIX_process_is_suspended(smpi_global->receiver_processes[hdata->index])) {
      SIMIX_process_resume(smpi_global->receiver_processes[hdata->index]);
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
