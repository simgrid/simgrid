#include "private.h"

SMPI_MPI_Global_t smpi_mpi_global = NULL;

void smpi_mpi_land_func(void *x, void *y, void *z)
{
	*(int *)z = *(int *)x && *(int *)y;
}

void smpi_mpi_sum_func(void *x, void *y, void *z)
{
	*(int *)z = *(int *)x + *(int *)y;
}

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
