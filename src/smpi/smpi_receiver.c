#include "private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_receiver, smpi,
                                "Logging specific to SMPI (receiver)");

int smpi_receiver(int argc, char*argv[])
{
	smpi_host_data_t mydata = SIMIX_process_get_data(SIMIX_process_self());
  smx_process_t self;
  int index = mydata->index;

  xbt_fifo_t request_queue;
  xbt_fifo_t message_queue;

  int running_hosts_count;

  smpi_mpi_request_t request;
  smpi_received_message_t message;

  xbt_fifo_item_t request_item;
  xbt_fifo_item_t message_item;

  self = SIMIX_process_self();

  request_queue = smpi_global->pending_recv_request_queues[index];
  message_queue = smpi_global->received_message_queues[index];

  smpi_global->receiver_processes[index] = self;

  do {

    // FIXME: better algorithm, maybe some kind of balanced tree? or a heap?

	xbt_fifo_foreach(request_queue,request_item,request,smpi_mpi_request_t){
	  xbt_fifo_foreach(message_queue,message_item,message, smpi_received_message_t) {

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

    running_hosts_count = smpi_global->running_hosts_count;

  } while (0 < running_hosts_count);

  return 0;
}
