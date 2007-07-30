#include <xbt/function_types.h>
#include <simix/simix.h>

#define SMPI_RAND_SEED 5

#define MPI_ANY_SOURCE -1

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
struct smpi_mpi_communicator_t {
  int            size;
  smx_host_t    *hosts;

  smx_process_t *processes;
  int            barrier_count;
  smx_mutex_t    barrier_mutex;
  smx_cond_t     barrier_cond;
};
typedef struct smpi_mpi_communicator_t smpi_mpi_communicator_t;
typedef smpi_mpi_communicator_t *MPI_Comm;

// MPI_Status
struct smpi_mpi_status_t {
  int MPI_SOURCE;
};
typedef struct smpi_mpi_status_t smpi_mpi_status_t;
typedef smpi_mpi_status_t MPI_Status;

// MPI_Datatype
struct smpi_mpi_datatype_t {
  size_t size;
};
typedef struct smpi_mpi_datatype_t smpi_mpi_datatype_t;
typedef smpi_mpi_datatype_t *MPI_Datatype;

// MPI_Request
struct smpi_mpi_request_t {
	smpi_mpi_communicator_t *comm;
	int src;
	int dst;
	int tag;

	void *buf;
	smpi_mpi_datatype_t *datatype;
	int count;

	short int completed :1;
	smx_mutex_t mutex;
	smx_cond_t  cond;
	xbt_fifo_t waitlist;
};
typedef struct smpi_mpi_request_t smpi_mpi_request_t;
typedef smpi_mpi_request_t *MPI_Request;

// MPI_Op
struct smpi_mpi_op_t {
  void (*func)(void *x, void *y, void *z);
};
typedef struct smpi_mpi_op_t smpi_mpi_op_t;
typedef smpi_mpi_op_t *MPI_Op;

// global SMPI data structure
typedef struct SMPI_MPI_Global {

	smpi_mpi_communicator_t mpi_comm_world;

	smpi_mpi_status_t       mpi_status_ignore;

	smpi_mpi_datatype_t     mpi_byte;
	smpi_mpi_datatype_t     mpi_int;
	smpi_mpi_datatype_t     mpi_double;

	smpi_mpi_op_t           mpi_land;
	smpi_mpi_op_t           mpi_sum;

} s_SMPI_MPI_Global_t, *SMPI_MPI_Global_t;
exterm SMPI_MPI_Global_t smpi_mpi_global;

#define MPI_COMM_WORLD    (smpi_mpi_global->mpi_comm_world)

#define MPI_STATUS_IGNORE (smpi_mpi_global->mpi_status_ignore)

#define MPI_BYTE          (smpi_mpi_global->mpi_byte)
#define MPI_DOUBLE        (smpi_mpi_global->mpi_double)
#define MPI_INT           (smpi_mpi_global->mpi_int)

#define MPI_LAND          (smpi_mpi_global->mpi_land)
#define MPI_SUM           (smpi_mpi_glboal->mpi_sum)

// smpi functions
extern int smpi_simulated_main(int argc, char **argv);
unsigned int smpi_sleep(unsigned int);
void smpi_exit(int);
