#include "private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_comm, smpi,
                                "Logging specific to SMPI (comm)");

typedef struct s_smpi_mpi_communicator {
  MPI_Group group;
} s_smpi_mpi_communicator_t;

MPI_Comm smpi_comm_new(MPI_Group group) {
  MPI_Comm comm;

  comm = xbt_new(s_smpi_mpi_communicator_t, 1);
  comm->group = group;
  smpi_group_use(comm->group);
  return comm;
}

void smpi_comm_destroy(MPI_Comm comm) {
  smpi_group_destroy(comm->group);
  xbt_free(comm);
}

MPI_Group smpi_comm_group(MPI_Comm comm) {
  return comm->group;
}

int smpi_comm_size(MPI_Comm comm) {
  return smpi_group_size(smpi_comm_group(comm));
}

int smpi_comm_rank(MPI_Comm comm) {
  return smpi_group_rank(smpi_comm_group(comm), smpi_process_index());
}
