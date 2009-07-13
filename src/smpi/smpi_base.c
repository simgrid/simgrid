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

/**
 * Operations of MPI_OP : implemented=land,sum,min,max
 **/
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

/**
 * sum two vectors element-wise
 *
 * @param a the first vectors
 * @param b the second vectors
 * @return the second vector is modified and contains the element-wise sums
 **/
void smpi_mpi_sum_func(void *a, void *b, int *length,
                       MPI_Datatype * datatype);

void smpi_mpi_sum_func(void *a, void *b, int *length, MPI_Datatype * datatype)
{
  int i;
  if ((*datatype == smpi_mpi_global->mpi_byte) || (*datatype == smpi_mpi_global->mpi_char)) {
    char *x = a, *y = b;
    for (i = 0; i < *length; i++) {
      y[i] = x[i] + y[i];
    }
  } else if (*datatype == smpi_mpi_global->mpi_int) {
    int *x = a, *y = b;
    for (i = 0; i < *length; i++) {
      y[i] = x[i] + y[i];
    }
  } else if (*datatype == smpi_mpi_global->mpi_float) {
    float *x = a, *y = b;
    for (i = 0; i < *length; i++) {
      y[i] = x[i] + y[i];
    }
  } else if (*datatype == smpi_mpi_global->mpi_double) {
    double *x = a, *y = b;
    for (i = 0; i < *length; i++) {
      y[i] = x[i] + y[i];
    }
  }
}
/**
 *i multiply two vectors element-wise
 *
 * @param a the first vectors
 * @param b the second vectors
 * @return the second vector is modified and contains the element-wise products
 **/
void smpi_mpi_prod_func(void *a, void *b, int *length,
                       MPI_Datatype * datatype);

void smpi_mpi_prod_func(void *a, void *b, int *length, MPI_Datatype * datatype)
{
  int i;
  if ((*datatype == smpi_mpi_global->mpi_byte) || (*datatype == smpi_mpi_global->mpi_char)) {
    char *x = a, *y = b;
    for (i = 0; i < *length; i++) {
      y[i] = x[i] * y[i];
    }
  } else if (*datatype == smpi_mpi_global->mpi_int) {
    int *x = a, *y = b;
    for (i = 0; i < *length; i++) {
      y[i] = x[i] * y[i];
    }
  } else if (*datatype == smpi_mpi_global->mpi_float) {
    float *x = a, *y = b;
    for (i = 0; i < *length; i++) {
      y[i] = x[i] * y[i];
    }
  } else if (*datatype == smpi_mpi_global->mpi_double) {
    double *x = a, *y = b;
    for (i = 0; i < *length; i++) {
      y[i] = x[i] * y[i];
    }
  }
}
/**
 * compute the min of two vectors element-wise
 **/
void smpi_mpi_min_func(void *a, void *b, int *length,
                       MPI_Datatype * datatype);

void smpi_mpi_min_func(void *a, void *b, int *length, MPI_Datatype * datatype)
{
  int i;
  if ((*datatype == smpi_mpi_global->mpi_byte) || (*datatype == smpi_mpi_global->mpi_char)) {
    char *x = a, *y = b;
    for (i = 0; i < *length; i++) {
      y[i] = x[i] < y[i] ? x[i] : y[i];
    }
  } else {
    if (*datatype == smpi_mpi_global->mpi_int) {
      int *x = a, *y = b;
      for (i = 0; i < *length; i++) {
        y[i] = x[i] < y[i] ? x[i] : y[i];
      }
    } else {
      if (*datatype == smpi_mpi_global->mpi_float) {
        float *x = a, *y = b;
        for (i = 0; i < *length; i++) {
          y[i] = x[i] < y[i] ? x[i] : y[i];
        }
      } else {
        if (*datatype == smpi_mpi_global->mpi_double) {
          double *x = a, *y = b;
          for (i = 0; i < *length; i++) {
            y[i] = x[i] < y[i] ? x[i] : y[i];
          }

        }
      }
    }
  }
}

/**
 * compute the max of two vectors element-wise
 **/
void smpi_mpi_max_func(void *a, void *b, int *length,
                       MPI_Datatype * datatype);

void smpi_mpi_max_func(void *a, void *b, int *length, MPI_Datatype * datatype)
{
  int i;
  if ((*datatype == smpi_mpi_global->mpi_byte) || (*datatype == smpi_mpi_global->mpi_char)) {
    char *x = a, *y = b;
    for (i = 0; i < *length; i++) {
      y[i] = x[i] > y[i] ? x[i] : y[i];
    }
  } else if (*datatype == smpi_mpi_global->mpi_int) {
    int *x = a, *y = b;
    for (i = 0; i < *length; i++) {
      y[i] = x[i] > y[i] ? x[i] : y[i];
    }
  } else if (*datatype == smpi_mpi_global->mpi_float) {
    float *x = a, *y = b;
    for (i = 0; i < *length; i++) {
      y[i] = x[i] > y[i] ? x[i] : y[i];
    }
  } else if (*datatype == smpi_mpi_global->mpi_double) {
    double *x = a, *y = b;
    for (i = 0; i < *length; i++) {
      y[i] = x[i] > y[i] ? x[i] : y[i];
    }

  }
}




/**
 * tell the MPI rank of the calling process (from its SIMIX process id)
 **/
int smpi_mpi_comm_rank(smpi_mpi_communicator_t comm)
{
  return comm->index_to_rank_map[smpi_process_index()];
}

void smpi_process_init(int *argc, char ***argv)
{
  smpi_process_data_t pdata;

  // initialize some local variables

  pdata = xbt_new(s_smpi_process_data_t, 1);
  SIMIX_process_set_data(SIMIX_process_self(), pdata);

  /* get rank from command line, and remove it from argv */
  pdata->index = atoi((*argv)[1]);
  DEBUG1("I'm rank %d", pdata->index);
  if (*argc > 2) {
    memmove((*argv)[1], (*argv)[2], sizeof(char *) * (*argc - 2));
    (*argv)[(*argc) - 1] = NULL;
  }
  (*argc)--;

  pdata->mutex = SIMIX_mutex_init();
  pdata->cond = SIMIX_cond_init();
  pdata->finalize = 0;

  pdata->pending_recv_request_queue = xbt_fifo_new();
  pdata->pending_send_request_queue = xbt_fifo_new();
  pdata->received_message_queue = xbt_fifo_new();

  pdata->main = SIMIX_process_self();
  pdata->sender = SIMIX_process_create("smpi_sender",
                                       smpi_sender, pdata,
                                       SIMIX_host_get_name(SIMIX_host_self()),
                                       0, NULL,
                                       /*props */ NULL);
  pdata->receiver = SIMIX_process_create("smpi_receiver",
                                         smpi_receiver, pdata,
                                         SIMIX_host_get_name(SIMIX_host_self
                                                             ()), 0, NULL,
                                         /*props */ NULL);

  smpi_global->main_processes[pdata->index] = SIMIX_process_self();
  return;
}

void smpi_process_finalize()
{
  smpi_process_data_t pdata = SIMIX_process_get_data(SIMIX_process_self());

  pdata->finalize = 2;          /* Tell sender and receiver to quit */
  SIMIX_process_resume(pdata->sender);
  SIMIX_process_resume(pdata->receiver);
  while (pdata->finalize > 0) { /* wait until it's done */
    SIMIX_cond_wait(pdata->cond, pdata->mutex);
  }

  SIMIX_mutex_destroy(pdata->mutex);
  SIMIX_cond_destroy(pdata->cond);
  xbt_fifo_free(pdata->pending_recv_request_queue);
  xbt_fifo_free(pdata->pending_send_request_queue);
  xbt_fifo_free(pdata->received_message_queue);
  xbt_free(pdata);
}


/*int smpi_mpi_barrier(smpi_mpi_communicator_t comm)
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
*/

int smpi_mpi_isend(smpi_mpi_request_t request)
{
  smpi_process_data_t pdata = SIMIX_process_get_data(SIMIX_process_self());
  int retval = MPI_SUCCESS;

  if (NULL == request) {
    retval = MPI_ERR_INTERN;
  } else {
    xbt_fifo_push(pdata->pending_send_request_queue, request);
    SIMIX_process_resume(pdata->sender);
  }

  return retval;
}

int smpi_mpi_irecv(smpi_mpi_request_t request)
{
  int retval = MPI_SUCCESS;
  smpi_process_data_t pdata = SIMIX_process_get_data(SIMIX_process_self());

  if (NULL == request) {
    retval = MPI_ERR_INTERN;
  } else {
    xbt_fifo_push(pdata->pending_recv_request_queue, request);

    if (SIMIX_process_is_suspended(pdata->receiver)) {
      SIMIX_process_resume(pdata->receiver);
    }
  }

  return retval;
}

void  print_req( smpi_mpi_request_t r ); 
void  print_req( smpi_mpi_request_t r ) {
        printf("***req %p-> src=%d dst=%d tag=%d completed=0x%x consumed=0x%x\n",r,r->src,r->dst,r->tag,r->completed,r->consumed);
}


/**
 * wait and friends ...
 **/
int smpi_mpi_wait(smpi_mpi_request_t request, smpi_mpi_status_t * status)
{
  int retval = MPI_SUCCESS;

  if (NULL == request) {
    retval = MPI_ERR_INTERN;
  } else {
    SIMIX_mutex_lock(request->mutex);

#ifdef DEBUG_STEPH
    print_req( request );  //@@
#endif
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

/**
 * waitall
 **/
int smpi_mpi_waitall(int count, smpi_mpi_request_t requests[],
                smpi_mpi_status_t status[])
{
        int cpt;
        int index;
        int retval;
        smpi_mpi_status_t stat;

        for (cpt = 0; cpt < count; cpt++) {
                retval = smpi_mpi_waitany(count, requests, &index, &stat);
                if (retval != MPI_SUCCESS)
                        return retval;
                if (MPI_STATUS_IGNORE != status)
                        memcpy(&(status[index]), &stat, sizeof(stat));
        }
        return MPI_SUCCESS;
}

/**
 * waitany
 **/
int smpi_mpi_waitany(int count, smpi_mpi_request_t * requests, int *index,
                     smpi_mpi_status_t * status)
{
  int cpt;

  *index = MPI_UNDEFINED;
  if (NULL == requests) {
    return MPI_ERR_INTERN;
  }
  /* First check if one of them is already done */
  for (cpt = 0; cpt < count; cpt++) {
#ifdef DEBUG_STEPH
          printf("...exam req[%d] of msg from [%d]\n",cpt,requests[cpt]->src);
#endif
    if (requests[cpt]->completed && !requests[cpt]->consumed) { /* got ya */
#ifdef DEBUG_STEPH
          printf("...found match req[%d] of msg from [%d]\n",cpt,requests[cpt]->src);
#endif
      *index = cpt;
      goto found_request;
    }
  }
  /* If none found, block */
  /* FIXME: should use a SIMIX_cond_waitany, when implemented. For now, block on the first one */
  while (1) {
    for (cpt = 0; cpt < count; cpt++) {

#ifdef DEBUG_STEPH
      print_req( requests[cpt] );
#endif
      if (!requests[cpt]->completed) {  /* this one is not done, wait on it */
#ifdef DEBUG_STEPH
              printf("... blocked waiting a msg %d->%d, tag=%d\n",requests[cpt]->src,requests[cpt]->dst,requests[cpt]->tag);
#endif
        while (!requests[cpt]->completed)
          SIMIX_cond_wait(requests[cpt]->cond, requests[cpt]->mutex);

        *index = cpt;
        goto found_request;
      }
    }
    if (cpt == count)           /* they are all done. Damn user */
      return MPI_ERR_REQUEST;
  }

found_request:
#ifdef DEBUG_STEPH
      print_req( requests[cpt] );
#endif
  requests[*index]->consumed = 1;
#ifdef DEBUG_STEPH
      print_req( requests[cpt] );
          printf("...accessing *req[%d]->consumed\n",cpt);
#endif
  if (NULL != status) {
    status->MPI_SOURCE = requests[*index]->src;
    status->MPI_TAG = requests[*index]->tag;
    status->MPI_ERROR = MPI_SUCCESS;
  }
  return MPI_SUCCESS;

}
