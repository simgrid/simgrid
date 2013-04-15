#include "colls_private.h"

int flattree_segment_in_byte = 8192;

int
smpi_coll_tuned_bcast_flattree_pipeline(void *buff, int count,
                                        MPI_Datatype data_type, int root,
                                        MPI_Comm comm)
{
  int i, j, rank, num_procs;
  int tag = 1;

  MPI_Aint extent;
  extent = smpi_datatype_get_extent(data_type);

  int segment = flattree_segment_in_byte / extent;
  int pipe_length = count / segment;
  int increment = segment * extent;
  if (pipe_length==0) {
    XBT_WARN("MPI_bcast_flattree_pipeline use default MPI_bcast_flattree.");
    return smpi_coll_tuned_bcast_flattree(buff, count, data_type, root, comm);
  }
  rank = smpi_comm_rank(comm);
  num_procs = smpi_comm_size(comm);

  MPI_Request *request_array;
  MPI_Status *status_array;

  request_array = (MPI_Request *) xbt_malloc(pipe_length * sizeof(MPI_Request));
  status_array = (MPI_Status *) xbt_malloc(pipe_length * sizeof(MPI_Status));

  if (rank != root) {
    for (i = 0; i < pipe_length; i++) {
      request_array[i] = smpi_mpi_irecv((char *)buff + (i * increment), segment, data_type, root, tag, comm);
    }
    smpi_mpi_waitall(pipe_length, request_array, status_array);
  }

  else {
    // Root sends data to all others
    for (j = 0; j < num_procs; j++) {
      if (j == rank)
        continue;
      else {
        for (i = 0; i < pipe_length; i++) {
          smpi_mpi_send((char *)buff + (i * increment), segment, data_type, j, tag, comm);
        }
      }
    }

  }

  free(request_array);
  free(status_array);
  return MPI_SUCCESS;
}
