/*!
 * get the information of which thread are on the same node
 *
 *
 * Authors: Quintin Jean-Noël
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "topo.h"

int split_comm_intra_node(MPI_Comm comm, MPI_Comm* comm_intra, int key) {
  // collect processor names
  char name[MPI_MAX_PROCESSOR_NAME];
  int len;
  int size;
  char** names = get_names(comm);
  int color = -1;
  int i = 0;

  MPI_Get_processor_name(name, &len);
  MPI_Comm_size(comm, &size);
  while (i < size){
    if (strcmp(name, names[i]) == 0) {
      break;
    }
    i++;
  }
  color = i;
  free(names);
  // split the communicator
  return MPI_Comm_split(comm, color, key, comm_intra);
}

char** get_names(MPI_Comm comm){
  char name[MPI_MAX_PROCESSOR_NAME];
  int len;
  int size;
  int i;
  char** friendly_names;
  char*  names;

  MPI_Get_processor_name(name, &len);
  MPI_Comm_size(comm, &size);
  friendly_names = malloc(sizeof(char*) * size);
  names          = malloc(sizeof(char) * MPI_MAX_PROCESSOR_NAME * size);

  MPI_Allgather(name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, names,
                MPI_MAX_PROCESSOR_NAME, MPI_CHAR, comm);

  for( i = 0; i < size;i++) friendly_names[i] = &names[MPI_MAX_PROCESSOR_NAME * i];
  return friendly_names;
}

int* get_index(MPI_Comm comm, MPI_Comm comm_intra){
  int rank = 0;
  int size;
  int* index;

  MPI_Comm_rank(comm_intra, &rank);
  MPI_Comm_size(comm, &size);
  index = (int*)malloc(sizeof(int) * size);
  MPI_Allgather(&rank, 1, MPI_INT, index, 1, MPI_INT, comm);
  return index;
}

