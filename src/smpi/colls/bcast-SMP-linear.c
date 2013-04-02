#include "colls.h"
#ifndef NUM_CORE
#define NUM_CORE 8
#endif

int bcast_SMP_linear_segment_byte = 8192;

int smpi_coll_tuned_bcast_SMP_linear(void *buf, int count,
                                     MPI_Datatype datatype, int root,
                                     MPI_Comm comm)
{
  int tag = 5000;
  MPI_Status status;
  MPI_Request request;
  MPI_Request *request_array;
  MPI_Status *status_array;
  int rank, size;
  int i;
  MPI_Aint extent;
  MPI_Type_extent(datatype, &extent);

  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);

  int segment = bcast_SMP_linear_segment_byte / extent;
  int pipe_length = count / segment;
  int remainder = count % segment;
  int increment = segment * extent;


  /* leader of each SMP do inter-communication
     and act as a root for intra-communication */
  int to_inter = (rank + NUM_CORE) % size;
  int to_intra = (rank + 1) % size;
  int from_inter = (rank - NUM_CORE + size) % size;
  int from_intra = (rank + size - 1) % size;

  // call native when MPI communication size is too small
  if (size <= NUM_CORE) {
    return MPI_Bcast(buf, count, datatype, root, comm);
  }
  // if root is not zero send to rank zero first
  if (root != 0) {
    if (rank == root)
      MPI_Send(buf, count, datatype, 0, tag, comm);
    else if (rank == 0)
      MPI_Recv(buf, count, datatype, root, tag, comm, &status);
  }
  // when a message is smaller than a block size => no pipeline 
  if (count <= segment) {
    // case ROOT
    if (rank == 0) {
      MPI_Send(buf, count, datatype, to_inter, tag, comm);
      MPI_Send(buf, count, datatype, to_intra, tag, comm);
    }
    // case last ROOT of each SMP
    else if (rank == (((size - 1) / NUM_CORE) * NUM_CORE)) {
      MPI_Irecv(buf, count, datatype, from_inter, tag, comm, &request);
      MPI_Wait(&request, &status);
      MPI_Send(buf, count, datatype, to_intra, tag, comm);
    }
    // case intermediate ROOT of each SMP
    else if (rank % NUM_CORE == 0) {
      MPI_Irecv(buf, count, datatype, from_inter, tag, comm, &request);
      MPI_Wait(&request, &status);
      MPI_Send(buf, count, datatype, to_inter, tag, comm);
      MPI_Send(buf, count, datatype, to_intra, tag, comm);
    }
    // case last non-ROOT of each SMP
    else if (((rank + 1) % NUM_CORE == 0) || (rank == (size - 1))) {
      MPI_Irecv(buf, count, datatype, from_intra, tag, comm, &request);
      MPI_Wait(&request, &status);
    }
    // case intermediate non-ROOT of each SMP
    else {
      MPI_Irecv(buf, count, datatype, from_intra, tag, comm, &request);
      MPI_Wait(&request, &status);
      MPI_Send(buf, count, datatype, to_intra, tag, comm);
    }
    return MPI_SUCCESS;
  }
  // pipeline bcast
  else {
    request_array =
        (MPI_Request *) malloc((size + pipe_length) * sizeof(MPI_Request));
    status_array =
        (MPI_Status *) malloc((size + pipe_length) * sizeof(MPI_Status));

    // case ROOT of each SMP
    if (rank % NUM_CORE == 0) {
      // case real root
      if (rank == 0) {
        for (i = 0; i < pipe_length; i++) {
          MPI_Send((char *) buf + (i * increment), segment, datatype, to_inter,
                   (tag + i), comm);
          MPI_Send((char *) buf + (i * increment), segment, datatype, to_intra,
                   (tag + i), comm);
        }
      }
      // case last ROOT of each SMP
      else if (rank == (((size - 1) / NUM_CORE) * NUM_CORE)) {
        for (i = 0; i < pipe_length; i++) {
          MPI_Irecv((char *) buf + (i * increment), segment, datatype,
                    from_inter, (tag + i), comm, &request_array[i]);
        }
        for (i = 0; i < pipe_length; i++) {
          MPI_Wait(&request_array[i], &status);
          MPI_Send((char *) buf + (i * increment), segment, datatype, to_intra,
                   (tag + i), comm);
        }
      }
      // case intermediate ROOT of each SMP
      else {
        for (i = 0; i < pipe_length; i++) {
          MPI_Irecv((char *) buf + (i * increment), segment, datatype,
                    from_inter, (tag + i), comm, &request_array[i]);
        }
        for (i = 0; i < pipe_length; i++) {
          MPI_Wait(&request_array[i], &status);
          MPI_Send((char *) buf + (i * increment), segment, datatype, to_inter,
                   (tag + i), comm);
          MPI_Send((char *) buf + (i * increment), segment, datatype, to_intra,
                   (tag + i), comm);
        }
      }
    } else {                    // case last non-ROOT of each SMP
      if (((rank + 1) % NUM_CORE == 0) || (rank == (size - 1))) {
        for (i = 0; i < pipe_length; i++) {
          MPI_Irecv((char *) buf + (i * increment), segment, datatype,
                    from_intra, (tag + i), comm, &request_array[i]);
        }
        for (i = 0; i < pipe_length; i++) {
          MPI_Wait(&request_array[i], &status);
        }
      }
      // case intermediate non-ROOT of each SMP
      else {
        for (i = 0; i < pipe_length; i++) {
          MPI_Irecv((char *) buf + (i * increment), segment, datatype,
                    from_intra, (tag + i), comm, &request_array[i]);
        }
        for (i = 0; i < pipe_length; i++) {
          MPI_Wait(&request_array[i], &status);
          MPI_Send((char *) buf + (i * increment), segment, datatype, to_intra,
                   (tag + i), comm);
        }
      }
    }
    free(request_array);
    free(status_array);
  }

  // when count is not divisible by block size, use default BCAST for the remainder
  if ((remainder != 0) && (count > segment)) {
    MPI_Bcast((char *) buf + (pipe_length * increment), remainder, datatype,
              root, comm);
  }

  return 1;
}
