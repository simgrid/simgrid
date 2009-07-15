#include "private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_receiver, smpi,
                                "Logging specific to SMPI (receiver)");

int smpi_receiver(int argc, char *argv[])
{
  smpi_process_data_t mydata = SIMIX_process_get_data(SIMIX_process_self());
  smx_process_t self;
  int index = mydata->index;

  xbt_fifo_t request_queue;
  xbt_fifo_t message_queue;

  smpi_mpi_request_t request;
  smpi_received_message_t message;

  xbt_fifo_item_t request_item;
  xbt_fifo_item_t message_item;

  self = SIMIX_process_self();

  request_queue = mydata->pending_recv_request_queue;
  message_queue = mydata->received_message_queue;

  DEBUG0("Up and running");


  while (1) {
    // FIXME: better algorithm, maybe some kind of balanced tree? or a heap?

    DEBUG0("Look for matching");
    xbt_fifo_foreach(request_queue, request_item, request, smpi_mpi_request_t) {
      xbt_fifo_foreach(message_queue, message_item, message,
                       smpi_received_message_t) {

//#define DEBUG_MATCH
#ifdef DEBUG_MATCH
        printf("[%s] try match (req_src=%d,msg_src=%d)x(req_tag=%d,msg_tag=%d)\n",
                        __FILE__,request->src,message->src,request->tag, message->tag);
#endif
        if (request->comm == message->comm &&
            (MPI_ANY_SOURCE == request->src || request->src == message->src)
            && (MPI_ANY_TAG == request->tag || request->tag == message->tag)) {
          xbt_fifo_remove_item(request_queue, request_item);
          xbt_fifo_free_item(request_item);
          xbt_fifo_remove_item(message_queue, message_item);
          xbt_fifo_free_item(message_item);
#ifdef DEBUG_MATCH
        printf("[%s] found match: req_src=%d,msg_src=%d)x(req_tag=%d,msg_tag=%d)\n",
                        __FILE__,request->src,message->src,request->tag, message->tag);
#endif
       DEBUG4("found match: (req_src=%d,msg_src=%d)x(req_tag=%d,msg_tag=%d)",
                        request->src,message->src,request->tag, message->tag);
          goto stopsearch;
        } else {
          DEBUG4("Matching fails: (req_src=%d,msg_src=%d)x(req_tag=%d,msg_tag=%d)",
                        request->src,message->src,request->tag, message->tag);
        }
      }
    }

    request = NULL;
    message = NULL;

  stopsearch:
    if (NULL != request) {
      if (NULL == message)
        DIE_IMPOSSIBLE;

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

    } else if (mydata->finalize > 0) {  /* main wants me to die and nothing to do */
      // FIXME: display the list of remaining requests and messages (user code synchronization faulty?)
      DEBUG0("Main wants me to die and I'm done. Bye, guys.");
      mydata->finalize--;
      SIMIX_cond_signal(mydata->cond);
      return 0;
    } else {
      DEBUG0("Nothing to do. Let's get a nap");
      SIMIX_process_suspend(self);
      DEBUG0("Uh? Someone called me?");
    }
  }

  return 0;
}
