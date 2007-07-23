#define SMPI_DEFAULT_SPEED 100
#define SMPI_REQUEST_MALLOCATOR_SIZE 100
#define SMPI_MESSAGE_MALLOCATOR_SIZE 100

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

#include <xbt/function_types.h>
#include <simix/simix.h>

// MPI_Comm
struct smpi_mpi_communicator_t {
  int size;
  int barrier;
  smx_mutex_t barrier_mutex;
  smx_cond_t barrier_cond;
  smx_host_t *hosts;
  smx_process_t *processes;
};
typedef struct smpi_mpi_communicator_t smpi_mpi_communicator_t;
typedef smpi_mpi_communicator_t *MPI_Comm;
extern smpi_mpi_communicator_t smpi_mpi_comm_world;
#define MPI_COMM_WORLD (&smpi_mpi_comm_world)

// MPI_Status
struct smpi_mpi_status_t {
  int MPI_SOURCE;
};
typedef struct smpi_mpi_status_t smpi_mpi_status_t;
typedef smpi_mpi_status_t MPI_Status;
extern smpi_mpi_status_t smpi_mpi_status_ignore;
#define MPI_STATUS_IGNORE (&smpi_mpi_status_ignore)

// MPI_Datatype
struct smpi_mpi_datatype_t {
  size_t size;
};
typedef struct smpi_mpi_datatype_t smpi_mpi_datatype_t;
typedef smpi_mpi_datatype_t *MPI_Datatype;
// FIXME: add missing datatypes
extern smpi_mpi_datatype_t smpi_mpi_byte;
#define MPI_BYTE (&smpi_mpi_byte)
extern smpi_mpi_datatype_t smpi_mpi_int;
#define MPI_INT (&smpi_mpi_int)
extern smpi_mpi_datatype_t smpi_mpi_double;
#define MPI_DOUBLE (&smpi_mpi_double)

// MPI_Request
struct smpi_mpi_request_t {
	smpi_mpi_communicator_t *comm;
	int src;
	int dst;
	int tag;
	void *buf;
	int count;
	smpi_mpi_datatype_t *datatype;
	smx_mutex_t mutex;
	smx_cond_t cond;
	short int completed :1;
	xbt_fifo_t waitlist;
};
typedef struct smpi_mpi_request_t smpi_mpi_request_t;
typedef smpi_mpi_request_t *MPI_Request;

// smpi_received_message_t
struct smpi_received_message_t {
	smpi_mpi_communicator_t *comm;
	int src;
	int dst;
	int tag;
	void *buf;
};
typedef struct smpi_received_message_t smpi_received_message_t;

// MPI_Op
struct smpi_mpi_op_t {
  void (*func)(void *x, void *y, void *z);
};
typedef struct smpi_mpi_op_t smpi_mpi_op_t;
typedef smpi_mpi_op_t *MPI_Op;
extern smpi_mpi_op_t smpi_mpi_land;
#define MPI_LAND (&smpi_mpi_land)
extern smpi_mpi_op_t smpi_mpi_sum;
#define MPI_SUM (&smpi_mpi_sum)


// smpi functions
extern int smpi_simulated_main(int argc, char **argv);
unsigned int smpi_sleep(unsigned int);
void smpi_exit(int);
