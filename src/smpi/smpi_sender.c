#include "private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_sender, smpi,
                                "Logging specific to SMPI (sender)");

int smpi_sender(int argc, char **argv)
{
  smx_process_t self;
  smx_host_t shost;

  int index;

  xbt_fifo_t request_queue;
  smx_mutex_t request_queue_mutex;

  int running_hosts_count;

  smpi_mpi_request_t request;

  smx_host_t dhost;

  smx_action_t action;

  e_surf_action_state_t state;

  smpi_received_message_t message;

  int dindex;

  smx_process_t receiver_process;

  self = SIMIX_process_self();
  shost = SIMIX_host_self();

  // make sure root is done before own initialization
  SIMIX_mutex_lock(smpi_global->start_stop_mutex);
  while (!smpi_global->root_ready) {
    SIMIX_cond_wait(smpi_global->start_stop_cond,
                    smpi_global->start_stop_mutex);
  }
  SIMIX_mutex_unlock(smpi_global->start_stop_mutex);

  index = smpi_host_index();

  request_queue = smpi_global->pending_send_request_queues[index];
  request_queue_mutex =
    smpi_global->pending_send_request_queues_mutexes[index];

  smpi_global->sender_processes[index] = self;

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

    SIMIX_mutex_lock(request_queue_mutex);
    request = xbt_fifo_shift(request_queue);
    SIMIX_mutex_unlock(request_queue_mutex);

    if (NULL == request) {
      SIMIX_process_suspend(self);
    } else {

      message = xbt_mallocator_get(smpi_global->message_mallocator);

      SIMIX_mutex_lock(request->mutex);

      message->comm = request->comm;
      message->src = request->comm->index_to_rank_map[index];
      message->tag = request->tag;
      message->data = request->data;
      message->buf = xbt_malloc(request->datatype->size * request->count);
      memcpy(message->buf, request->buf,
             request->datatype->size * request->count);

      dindex = request->comm->rank_to_index_map[request->dst];
      dhost = smpi_global->hosts[dindex];

      message->forward = (request->forward - 1) / 2;
      request->forward = request->forward / 2;

      if (0 < request->forward) {
        request->dst =
          (request->dst + message->forward + 1) % request->comm->size;
        SIMIX_mutex_lock(request_queue_mutex);
        xbt_fifo_push(request_queue, request);
        SIMIX_mutex_unlock(request_queue_mutex);
      } else {
        request->completed = 1;
      }

      action =
        SIMIX_action_communicate(shost, dhost, "communication",
                                 request->datatype->size * request->count,
                                 -1.0);

      SIMIX_register_action_to_condition(action, request->cond);

      for (state = SIMIX_action_get_state(action);
           state == SURF_ACTION_READY ||
           state == SURF_ACTION_RUNNING;
           state = SIMIX_action_get_state(action)
        ) {
        SIMIX_cond_wait(request->cond, request->mutex);
      }

      SIMIX_mutex_lock(smpi_global->received_message_queues_mutexes[dindex]);
      xbt_fifo_push(smpi_global->received_message_queues[dindex], message);
      SIMIX_mutex_unlock(smpi_global->received_message_queues_mutexes
                         [dindex]);

      SIMIX_unregister_action_to_condition(action, request->cond);
      SIMIX_action_destroy(action);

      SIMIX_mutex_unlock(request->mutex);

      // wake up receiver if necessary
      receiver_process = smpi_global->receiver_processes[dindex];
      if (SIMIX_process_is_suspended(receiver_process)) {
        SIMIX_process_resume(receiver_process);
      }

    }

    SIMIX_mutex_lock(smpi_global->running_hosts_count_mutex);
    running_hosts_count = smpi_global->running_hosts_count;
    SIMIX_mutex_unlock(smpi_global->running_hosts_count_mutex);

  } while (0 < running_hosts_count);

  return 0;
}
