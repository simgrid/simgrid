#include "private.h"

int smpi_sender(int argc, char **argv)
{
	smx_process_t self;
	smx_host_t shost;
	int rank;

	xbt_fifo_t request_queue;
	smx_mutex_t request_queue_mutex;
	int size;

	int running_hosts_count;

	smpi_mpi_request_t request;

	smx_host_t dhost;

	char communication[] = "communication";
	smx_action_t action;

	smpi_received_message_t message;

	int drank;

	smx_process_t receiver_process;

	self  = SIMIX_process_self();
	shost = SIMIX_host_self();

	// make sure root is done before own initialization
	SIMIX_mutex_lock(smpi_global->start_stop_mutex);
	if (!smpi_global->root_ready) {
		SIMIX_cond_wait(smpi_global->start_stop_cond, smpi_global->start_stop_mutex);
	}
	SIMIX_mutex_unlock(smpi_global->start_stop_mutex);

	rank = smpi_mpi_comm_rank(smpi_mpi_global->mpi_comm_world, shost);
	size = smpi_mpi_comm_size(smpi_mpi_global->mpi_comm_world);

	request_queue       = smpi_global->pending_send_request_queues[rank];
	request_queue_mutex = smpi_global->pending_send_request_queues_mutexes[rank];

	smpi_global->sender_processes[rank] = self;

	// wait for all nodes to signal initializatin complete
	SIMIX_mutex_lock(smpi_global->start_stop_mutex);
	smpi_global->ready_process_count++;
	if (smpi_global->ready_process_count < 3 * size) {
		SIMIX_cond_wait(smpi_global->start_stop_cond, smpi_global->start_stop_mutex);
	} else {
		SIMIX_cond_broadcast(smpi_global->start_stop_cond);
	}
	SIMIX_mutex_unlock(smpi_global->start_stop_mutex);

	do {

		SIMIX_mutex_lock(request_queue_mutex);
		request = xbt_fifo_shift(request_queue);
		SIMIX_mutex_unlock(request_queue_mutex);

		if (NULL == request) {
			SIMIX_process_suspend(self);
		} else {

			message       = xbt_mallocator_get(smpi_global->message_mallocator);

			SIMIX_mutex_lock(request->mutex);

			message->comm = request->comm;
			message->src  = request->src;
			message->dst  = request->dst;
			message->tag  = request->tag;
			message->buf  = xbt_malloc(request->datatype->size * request->count);
			memcpy(message->buf, request->buf, request->datatype->size * request->count);

			dhost = request->comm->hosts[request->dst];
			drank = smpi_mpi_comm_rank(smpi_mpi_global->mpi_comm_world, dhost);

			SIMIX_mutex_lock(smpi_global->received_message_queues_mutexes[drank]);
			xbt_fifo_push(smpi_global->received_message_queues[drank], message);
			SIMIX_mutex_unlock(smpi_global->received_message_queues_mutexes[drank]);

			request->completed = 1;

			action = SIMIX_action_communicate(shost, dhost, communication, request->datatype->size * request->count, -1.0);

			SIMIX_register_action_to_condition(action, request->cond);
			SIMIX_cond_wait(request->cond, request->mutex);
			SIMIX_unregister_action_to_condition(action, request->cond);

			SIMIX_mutex_unlock(request->mutex);

			//SIMIX_action_destroy(action);

			// wake up receiver if necessary
			receiver_process = smpi_global->receiver_processes[drank];
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
