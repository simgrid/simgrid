#include <stdio.h>

#include "private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi, XBT_LOG_ROOT_CAT, "All SMPI categories");

smpi_global_t     smpi_global     = NULL;

void *smpi_request_new(void);

void *smpi_request_new()
{
	smpi_mpi_request_t request = xbt_new(s_smpi_mpi_request_t, 1);

	request->completed = 0;
	request->mutex     = SIMIX_mutex_init();
	request->cond      = SIMIX_cond_init();

	return request;
}

void smpi_request_free(void *pointer);

void smpi_request_free(void *pointer)
{

	smpi_mpi_request_t request = pointer;

	if (NULL != request) {
		SIMIX_cond_destroy(request->cond);
		SIMIX_mutex_destroy(request->mutex);
		xbt_free(request);
	}

	return;
}

void smpi_request_reset(void *pointer);

void smpi_request_reset(void *pointer)
{
	smpi_mpi_request_t request = pointer;

	request->completed = 0;

	return;
}


void *smpi_message_new(void);

void *smpi_message_new()
{
	return xbt_new(s_smpi_received_message_t, 1);
}

void smpi_message_free(void *pointer);

void smpi_message_free(void *pointer)
{
	if (NULL != pointer) {
		xbt_free(pointer);
	}

	return;
}

void smpi_message_reset(void *pointer);

void smpi_message_reset(void *pointer)
{
	return;
}

int smpi_create_request(void *buf, int count, smpi_mpi_datatype_t datatype,
	int src, int dst, int tag, smpi_mpi_communicator_t comm, smpi_mpi_request_t *requestptr)
{
	int retval = MPI_SUCCESS;

	smpi_mpi_request_t request = NULL;

	// FIXME: make sure requestptr is not null
	if (NULL == buf) {
		retval = MPI_ERR_INTERN;
	} else if (0 > count) {
		retval = MPI_ERR_COUNT;
	} else if (NULL == datatype) {
		retval = MPI_ERR_TYPE;
	} else if (MPI_ANY_SOURCE != src && (0 > src || comm->size <= src)) {
		retval = MPI_ERR_RANK;
	} else if (0 > dst || comm->size <= dst) {
		retval = MPI_ERR_RANK;
	} else if (MPI_ANY_TAG != tag && 0 > tag) {
		retval = MPI_ERR_TAG;
	} else if (NULL == comm) {
		retval = MPI_ERR_COMM;
	} else if (NULL == requestptr) {
		retval = MPI_ERR_INTERN;
	} else {
		request           = xbt_mallocator_get(smpi_global->request_mallocator);
		request->comm     = comm;
		request->src      = src;
		request->dst      = dst;
		request->tag      = tag;
		request->buf      = buf;
		request->datatype = datatype;
		request->count    = count;

		*requestptr       = request;
	}
	return retval;
}

void smpi_global_init()
{
	int i;

	int size = SIMIX_host_get_number();

	smpi_global                                      = xbt_new(s_smpi_global_t, 1);

	// config variable
	smpi_global->reference_speed                     = SMPI_DEFAULT_SPEED;

	smpi_global->root_ready                          = 0;
	smpi_global->ready_process_count                 = 0;

	// start/stop
	smpi_global->start_stop_mutex                    = SIMIX_mutex_init();
	smpi_global->start_stop_cond                     = SIMIX_cond_init();

	// host info blank until sim starts
	// FIXME: is this okay?
	smpi_global->hosts                               = NULL;
	smpi_global->host_count                          = 0;

	// running hosts
	smpi_global->running_hosts_count_mutex           = SIMIX_mutex_init();
        smpi_global->running_hosts_count                 = 0;

	// mallocators
	smpi_global->request_mallocator                  = xbt_mallocator_new(SMPI_REQUEST_MALLOCATOR_SIZE,
	                                                     smpi_request_new, smpi_request_free, smpi_request_reset);
	smpi_global->message_mallocator                  = xbt_mallocator_new(SMPI_MESSAGE_MALLOCATOR_SIZE,
	                                                     smpi_message_new, smpi_message_free, smpi_message_reset);

	// queues
	smpi_global->pending_send_request_queues         = xbt_new(xbt_fifo_t,  size);
	smpi_global->pending_send_request_queues_mutexes = xbt_new(smx_mutex_t, size);
	smpi_global->pending_recv_request_queues         = xbt_new(xbt_fifo_t,  size);
	smpi_global->pending_recv_request_queues_mutexes = xbt_new(smx_mutex_t, size);
	smpi_global->received_message_queues             = xbt_new(xbt_fifo_t,  size);
	smpi_global->received_message_queues_mutexes     = xbt_new(smx_mutex_t, size);

	// sender/receiver processes
	smpi_global->sender_processes                    = xbt_new(smx_process_t, size);
	smpi_global->receiver_processes                  = xbt_new(smx_process_t, size);

	// timers
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

	smpi_global = NULL;
}

// FIXME: smarter algorithm?
int smpi_host_index()
{
	int i;
	smx_host_t host = SIMIX_host_self();

	for(i = smpi_global->host_count - 1; i > 0 && host != smpi_global->hosts[i]; i--);

	return i;
}

int smpi_run_simulation(int argc, char **argv)
{
	xbt_fifo_item_t cond_item   = NULL;
	smx_cond_t   cond           = NULL;
	xbt_fifo_item_t action_item = NULL;
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
		xbt_fifo_foreach(actions_failed, action_item, action, smx_action_t) {
			DEBUG1("** %s failed **", action->name);
			xbt_fifo_foreach(action->cond_list, cond_item, cond, smx_cond_t) {
				SIMIX_cond_broadcast(cond);
			}
		}
		xbt_fifo_foreach(actions_done, action_item, action, smx_action_t) {
			DEBUG1("** %s done **",action->name);
			xbt_fifo_foreach(action->cond_list, cond_item, cond, smx_cond_t) {
				SIMIX_cond_broadcast(cond);
			}
		}
	}

	// FIXME: cleanup incomplete
	xbt_fifo_free(actions_failed);
	xbt_fifo_free(actions_done);

	INFO1("simulation time %g", SIMIX_get_clock());

	smpi_global_destroy();

	SIMIX_clean();

	return 0;
}
