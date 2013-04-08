#include "colls_private.h"
int binary_pipeline_bcast_tree_height = 10;

int binary_pipeline_bcast_send_to[2][128] = {
  {1, 2, 3, -1, -1, 6, -1, -1, 9, 10, 11, -1, -1, 14, -1, 16, 17, 18, 19, -1,
   -1, 22, -1, 24, 25, 26, 27, -1, -1, 30, -1, -1, 33, 34, 35, -1, -1, 38, -1,
   -1, 41, 42, 43, -1, -1, 46, -1, -1, 49, 50, 51, -1, -1, 54, -1, -1, 57, 58,
   59, -1, -1, 62, -1, -1, 65, 66, 67, -1, -1, 70, -1, -1, 73, 74, 75, -1, -1,
   78, -1, 80, 81, 82, 83, -1, -1, 86, -1, -1, 89, 90, 91, -1, -1, 94, -1, -1,
   97, 98, 99, -1, -1, 102, -1, -1, 105, 106, 107, -1, -1, 110, -1, -1, 113,
   114, 115, -1, -1, 118, -1, -1, 121, 122, 123, -1, -1, 126, -1, -1},
  {8, 5, 4, -1, -1, 7, -1, -1, 15, 13, 12, -1, -1, -1, -1, 72, 23, 21, 20, -1,
   -1, -1, -1, 48, 32, 29, 28, -1, -1, 31, -1, -1, 40, 37, 36, -1, -1, 39, -1,
   -1, -1, 45, 44, -1, -1, 47, -1, -1, 56, 53, 52, -1, -1, 55, -1, -1, 64, 61,
   60, -1, -1, 63, -1, -1, -1, 69, 68, -1, -1, 71, -1, -1, 79, 77, 76, -1, -1,
   -1, -1, 104, 88, 85, 84, -1, -1, 87, -1, -1, 96, 93, 92, -1, -1, 95, -1, -1,
   -1, 101, 100, -1, -1, 103, -1, -1, 112, 109, 108, -1, -1, 111, -1, -1, 120,
   117, 116, -1, -1, 119, -1, -1, -1, 125, 124, -1, -1, 127, -1, -1}
};

int binary_pipeline_bcast_recv_from[128] =
    { -1, 0, 1, 2, 2, 1, 5, 5, 0, 8, 9, 10, 10, 9, 13, 8, 15, 16, 17, 18, 18,
17, 21, 16, 23, 24, 25, 26, 26, 25, 29, 29, 24, 32, 33, 34, 34, 33, 37, 37, 32, 40, 41, 42, 42, 41, 45, 45, 23,
48, 49, 50, 50, 49, 53, 53, 48, 56, 57, 58, 58, 57, 61, 61, 56, 64, 65, 66, 66, 65, 69, 69, 15, 72, 73, 74, 74,
73, 77, 72, 79, 80, 81, 82, 82, 81, 85, 85, 80, 88, 89, 90, 90, 89, 93, 93, 88, 96, 97, 98, 98, 97, 101, 101, 79,
104, 105, 106, 106, 105, 109, 109, 104, 112, 113, 114, 114, 113, 117, 117, 112, 120, 121, 122, 122, 121, 125,
125 };

int binary_pipeline_bcast_sequence[128] =
    { 0, 1, 2, 3, 3, 2, 3, 3, 1, 2, 3, 4, 4, 3, 4, 2, 3, 4, 5, 6, 6, 5, 6, 4, 5,
6, 7, 8, 8, 7, 8, 8, 6, 7, 8, 9, 9, 8, 9, 9, 7, 8, 9, 10, 10, 9, 10, 10, 5, 6, 7, 8, 8, 7, 8, 8, 6, 7, 8, 9, 9,
8, 9, 9, 7, 8, 9, 10, 10, 9, 10, 10, 3, 4, 5, 6, 6, 5, 6, 4, 5, 6, 7, 8, 8, 7, 8, 8, 6, 7, 8, 9, 9, 8, 9, 9, 7,
8, 9, 10, 10, 9, 10, 10, 5, 6, 7, 8, 8, 7, 8, 8, 6, 7, 8, 9, 9, 8, 9, 9, 7, 8, 9, 10, 10, 9, 10, 10 };


int bcast_TSB_segment_size_in_byte = 8192;

int smpi_coll_tuned_bcast_TSB(void *buf, int count, MPI_Datatype datatype,
                              int root, MPI_Comm comm)
{
  int tag = 5000;
  MPI_Status status;
  int rank, size;
  int i;

  MPI_Aint extent;
  extent = smpi_datatype_get_extent(datatype);

  rank = smpi_comm_rank(MPI_COMM_WORLD);
  size = smpi_comm_size(MPI_COMM_WORLD);

  /* source node and destination nodes (same through out the functions) */
  int to_left = binary_pipeline_bcast_send_to[0][rank];
  int to_right = binary_pipeline_bcast_send_to[1][rank];
  int from = binary_pipeline_bcast_recv_from[rank];

  /* segment is segment size in number of elements (not bytes) */
  int segment = bcast_TSB_segment_size_in_byte / extent;

  /* pipeline length */
  int pipe_length = count / segment;

  /* use for buffer offset for sending and receiving data = segment size in byte */
  int increment = segment * extent;

  /* if the input size is not divisible by segment size => 
     the small remainder will be done with native implementation */
  int remainder = count % segment;

  /* if root is not zero send to rank zero first */
  if (root != 0) {
    if (rank == root) {
      smpi_mpi_send(buf, count, datatype, 0, tag, comm);
    } else if (rank == 0) {
      smpi_mpi_recv(buf, count, datatype, root, tag, comm, &status);
    }
  }

  /* when a message is smaller than a block size => no pipeline */
  if (count <= segment) {

    /* case: root */
    if (rank == 0) {
      /* case root has only a left child */
      if (to_right == -1) {
        smpi_mpi_send(buf, count, datatype, to_left, tag, comm);
      }
      /* case root has both left and right children */
      else {
        smpi_mpi_send(buf, count, datatype, to_left, tag, comm);
        smpi_mpi_send(buf, count, datatype, to_right, tag, comm);
      }
    }

    /* case: leaf ==> receive only */
    else if (to_left == -1) {
      smpi_mpi_recv(buf, count, datatype, from, tag, comm, &status);
    }

    /* case: intermidiate node with only left child ==> relay message */
    else if (to_right == -1) {
      smpi_mpi_recv(buf, count, datatype, from, tag, comm, &status);
      smpi_mpi_send(buf, count, datatype, to_left, tag, comm);
    }

    /* case: intermidiate node with both left and right children ==> relay message */
    else {
      smpi_mpi_recv(buf, count, datatype, from, tag, comm, &status);
      smpi_mpi_send(buf, count, datatype, to_left, tag, comm);
      smpi_mpi_send(buf, count, datatype, to_right, tag, comm);
    }
    return MPI_SUCCESS;
  }
  // pipelining
  else {

    /* case: root */
    if (rank == 0) {
      /* case root has only a left child */
      if (to_right == -1) {
        for (i = 0; i < pipe_length; i++) {
          smpi_mpi_send((char *) buf + (i * increment), segment, datatype, to_left,
                   tag + i, comm);
        }
      }
      /* case root has both left and right children */
      else {
        for (i = 0; i < pipe_length; i++) {
          smpi_mpi_send((char *) buf + (i * increment), segment, datatype, to_left,
                   tag + i, comm);
          smpi_mpi_send((char *) buf + (i * increment), segment, datatype, to_right,
                   tag + i, comm);
        }
      }
    }

    /* case: leaf ==> receive only */
    else if (to_left == -1) {
      for (i = 0; i < pipe_length; i++) {
        smpi_mpi_recv((char *) buf + (i * increment), segment, datatype, from,
                 tag + i, comm, &status);
      }
    }

    /* case: intermidiate node with only left child ==> relay message */
    else if (to_right == -1) {
      for (i = 0; i < pipe_length; i++) {
        smpi_mpi_recv((char *) buf + (i * increment), segment, datatype, from,
                 tag + i, comm, &status);
        smpi_mpi_send((char *) buf + (i * increment), segment, datatype, to_left,
                 tag + i, comm);
      }
    }
    /* case: intermidiate node with both left and right children ==> relay message */
    else {
      for (i = 0; i < pipe_length; i++) {
        smpi_mpi_recv((char *) buf + (i * increment), segment, datatype, from,
                 tag + i, comm, &status);
        smpi_mpi_send((char *) buf + (i * increment), segment, datatype, to_left,
                 tag + i, comm);
        smpi_mpi_send((char *) buf + (i * increment), segment, datatype, to_right,
                 tag + i, comm);
      }
    }
  }

  /* when count is not divisible by block size, use default BCAST for the remainder */
  if ((remainder != 0) && (count > segment)) {
    XBT_WARN("MPI_bcast_TSB use default MPI_bcast.");
    smpi_mpi_bcast((char *) buf + (pipe_length * increment), remainder, datatype,
              root, comm);
  }

  return MPI_SUCCESS;
}
