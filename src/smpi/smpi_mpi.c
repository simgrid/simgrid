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
		retval = smpi_mpi_comm_rank(comm, rank);
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
	int dst = 0;

	smpi_bench_end();

	//dst = smpi_mpi_comm_rank(comm);
	if (NULL == request) {
		retval = MPI_ERR_ARG;
	} else {
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

	// FIXME: necessary?
	//dst = smpi_mpi_comm_rank(comm);
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
	int src = 0;

	smpi_bench_end();

	//src = smpi_mpi_comm_rank(comm);
	if (NULL == request) {
		retval = MPI_ERR_ARG;
	} else {
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

	//src = smpi_mpi_comm_rank(comm);
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
