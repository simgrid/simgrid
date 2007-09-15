#ifndef SMPI_H
#define SMPI_H

#include <sys/time.h>
#include <xbt/function_types.h>

#define SMPI_RAND_SEED 5

#define MPI_ANY_SOURCE -1

#define MPI_ANY_TAG -1

#define MPI_UNDEFINED -1

// errorcodes
#define MPI_SUCCESS     0
#define MPI_ERR_COMM    1
#define MPI_ERR_ARG     2
#define MPI_ERR_TYPE    3
#define MPI_ERR_REQUEST 4
#define MPI_ERR_INTERN  5
#define MPI_ERR_COUNT   6
#define MPI_ERR_RANK    7
#define MPI_ERR_TAG     8

// MPI_Comm
typedef struct smpi_mpi_communicator_t *smpi_mpi_communicator_t;
typedef smpi_mpi_communicator_t MPI_Comm;

// MPI_Datatype
typedef struct smpi_mpi_datatype_t *smpi_mpi_datatype_t;
typedef smpi_mpi_datatype_t MPI_Datatype;

// MPI_Request
typedef struct smpi_mpi_request_t *smpi_mpi_request_t;
typedef smpi_mpi_request_t MPI_Request;

// MPI_Op
typedef struct smpi_mpi_op_t *smpi_mpi_op_t;
typedef smpi_mpi_op_t MPI_Op;

// MPI_Status
struct smpi_mpi_status_t {
  int MPI_SOURCE;
  int MPI_TAG;
  int MPI_ERROR;
};
typedef struct smpi_mpi_status_t smpi_mpi_status_t;
typedef smpi_mpi_status_t MPI_Status;

// global SMPI data structure
typedef struct smpi_mpi_global_t {

	smpi_mpi_communicator_t mpi_comm_world;

	smpi_mpi_datatype_t     mpi_byte;
	smpi_mpi_datatype_t     mpi_int;
	smpi_mpi_datatype_t     mpi_double;

	smpi_mpi_op_t           mpi_land;
	smpi_mpi_op_t           mpi_sum;

} s_smpi_mpi_global_t;
typedef struct smpi_mpi_global_t *smpi_mpi_global_t;
extern smpi_mpi_global_t smpi_mpi_global;

#define MPI_COMM_WORLD    (smpi_mpi_global->mpi_comm_world)
#define MPI_COMM_NULL     NULL

#define MPI_STATUS_IGNORE NULL

#define MPI_BYTE          (smpi_mpi_global->mpi_byte)
#define MPI_DOUBLE        (smpi_mpi_global->mpi_double)
#define MPI_INT           (smpi_mpi_global->mpi_int)

#define MPI_LAND          (smpi_mpi_global->mpi_land)
#define MPI_SUM           (smpi_mpi_glboal->mpi_sum)

// MPI Functions
int MPI_Init(int *argc, char ***argv);
int MPI_Finalize(void);
int MPI_Abort(MPI_Comm comm, int errorcode);
int MPI_Comm_size(MPI_Comm comm, int *size);
int MPI_Comm_rank(MPI_Comm comm, int *rank);
int MPI_Type_size(MPI_Datatype datatype, size_t *size);
int MPI_Barrier(MPI_Comm comm);
int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Status *status);
int MPI_Isend(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Send(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm);
int MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *comm_out);

// smpi functions
XBT_IMPORT_NO_EXPORT(int) smpi_simulated_main(int argc, char **argv);
unsigned int smpi_sleep(unsigned int);
void smpi_exit(int);
int smpi_gettimeofday(struct timeval *tv, struct timezone *tz);

#endif
