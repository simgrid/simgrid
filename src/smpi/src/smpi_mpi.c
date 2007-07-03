#include <stdio.h>
#include <sys/time.h>
#include "smpi.h"

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
		*rank = smpi_comm_rank(comm, SIMIX_host_self());
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
	smpi_bench_end();
	smpi_barrier(comm);
	smpi_bench_begin();
	return MPI_SUCCESS;
}
