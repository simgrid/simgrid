#include "colls.h"

int flattree_segment_in_byte = 8192;

int
smpi_coll_tuned_bcast_flattree_pipeline(void *buff, int count,
                                        MPI_Datatype data_type, int root,
                                        MPI_Comm comm)
{
  int i, j, rank, num_procs;
  int tag = 1;

  MPI_Aint extent;
  MPI_Type_extent(data_type, &extent);

  int segment = flattree_segment_in_byte / extent;
  int pipe_length = count / segment;
  int increment = segment * extent;

  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &num_procs);

  MPI_Request *request_array;
  MPI_Status *status_array;

  request_array = (MPI_Request *) malloc(pipe_length * sizeof(MPI_Request));
  status_array = (MPI_Status *) malloc(pipe_length * sizeof(MPI_Status));

  if (rank != root) {
    for (i = 0; i < pipe_length; i++) {
      MPI_Irecv((char *)buff + (i * increment), segment, data_type, root, tag, comm,
                &request_array[i]);
    }
    MPI_Waitall(pipe_length, request_array, status_array);
  }

  else {
    // Root sends data to all others
    for (j = 0; j < num_procs; j++) {
      if (j == rank)
        continue;
      else {
        for (i = 0; i < pipe_length; i++) {
          MPI_Send((char *)buff + (i * increment), segment, data_type, j, tag, comm);
        }
      }
    }

  }

  free(request_array);
  free(status_array);
  return MPI_SUCCESS;
}
