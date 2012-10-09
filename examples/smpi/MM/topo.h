#ifndef topo_H_
#define topo_H_
#include <mpi.h>

/*!
 * \defgroup topo.h
 * file related to the plateform description.
 *
 * provide a communicator between the process on the same node*/
int split_comm_intra_node(MPI_Comm comm, MPI_Comm* comm_intra, int key);


int* get_index(MPI_Comm comm, MPI_Comm comm_intra);
char** get_names(MPI_Comm comm);

#endif /*topo_H_*/
