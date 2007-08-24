#include <stdio.h>
#include <signal.h>
#include <sys/time.h>

#include "private.h"

SMPI_Global_t     smpi_global     = NULL;

SMPI_MPI_Global_t smpi_mpi_global = NULL;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi,XBT_LOG_ROOT_CAT, "All SMPI categories (see \ref SMPI_API)");

int inline smpi_mpi_comm_size(smpi_mpi_communicator_t *comm)
{
	return comm->size;
}

// FIXME: smarter algorithm?
int smpi_mpi_comm_rank(smpi_mpi_communicator_t *comm, smx_host_t host)
{
	int i;

	for(i = comm->size - 1; i > 0 && host != comm->simdata->hosts[i]; i--);

	return i;
}

int inline smpi_mpi_comm_rank_self(smpi_mpi_communicator_t *comm)
{
	return smpi_mpi_comm_rank(comm, SIMIX_host_self());
}

int smpi_sender(int argc, char **argv)
{
	smx_process_t self;
	smx_host_t shost;
	int rank;

	xbt_fifo_t request_queue;
	smx_mutex_t request_queue_mutex;
	int size;

	int running_hosts_count;

	smpi_mpi_request_t *request;

	smx_host_t dhost;

	smx_action_t action;

	smpi_received_message_t *message;

	int drank;

	smx_process_t receiver_process;

	self  = SIMIX_process_self();
	shost = SIMIX_host_self();
	rank  = smpi_mpi_comm_rank(smpi_mpi_global->mpi_comm_world, shost);

	// make sure root is done before own initialization
	SIMIX_mutex_lock(smpi_global->start_stop_mutex);
	if (!smpi_global->root_ready) {
		SIMIX_cond_wait(smpi_global->start_stop_cond, smpi_global->start_stop_mutex);
	}
	SIMIX_mutex_unlock(smpi_global->start_stop_mutex);

	request_queue       = smpi_global->pending_send_request_queues[rank];
	request_queue_mutex = smpi_global->pending_send_request_queues_mutexes[rank];
	size                = smpi_mpi_comm_size(smpi_mpi_global->mpi_comm_world);

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

			SIMIX_mutex_lock(request->simdata->mutex);

			message->comm = request->comm;
			message->src  = request->src;
			message->dst  = request->dst;
			message->tag  = request->tag;
			message->buf  = xbt_malloc(request->datatype->size * request->count);
			memcpy(message->buf, request->buf, request->datatype->size * request->count);

			dhost = request->comm->simdata->hosts[request->dst];
			drank = smpi_mpi_comm_rank(smpi_mpi_global->mpi_comm_world, dhost);

			SIMIX_mutex_lock(smpi_global->received_message_queues_mutexes[drank]);
			xbt_fifo_push(smpi_global->received_message_queues[drank], message);
			SIMIX_mutex_unlock(smpi_global->received_message_queues_mutexes[drank]);

			request->completed = 1;

			action = SIMIX_action_communicate(shost, dhost, NULL, request->datatype->size * request->count * 1.0, -1.0);

			SIMIX_register_action_to_condition(action, request->simdata->cond);

			SIMIX_cond_wait(request->simdata->cond, request->simdata->mutex);

			SIMIX_mutex_unlock(request->simdata->mutex);

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

	SIMIX_mutex_lock(smpi_global->start_stop_mutex);
	smpi_global->ready_process_count--;
	if (smpi_global->ready_process_count == 0) {
		SIMIX_cond_broadcast(smpi_global->start_stop_cond);
	} else if (smpi_global->ready_process_count < 0) {
		// FIXME: can't happen! abort!
	}
	SIMIX_mutex_unlock(smpi_global->start_stop_mutex);

	return 0;
}

int smpi_receiver(int argc, char **argv)
{
	smx_process_t self;
	int rank;

	xbt_fifo_t request_queue;
	smx_mutex_t request_queue_mutex;
	xbt_fifo_t message_queue;
	smx_mutex_t message_queue_mutex;
	int size;

	int running_hosts_count;

	smpi_mpi_request_t *request;
	smpi_received_message_t *message;

	xbt_fifo_item_t request_item;
	xbt_fifo_item_t message_item;

	self  = SIMIX_process_self();
	rank  = smpi_mpi_comm_rank_self(smpi_mpi_global->mpi_comm_world);

	// make sure root is done before own initialization
	SIMIX_mutex_lock(smpi_global->start_stop_mutex);
	if (!smpi_global->root_ready) {
		SIMIX_cond_wait(smpi_global->start_stop_cond, smpi_global->start_stop_mutex);
	}
	SIMIX_mutex_unlock(smpi_global->start_stop_mutex);

	request_queue       = smpi_global->pending_recv_request_queues[rank];
	request_queue_mutex = smpi_global->pending_recv_request_queues_mutexes[rank];
	message_queue       = smpi_global->received_message_queues[rank];
	message_queue_mutex = smpi_global->received_message_queues_mutexes[rank];
	size                = smpi_mpi_comm_size(smpi_mpi_global->mpi_comm_world);

	smpi_global->receiver_processes[rank] = self;

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
		request = NULL;
		message = NULL;

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
						(MPI_ANY_SOURCE == request->src || request->src == message->src) &&
						request->tag == message->tag) {
					xbt_fifo_remove_item(request_queue, request_item);
					xbt_fifo_remove_item(message_queue, message_item);
					goto stopsearch;
				}
			}
		}
stopsearch:
		SIMIX_mutex_unlock(message_queue_mutex);
		SIMIX_mutex_unlock(request_queue_mutex);

		if (NULL == request || NULL == message) {
			SIMIX_process_suspend(self);
		} else {
			SIMIX_mutex_lock(request->simdata->mutex);

			memcpy(request->buf, message->buf, request->datatype->size * request->count);
			request->src = message->src;
			request->completed = 1;
			SIMIX_cond_broadcast(request->simdata->cond);

			SIMIX_mutex_unlock(request->simdata->mutex);

			xbt_free(message->buf);
			xbt_mallocator_release(smpi_global->message_mallocator, message);
		}

		SIMIX_mutex_lock(smpi_global->running_hosts_count_mutex);
		running_hosts_count = smpi_global->running_hosts_count;
		SIMIX_mutex_unlock(smpi_global->running_hosts_count_mutex);

	} while (0 < running_hosts_count);

	SIMIX_mutex_lock(smpi_global->start_stop_mutex);
	smpi_global->ready_process_count--;
	if (smpi_global->ready_process_count == 0) {
		SIMIX_cond_broadcast(smpi_global->start_stop_cond);
	} else if (smpi_global->ready_process_count < 0) {
		// FIXME: can't happen, abort!
	}
	SIMIX_mutex_unlock(smpi_global->start_stop_mutex);

	return 0;
}

void *smpi_request_new()
{
	smpi_mpi_request_t *request = xbt_new(smpi_mpi_request_t, 1);

	request->completed = 0;
	request->simdata            = xbt_new(s_smpi_mpi_request_simdata_t, 1);
	request->simdata->mutex     = SIMIX_mutex_init();
	request->simdata->cond      = SIMIX_cond_init();

	return request;
}

void smpi_request_free(void *pointer)
{

	smpi_mpi_request_t *request = pointer;

	if (NULL != request) {
		SIMIX_cond_destroy(request->simdata->cond);
		SIMIX_mutex_destroy(request->simdata->mutex);
		xbt_free(request->simdata);
		xbt_free(request);
	}

	return;
}

void smpi_request_reset(void *pointer)
{
	return;
}


void *smpi_message_new()
{
	return xbt_new(smpi_received_message_t, 1);
}

void smpi_message_free(void *pointer)
{
	if (NULL != pointer) {
		xbt_free(pointer);
	}

	return;
}

void smpi_message_reset(void *pointer)
{
	return;
}

void smpi_global_init()
{
	int i;

	int size = SIMIX_host_get_number();

	smpi_global = xbt_new(s_SMPI_Global_t, 1);

	// config variable
	smpi_global->reference_speed                     = SMPI_DEFAULT_SPEED;

	smpi_global->root_ready                          = 0;
	smpi_global->ready_process_count                 = 0;

	// start/stop
	smpi_global->start_stop_mutex                    = SIMIX_mutex_init();
	smpi_global->start_stop_cond                     = SIMIX_cond_init();

	// processes
	smpi_global->sender_processes                    = xbt_new(smx_process_t, size);
	smpi_global->receiver_processes                  = xbt_new(smx_process_t, size);

	// running hosts
	smpi_global->running_hosts_count_mutex           = SIMIX_mutex_init();
        smpi_global->running_hosts_count                 = 0;

	// mallocators
	smpi_global->request_mallocator                  = xbt_mallocator_new(SMPI_REQUEST_MALLOCATOR_SIZE,
	                                                     smpi_request_new, smpi_request_free, smpi_request_reset);
	smpi_global->message_mallocator                  = xbt_mallocator_new(SMPI_MESSAGE_MALLOCATOR_SIZE,
	                                                     smpi_message_new, smpi_message_free, smpi_message_reset);

	//
	smpi_global->pending_send_request_queues         = xbt_new(xbt_fifo_t,  size);
	smpi_global->pending_send_request_queues_mutexes = xbt_new(smx_mutex_t, size);
	smpi_global->pending_recv_request_queues         = xbt_new(xbt_fifo_t,  size);
	smpi_global->pending_recv_request_queues_mutexes = xbt_new(smx_mutex_t, size);
	smpi_global->received_message_queues             = xbt_new(xbt_fifo_t,  size);
	smpi_global->received_message_queues_mutexes     = xbt_new(smx_mutex_t, size);
	smpi_global->timers                              = xbt_new(xbt_os_timer_t, size);
	smpi_global->timers_mutexes                      = xbt_new(smx_mutex_t, size);

	for(i = 0; i < size; i++) {
		smpi_global->pending_send_request_queues[i]         = xbt_fifo_new();
		smpi_global->pending_send_request_queues_mutexes[i] = SIMIX_mutex_init();
		smpi_global->pending_recv_request_queues[i]         = xbt_fifo_new();
		smpi_global->pending_recv_request_queues_mutexes[i] = SIMIX_mutex_init();
		smpi_global->received_message_queues[i]             = xbt_fifo_new();
		smpi_global->received_message_queues_mutexes[i]     = SIMIX_mutex_init();
		smpi_global->timers[i]                              = xbt_os_timer_new();
		smpi_global->timers_mutexes[i]                      = SIMIX_mutex_init();
	}

}

void smpi_global_destroy()
{
	int i;

	int size = SIMIX_host_get_number();

	// start/stop
	SIMIX_mutex_destroy(smpi_global->start_stop_mutex);
	SIMIX_cond_destroy(smpi_global->start_stop_cond);

	// processes
	xbt_free(smpi_global->sender_processes);
	xbt_free(smpi_global->receiver_processes);

	// running hosts
	SIMIX_mutex_destroy(smpi_global->running_hosts_count_mutex);

	// mallocators
	xbt_mallocator_free(smpi_global->request_mallocator);
	xbt_mallocator_free(smpi_global->message_mallocator);

	for(i = 0; i < size; i++) {
		xbt_fifo_free(smpi_global->pending_send_request_queues[i]);
		SIMIX_mutex_destroy(smpi_global->pending_send_request_queues_mutexes[i]);
		xbt_fifo_free(smpi_global->pending_recv_request_queues[i]);
		SIMIX_mutex_destroy(smpi_global->pending_recv_request_queues_mutexes[i]);
		xbt_fifo_free(smpi_global->received_message_queues[i]);
		SIMIX_mutex_destroy(smpi_global->received_message_queues_mutexes[i]);
		xbt_os_timer_free(smpi_global->timers[i]);
		SIMIX_mutex_destroy(smpi_global->timers_mutexes[i]);
	}

	xbt_free(smpi_global->pending_send_request_queues);
	xbt_free(smpi_global->pending_send_request_queues_mutexes);
	xbt_free(smpi_global->pending_recv_request_queues);
	xbt_free(smpi_global->pending_recv_request_queues_mutexes);
	xbt_free(smpi_global->received_message_queues);
	xbt_free(smpi_global->received_message_queues_mutexes);
	xbt_free(smpi_global->timers);
	xbt_free(smpi_global->timers_mutexes);

	xbt_free(smpi_global);
}

int smpi_run_simulation(int argc, char **argv)
{
	xbt_fifo_item_t cond_item   = NULL;
	smx_cond_t   cond           = NULL;
	smx_action_t action         = NULL;

	xbt_fifo_t   actions_failed = xbt_fifo_new();
	xbt_fifo_t   actions_done   = xbt_fifo_new();

	srand(SMPI_RAND_SEED);

	SIMIX_global_init(&argc, argv);

	SIMIX_function_register("smpi_simulated_main", smpi_simulated_main);
	SIMIX_function_register("smpi_sender",         smpi_sender);
	SIMIX_function_register("smpi_receiver",       smpi_receiver);

	// FIXME: ought to verify these files...
	SIMIX_create_environment(argv[1]);

	// must initialize globals between creating environment and launching app....
	smpi_global_init();

	SIMIX_launch_application(argv[2]);

	/* Prepare to display some more info when dying on Ctrl-C pressing */
	// FIXME: doesn't work
	//signal(SIGINT, inthandler);

	/* Clean IO before the run */
	fflush(stdout);
	fflush(stderr);

	while (SIMIX_solve(actions_done, actions_failed) != -1.0) {
		while ((action = xbt_fifo_pop(actions_failed))) {
			DEBUG1("** %s failed **", action->name);
			xbt_fifo_foreach(action->cond_list, cond_item, cond, smx_cond_t) {
				SIMIX_cond_broadcast(cond);
				SIMIX_unregister_action_to_condition(action, cond);
			}
			SIMIX_action_destroy(action);
		}
		while ((action = xbt_fifo_pop(actions_done))) {
			DEBUG1("** %s done **",action->name);
			xbt_fifo_foreach(action->cond_list, cond_item, cond, smx_cond_t) {
				SIMIX_cond_broadcast(cond);
				SIMIX_unregister_action_to_condition(action, cond);
			}
			SIMIX_action_destroy(action);
		}
	}

	xbt_fifo_free(actions_failed);
	xbt_fifo_free(actions_done);

	INFO1("simulation time %g", SIMIX_get_clock());

	smpi_global_destroy();

	SIMIX_clean();

	return 0;
}

void smpi_mpi_land_func(void *x, void *y, void *z)
{
	*(int *)z = *(int *)x && *(int *)y;
}

void smpi_mpi_sum_func(void *x, void *y, void *z)
{
	*(int *)z = *(int *)x + *(int *)y;
}


void smpi_mpi_init()
{
	smx_process_t process;
	smx_host_t host;
	smx_host_t *hosts;
	int size;

	SIMIX_mutex_lock(smpi_global->running_hosts_count_mutex);
	smpi_global->running_hosts_count++;
	SIMIX_mutex_unlock(smpi_global->running_hosts_count_mutex);

	// initialize some local variables
	process = SIMIX_process_self();
	host    = SIMIX_host_self();
	hosts   = SIMIX_host_get_table();
	size    = SIMIX_host_get_number();

	// node 0 sets the globals
	if (host == hosts[0]) {

		smpi_mpi_global                                = xbt_new(s_SMPI_MPI_Global_t, 1);

		// global communicator
		smpi_mpi_global->mpi_comm_world                         = xbt_new(smpi_mpi_communicator_t, 1);
		smpi_mpi_global->mpi_comm_world->size                   = size;
		smpi_mpi_global->mpi_comm_world->simdata                = xbt_new(s_smpi_mpi_communicator_simdata_t, 1);
		smpi_mpi_global->mpi_comm_world->simdata->barrier_count = 0;
		smpi_mpi_global->mpi_comm_world->simdata->barrier_mutex = SIMIX_mutex_init();
		smpi_mpi_global->mpi_comm_world->simdata->barrier_cond  = SIMIX_cond_init();
		smpi_mpi_global->mpi_comm_world->simdata->hosts         = hosts;
		smpi_mpi_global->mpi_comm_world->simdata->processes     = xbt_new(smx_process_t, size);
		smpi_mpi_global->mpi_comm_world->simdata->processes[0]  = process;

		// mpi datatypes
		smpi_mpi_global->mpi_byte                      = xbt_new(smpi_mpi_datatype_t, 1);
		smpi_mpi_global->mpi_byte->size                = (size_t)1;
		smpi_mpi_global->mpi_int                       = xbt_new(smpi_mpi_datatype_t, 1);
		smpi_mpi_global->mpi_int->size                 = sizeof(int);
		smpi_mpi_global->mpi_double                    = xbt_new(smpi_mpi_datatype_t, 1);
		smpi_mpi_global->mpi_double->size              = sizeof(double);

		// mpi operations
		smpi_mpi_global->mpi_land                      = xbt_new(smpi_mpi_op_t, 1);
		smpi_mpi_global->mpi_land->func                = smpi_mpi_land_func;
		smpi_mpi_global->mpi_sum                       = xbt_new(smpi_mpi_op_t, 1);
		smpi_mpi_global->mpi_sum->func                 = smpi_mpi_sum_func;

		// signal all nodes to perform initialization
		SIMIX_mutex_lock(smpi_global->start_stop_mutex);
		smpi_global->root_ready = 1;
		SIMIX_cond_broadcast(smpi_global->start_stop_cond);
		SIMIX_mutex_unlock(smpi_global->start_stop_mutex);

	} else {

		// make sure root is done before own initialization
		SIMIX_mutex_lock(smpi_global->start_stop_mutex);
		if (!smpi_global->root_ready) {
			SIMIX_cond_wait(smpi_global->start_stop_cond, smpi_global->start_stop_mutex);
		}
		SIMIX_mutex_unlock(smpi_global->start_stop_mutex);

		smpi_mpi_global->mpi_comm_world->simdata->processes[smpi_mpi_comm_rank_self(smpi_mpi_global->mpi_comm_world)] = process;
	}

	// wait for all nodes to signal initializatin complete
	SIMIX_mutex_lock(smpi_global->start_stop_mutex);
	smpi_global->ready_process_count++;
	if (smpi_global->ready_process_count < 3 * size) {
		SIMIX_cond_wait(smpi_global->start_stop_cond, smpi_global->start_stop_mutex);
	} else {
		SIMIX_cond_broadcast(smpi_global->start_stop_cond);
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

	if (0 >= i) {

		// wake up senders/receivers
		for (i = 0; i < smpi_mpi_global->mpi_comm_world->size; i++) {
			if (SIMIX_process_is_suspended(smpi_global->sender_processes[i])) {
				SIMIX_process_resume(smpi_global->sender_processes[i]);
			}
			if (SIMIX_process_is_suspended(smpi_global->receiver_processes[i])) {
				SIMIX_process_resume(smpi_global->receiver_processes[i]);
			}
		}

		// wait for senders/receivers to exit...
		SIMIX_mutex_lock(smpi_global->start_stop_mutex);
		if (smpi_global->ready_process_count > 0) {
			SIMIX_cond_wait(smpi_global->start_stop_cond, smpi_global->start_stop_mutex);
		}
		SIMIX_mutex_unlock(smpi_global->start_stop_mutex);

		SIMIX_mutex_destroy(smpi_mpi_global->mpi_comm_world->simdata->barrier_mutex);
		SIMIX_cond_destroy(smpi_mpi_global->mpi_comm_world->simdata->barrier_cond);
		xbt_free(smpi_mpi_global->mpi_comm_world->simdata->processes);
		xbt_free(smpi_mpi_global->mpi_comm_world->simdata);
		xbt_free(smpi_mpi_global->mpi_comm_world);

		xbt_free(smpi_mpi_global->mpi_byte);
		xbt_free(smpi_mpi_global->mpi_int);
		xbt_free(smpi_mpi_global->mpi_double);

		xbt_free(smpi_mpi_global->mpi_land);
		xbt_free(smpi_mpi_global->mpi_sum);

		xbt_free(smpi_mpi_global);
	}

}

// FIXME: could cause trouble with multithreaded procs on same host...
void smpi_bench_begin()
{
	int rank = smpi_mpi_comm_rank_self(smpi_mpi_global->mpi_comm_world);
	SIMIX_mutex_lock(smpi_global->timers_mutexes[rank]);
	xbt_os_timer_start(smpi_global->timers[rank]);
	return;
}

void smpi_bench_end()
{
	int rank = smpi_mpi_comm_rank_self(smpi_mpi_global->mpi_comm_world);
	double duration;
	smx_host_t host;
	smx_action_t compute_action;
	smx_mutex_t mutex;
	smx_cond_t cond;

	xbt_os_timer_stop(smpi_global->timers[rank]);

	duration       = xbt_os_timer_elapsed(smpi_global->timers[rank]);
	SIMIX_mutex_unlock(smpi_global->timers_mutexes[rank]);

	host           = smpi_mpi_global->mpi_comm_world->simdata->hosts[rank];
	compute_action = SIMIX_action_execute(host, NULL, duration * SMPI_DEFAULT_SPEED);
	mutex          = SIMIX_mutex_init();
	cond           = SIMIX_cond_init();

	SIMIX_mutex_lock(mutex);
	SIMIX_register_action_to_condition(compute_action, cond);
	SIMIX_cond_wait(cond, mutex);
	//SIMIX_unregister_action_to_condition(compute_action, cond);
	SIMIX_mutex_unlock(mutex);

	SIMIX_mutex_destroy(mutex);
	SIMIX_cond_destroy(cond);

	// FIXME: check for success/failure?

	return;
}

void smpi_barrier(smpi_mpi_communicator_t *comm)
{

	SIMIX_mutex_lock(comm->simdata->barrier_mutex);
	if(++comm->simdata->barrier_count < comm->size) {
		SIMIX_cond_wait(comm->simdata->barrier_cond, comm->simdata->barrier_mutex);
	} else {
		comm->simdata->barrier_count = 0;
		SIMIX_cond_broadcast(comm->simdata->barrier_cond);
	}
	SIMIX_mutex_unlock(comm->simdata->barrier_mutex);

	return;
}

// FIXME: smarter algorithm...
int smpi_comm_rank(smpi_mpi_communicator_t *comm, smx_host_t host)
{
	int i;
	for(i = 0; i < comm->size && host != comm->simdata->hosts[i]; i++);
	if (i >= comm->size) i = -1;
	return i;
}

int smpi_create_request(void *buf, int count, smpi_mpi_datatype_t *datatype,
	int src, int dst, int tag, smpi_mpi_communicator_t *comm, smpi_mpi_request_t **request)
{
	int retval = MPI_SUCCESS;

	*request = NULL;

	if (0 > count) {
		retval = MPI_ERR_COUNT;
	} else if (NULL == buf) {
		retval = MPI_ERR_INTERN;
	} else if (NULL == datatype) {
		retval = MPI_ERR_TYPE;
	} else if (NULL == comm) {
		retval = MPI_ERR_COMM;
	} else if (MPI_ANY_SOURCE != src && (0 > src || comm->size <= src)) {
		retval = MPI_ERR_RANK;
	} else if (0 > dst || comm->size <= dst) {
		retval = MPI_ERR_RANK;
	} else if (0 > tag) {
		retval = MPI_ERR_TAG;
	} else {
		*request = xbt_mallocator_get(smpi_global->request_mallocator);
		(*request)->comm       = comm;
		(*request)->src        = src;
		(*request)->dst        = dst;
		(*request)->tag        = tag;
		(*request)->buf        = buf;
		(*request)->count      = count;
		(*request)->datatype   = datatype;
	}
	return retval;
}

int smpi_isend(smpi_mpi_request_t *request)
{
	int retval = MPI_SUCCESS;
	int rank   = smpi_mpi_comm_rank_self(smpi_mpi_global->mpi_comm_world);

	if (NULL != request) {
		SIMIX_mutex_lock(smpi_global->pending_send_request_queues_mutexes[rank]);
		xbt_fifo_push(smpi_global->pending_send_request_queues[rank], request);
		SIMIX_mutex_unlock(smpi_global->pending_send_request_queues_mutexes[rank]);
	}

	if (SIMIX_process_is_suspended(smpi_global->sender_processes[rank])) {
		SIMIX_process_resume(smpi_global->sender_processes[rank]);
	}

	return retval;
}

int smpi_irecv(smpi_mpi_request_t *request)
{
	int retval = MPI_SUCCESS;
	int rank = smpi_mpi_comm_rank_self(smpi_mpi_global->mpi_comm_world);

	if (NULL != request) {
		SIMIX_mutex_lock(smpi_global->pending_recv_request_queues_mutexes[rank]);
		xbt_fifo_push(smpi_global->pending_recv_request_queues[rank], request);
		SIMIX_mutex_unlock(smpi_global->pending_recv_request_queues_mutexes[rank]);
	}

	if (SIMIX_process_is_suspended(smpi_global->receiver_processes[rank])) {
		SIMIX_process_resume(smpi_global->receiver_processes[rank]);
	}

	return retval;
}

void smpi_wait(smpi_mpi_request_t *request, smpi_mpi_status_t *status)
{
	if (NULL != request) {
		SIMIX_mutex_lock(request->simdata->mutex);
		if (!request->completed) {
			SIMIX_cond_wait(request->simdata->cond, request->simdata->mutex);
		}
		if (NULL != status) {
			status->MPI_SOURCE = request->src;
		}
		SIMIX_mutex_unlock(request->simdata->mutex);
	}
}

// FIXME: move into own file
int smpi_gettimeofday(struct timeval *tv, struct timezone *tz)
{
	double now;
	int retval = 0;
	smpi_bench_end();
	if (NULL == tv) {
		retval = -1;
	} else {
		now = SIMIX_get_clock();
		tv->tv_sec  = now;
		tv->tv_usec = ((now - (double)tv->tv_sec) * 1000000.0);
	}
	smpi_bench_begin();
	return retval;
}

unsigned int smpi_sleep(unsigned int seconds)
{
	smx_mutex_t mutex;
	smx_cond_t cond;
	smx_host_t host;
	smx_action_t sleep_action;

	smpi_bench_end();
	host         = SIMIX_host_self();
	sleep_action = SIMIX_action_sleep(host, seconds);
	mutex        = SIMIX_mutex_init();
	cond         = SIMIX_cond_init();

	SIMIX_mutex_lock(mutex);
	SIMIX_register_action_to_condition(sleep_action, cond);
	SIMIX_cond_wait(cond, mutex);
	//SIMIX_unregister_action_to_condition(sleep_action, cond);
	SIMIX_mutex_unlock(mutex);

	SIMIX_mutex_destroy(mutex);
	SIMIX_cond_destroy(cond);

	// FIXME: check for success/failure?

	smpi_bench_begin();
	return 0;
}

void smpi_exit(int status)
{
	smpi_bench_end();
	SIMIX_mutex_lock(smpi_global->running_hosts_count_mutex);
	smpi_global->running_hosts_count--;
	SIMIX_mutex_unlock(smpi_global->running_hosts_count_mutex);
	SIMIX_process_kill(SIMIX_process_self());
	return;
}
