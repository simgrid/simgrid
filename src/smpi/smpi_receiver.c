#include "private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_receiver, smpi,
                                "Logging specific to SMPI (receiver)");

int smpi_receiver(int argc, char **argv)
{
  smx_process_t self;
  int index;

  xbt_fifo_t request_queue;
  smx_mutex_t request_queue_mutex;
  xbt_fifo_t message_queue;
  smx_mutex_t message_queue_mutex;

  int running_hosts_count;

  smpi_mpi_request_t request;
  smpi_received_message_t message;

  xbt_fifo_item_t request_item;
  xbt_fifo_item_t message_item;

  self = SIMIX_process_self();

  // make sure root is done before own initialization
  SIMIX_mutex_lock(smpi_global->start_stop_mutex);
  while (!smpi_global->root_ready) {
    SIMIX_cond_wait(smpi_global->start_stop_cond,
                    smpi_global->start_stop_mutex);
  }
  SIMIX_mutex_unlock(smpi_global->start_stop_mutex);

  index = smpi_host_index();

  request_queue = smpi_global->pending_recv_request_queues[index];
  request_queue_mutex =
    smpi_global->pending_recv_request_queues_mutexes[index];
  message_queue = smpi_global->received_message_queues[index];
  message_queue_mutex = smpi_global->received_message_queues_mutexes[index];

  smpi_global->receiver_processes[index] = self;

  // wait for all nodes to signal initializatin complete
  SIMIX_mutex_lock(smpi_global->start_stop_mutex);
  smpi_global->ready_process_count++;
  if (smpi_global->ready_process_count >= 3 * smpi_global->host_count) {
    SIMIX_cond_broadcast(smpi_global->start_stop_cond);
  }
  while (smpi_global->ready_process_count < 3 * smpi_global->host_count) {
    SIMIX_cond_wait(smpi_global->start_stop_cond,
                    smpi_global->start_stop_mutex);
  }
  SIMIX_mutex_unlock(smpi_global->start_stop_mutex);

  do {

    // FIXME: better algorithm, maybe some kind of balanced tree? or a heap?

    // FIXME: not the best way to request multiple locks...
    SIMIX_mutex_lock(request_queue_mutex);
    SIMIX_mutex_lock(message_queue_mutex);
    for (request_item = xbt_fifo_get_first_item(request_queue);
         NULL != request_item;
         request_item = xbt_fifo_get_next_item(request_item)) {
      request = xbt_fifo_get_item_content(request_item);
      for (message_item = xbt_fifo_get_first_item(message_queue);
           NULL != message_item;
           message_item = xbt_fifo_get_next_item(message_item)) {
        message = xbt_fifo_get_item_content(message_item);
        if (request->comm == message->comm &&
            (MPI_ANY_SOURCE == request->src || request->src == message->src)
            && (MPI_ANY_TAG == request->tag || request->tag == message->tag)) {
          xbt_fifo_remove_item(request_queue, request_item);
          xbt_fifo_free_item(request_item);
          xbt_fifo_remove_item(message_queue, message_item);
          xbt_fifo_free_item(message_item);
          goto stopsearch;
        }
      }
    }

    request = NULL;
    message = NULL;

  stopsearch:
    SIMIX_mutex_unlock(message_queue_mutex);
    SIMIX_mutex_unlock(request_queue_mutex);

    if (NULL == request || NULL == message) {
      SIMIX_process_suspend(self);
    } else {

      SIMIX_mutex_lock(request->mutex);
      memcpy(request->buf, message->buf,
             request->datatype->size * request->count);
      request->src = message->src;
      request->data = message->data;
      request->forward = message->forward;

      if (0 == request->forward) {
        request->completed = 1;
        SIMIX_cond_broadcast(request->cond);
      } else {
        request->src = request->comm->index_to_rank_map[index];
        request->dst = (request->src + 1) % request->comm->size;
        smpi_mpi_isend(request);
      }

      SIMIX_mutex_unlock(request->mutex);

      xbt_free(message->buf);
      xbt_mallocator_release(smpi_global->message_mallocator, message);

    }

    SIMIX_mutex_lock(smpi_global->running_hosts_count_mutex);
    running_hosts_count = smpi_global->running_hosts_count;
    SIMIX_mutex_unlock(smpi_global->running_hosts_count_mutex);

  } while (0 < running_hosts_count);

  return 0;
}
