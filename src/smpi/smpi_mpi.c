// FIXME: remove
#include <stdio.h>
#include "private.h"

int MPI_Init(int *argc, char ***argv)
{
	smpi_mpi_init();
	smpi_bench_begin();
	return MPI_SUCCESS;
}

int MPI_Finalize()
{
	smpi_bench_end();
	smpi_mpi_finalize();
	return MPI_SUCCESS;
}

// right now this just exits the current node, should send abort signal to all
// hosts in the communicator;
int MPI_Abort(MPI_Comm comm, int errorcode)
{
	smpi_exit(errorcode);
	return 0;
}

int MPI_Comm_size(MPI_Comm comm, int *size)
{
	int retval = MPI_SUCCESS;

	smpi_bench_end();

	if (NULL == comm) {
		retval = MPI_ERR_COMM;
	} else if (NULL == size) {
		retval = MPI_ERR_ARG;
	} else {
		*size = comm->size;
	}

	smpi_bench_begin();

	return retval;
}

int MPI_Comm_rank(MPI_Comm comm, int *rank)
{
	int retval = MPI_SUCCESS;

	smpi_bench_end();

	if (NULL == comm) {
		retval = MPI_ERR_COMM;
	} else if (NULL == rank) {
		retval = MPI_ERR_ARG;
	} else {
		*rank = smpi_mpi_comm_rank(comm);
	}

	smpi_bench_begin();

	return retval;
}

int MPI_Type_size(MPI_Datatype datatype, size_t *size)
{
	int retval = MPI_SUCCESS;

	smpi_bench_end();

	if (NULL == datatype) {
		retval = MPI_ERR_TYPE;
	} else if (NULL == size) {
		retval = MPI_ERR_ARG;
	} else {
		*size = datatype->size;
	}

	smpi_bench_begin();

	return retval;
}

int MPI_Barrier(MPI_Comm comm)
{
	int retval = MPI_SUCCESS;

	smpi_bench_end();

	if (NULL == comm) {
		retval = MPI_ERR_COMM;
	} else {
		retval = smpi_mpi_barrier(comm);
	}

	smpi_bench_begin();

	return retval;
}

int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Request *request)
{
	int retval = MPI_SUCCESS;

	smpi_bench_end();

	if (NULL == request) {
		retval = MPI_ERR_ARG;
	} else {
		int dst = 0;
		retval = smpi_create_request(buf, count, datatype, src, dst, tag, comm, request);
		if (NULL != *request && MPI_SUCCESS == retval) {
			retval = smpi_mpi_irecv(*request);
		}
	}

	smpi_bench_begin();

	return retval;
}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Status *status)
{
	int retval = MPI_SUCCESS;
	int dst = 0;
	smpi_mpi_request_t request;

	smpi_bench_end();

	retval = smpi_create_request(buf, count, datatype, src, dst, tag, comm, &request);
	if (NULL != request && MPI_SUCCESS == retval) {
		retval = smpi_mpi_irecv(request);
		if (MPI_SUCCESS == retval) {
			retval = smpi_mpi_wait(request, status);
		}
		xbt_mallocator_release(smpi_global->request_mallocator, request);
	}

	smpi_bench_begin();

	return retval;
}

int MPI_Isend(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request *request)
{
	int retval = MPI_SUCCESS;

	smpi_bench_end();

	if (NULL == request) {
		retval = MPI_ERR_ARG;
	} else {
		int src = 0;
		retval = smpi_create_request(buf, count, datatype, src, dst, tag, comm, request);
		if (NULL != *request && MPI_SUCCESS == retval) {
			retval = smpi_mpi_isend(*request);
		}
	}

	smpi_bench_begin();

	return retval;
}

int MPI_Send(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm)
{
	int retval = MPI_SUCCESS;
	int src = 0;
	smpi_mpi_request_t request;

	smpi_bench_end();

	retval = smpi_create_request(buf, count, datatype, src, dst, tag, comm, &request);
	if (NULL != request && MPI_SUCCESS == retval) {
		retval = smpi_mpi_isend(request);
		if (MPI_SUCCESS == retval) {
			smpi_mpi_wait(request, MPI_STATUS_IGNORE);
		}
		xbt_mallocator_release(smpi_global->request_mallocator, request);
	}

	smpi_bench_begin();

	return retval;
}

// FIXME: overly simplistic
int MPI_Bcast(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {

	int retval = MPI_SUCCESS;
	int rank;

	smpi_bench_end();

	rank = smpi_mpi_comm_rank(comm);

	if (rank == root) {
		int i;
		smpi_mpi_request_t *requests = xbt_new(smpi_mpi_request_t, comm->size - 1);
		for (i = 1; i < comm->size; i++) {
			retval = smpi_create_request(buf, count, datatype, root, (root + i) % comm->size, 0, comm, requests + i - 1);
			smpi_mpi_isend(requests[i - 1]);
		}
		for (i = 0; i < comm->size - 1; i++) {
			smpi_mpi_wait(requests[i], MPI_STATUS_IGNORE);
			xbt_mallocator_release(smpi_global->request_mallocator, requests[i]);
		}
		xbt_free(requests);
	} else {
		smpi_mpi_request_t request;
		retval = smpi_create_request(buf, count, datatype, root, rank, 0, comm, &request);
		smpi_mpi_irecv(request);
		smpi_mpi_wait(request, MPI_STATUS_IGNORE);
		xbt_mallocator_release(smpi_global->request_mallocator, request);
	}

	smpi_bench_begin();

	return retval;
}

// FIXME: needs to return null in event of MPI_UNDEFINED color...
// FIXME: seriously, this isn't pretty
int MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *comm_out)
{
	int retval = MPI_SUCCESS;

	int index, rank;
	smpi_mpi_request_t request;
	int colorkey[2];
	smpi_mpi_status_t status;

	smpi_bench_end();

	// FIXME: need to test parameters

	index = smpi_host_index();
	rank  = comm->index_to_rank_map[index];

	if (0 == rank) {
		int *colors = xbt_new(int, comm->size);
		int *keys   = xbt_new(int, comm->size);
		int i, j, k;
		smpi_mpi_communicator_t tempcomm = NULL;
		int colortmp;
		int keycount;
		int *keystmp  = xbt_new(int, comm->size);
		int *rankstmp = xbt_new(int, comm->size);
		int tmpval;
		int indextmp;

		colors[0] = color;
		keys[0]   = key;

		// FIXME: not efficient
		for (i = 1; i < comm->size; i++) {
			retval = smpi_create_request(colorkey, 2, MPI_INT, MPI_ANY_SOURCE, rank, MPI_ANY_TAG, comm, &request);
			smpi_mpi_irecv(request);
			smpi_mpi_wait(request, &status);
			xbt_mallocator_release(smpi_global->request_mallocator, request);
			colors[i] = colorkey[0];
			keys[i]   = colorkey[1];
		}

		for (i = 0; i < comm->size; i++) {
			if (-1 == colors[i]) {
				continue;
			}
			colortmp = colors[i];
			keycount = 0;
			for (j = i; j < comm->size; j++) {
				if(colortmp == colors[j]) {
					colors[j] = -1;
					keystmp[keycount] = keys[j];
					rankstmp[keycount] = j;
					keycount++;
				}
			}
			if (0 < keycount) {
				// FIXME: yes, mock me, bubble sort...
				for (j = 0; j < keycount; j++) {
					for (k = keycount - 1; k > j; k--) {
						if (keystmp[k] < keystmp[k - 1]) {
							tmpval          = keystmp[k];
							keystmp[k]      = keystmp[k - 1];
							keystmp[k - 1]  = tmpval;

							tmpval          = rankstmp[k];
							rankstmp[k]     = rankstmp[k - 1];
							rankstmp[k - 1] = tmpval;
						}
					}
				}
				tempcomm                    = xbt_new(s_smpi_mpi_communicator_t, 1);
				tempcomm->barrier_count     = 0;
				tempcomm->barrier_mutex     = SIMIX_mutex_init();
				tempcomm->barrier_cond      = SIMIX_cond_init();
				tempcomm->rank_to_index_map = xbt_new(int, keycount);
				tempcomm->index_to_rank_map = xbt_new(int, smpi_global->host_count);
				for (j = 0; j < smpi_global->host_count; j++) {
					tempcomm->index_to_rank_map[j] = -1;
				}
				for (j = 0; j < keycount; j++) {
					indextmp = comm->rank_to_index_map[rankstmp[j]];
					tempcomm->rank_to_index_map[j]        = indextmp;
					tempcomm->index_to_rank_map[indextmp] = j;
				}
				for (j = 0; j < keycount; j++) {
					retval = smpi_create_request(&j, 1, MPI_INT, 0, rankstmp[j], 0, comm, &request);
					request->data = tempcomm;
					smpi_mpi_isend(request);
					smpi_mpi_wait(request, &status);
					xbt_mallocator_release(smpi_global->request_mallocator, request);
				}
			}
		}
	} else {
		colorkey[0] = color;
		colorkey[1] = key;
		retval = smpi_create_request(colorkey, 2, MPI_INT, rank, 0, 0, comm, &request);
		smpi_mpi_isend(request);
		smpi_mpi_wait(request, &status);
		xbt_mallocator_release(smpi_global->request_mallocator, request);
		retval = smpi_create_request(colorkey, 1, MPI_INT, 0, rank, 0, comm, &request);
		smpi_mpi_irecv(request);
		smpi_mpi_wait(request, &status);
		*comm_out = request->data;
	}

	smpi_bench_begin();

	return retval;
}
