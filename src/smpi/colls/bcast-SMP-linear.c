#include "colls_private.h"
#ifndef NUM_CORE
#define NUM_CORE 8
#endif

int bcast_SMP_linear_segment_byte = 8192;

int smpi_coll_tuned_bcast_SMP_linear(void *buf, int count,
                                     MPI_Datatype datatype, int root,
                                     MPI_Comm comm)
{
  int tag = COLL_TAG_BCAST;
  MPI_Status status;
  MPI_Request request;
  MPI_Request *request_array;
  MPI_Status *status_array;
  int rank, size;
  int i;
  MPI_Aint extent;
  extent = smpi_datatype_get_extent(datatype);

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);

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
    XBT_WARN("MPI_bcast_SMP_linear use default MPI_bcast.");	  	  
    smpi_mpi_bcast(buf, count, datatype, root, comm);
    return MPI_SUCCESS;            
  }
  // if root is not zero send to rank zero first
  if (root != 0) {
    if (rank == root)
      smpi_mpi_send(buf, count, datatype, 0, tag, comm);
    else if (rank == 0)
      smpi_mpi_recv(buf, count, datatype, root, tag, comm, &status);
  }
  // when a message is smaller than a block size => no pipeline 
  if (count <= segment) {
    // case ROOT
    if (rank == 0) {
      smpi_mpi_send(buf, count, datatype, to_inter, tag, comm);
      smpi_mpi_send(buf, count, datatype, to_intra, tag, comm);
    }
    // case last ROOT of each SMP
    else if (rank == (((size - 1) / NUM_CORE) * NUM_CORE)) {
      request = smpi_mpi_irecv(buf, count, datatype, from_inter, tag, comm);
      smpi_mpi_wait(&request, &status);
      smpi_mpi_send(buf, count, datatype, to_intra, tag, comm);
    }
    // case intermediate ROOT of each SMP
    else if (rank % NUM_CORE == 0) {
      request = smpi_mpi_irecv(buf, count, datatype, from_inter, tag, comm);
      smpi_mpi_wait(&request, &status);
      smpi_mpi_send(buf, count, datatype, to_inter, tag, comm);
      smpi_mpi_send(buf, count, datatype, to_intra, tag, comm);
    }
    // case last non-ROOT of each SMP
    else if (((rank + 1) % NUM_CORE == 0) || (rank == (size - 1))) {
      request = smpi_mpi_irecv(buf, count, datatype, from_intra, tag, comm);
      smpi_mpi_wait(&request, &status);
    }
    // case intermediate non-ROOT of each SMP
    else {
      request = smpi_mpi_irecv(buf, count, datatype, from_intra, tag, comm);
      smpi_mpi_wait(&request, &status);
      smpi_mpi_send(buf, count, datatype, to_intra, tag, comm);
    }
    return MPI_SUCCESS;
  }
  // pipeline bcast
  else {
    request_array =
        (MPI_Request *) xbt_malloc((size + pipe_length) * sizeof(MPI_Request));
    status_array =
        (MPI_Status *) xbt_malloc((size + pipe_length) * sizeof(MPI_Status));

    // case ROOT of each SMP
    if (rank % NUM_CORE == 0) {
      // case real root
      if (rank == 0) {
        for (i = 0; i < pipe_length; i++) {
          smpi_mpi_send((char *) buf + (i * increment), segment, datatype, to_inter,
                   (tag + i), comm);
          smpi_mpi_send((char *) buf + (i * increment), segment, datatype, to_intra,
                   (tag + i), comm);
        }
      }
      // case last ROOT of each SMP
      else if (rank == (((size - 1) / NUM_CORE) * NUM_CORE)) {
        for (i = 0; i < pipe_length; i++) {
          request_array[i] = smpi_mpi_irecv((char *) buf + (i * increment), segment, datatype,
                    from_inter, (tag + i), comm);
        }
        for (i = 0; i < pipe_length; i++) {
          smpi_mpi_wait(&request_array[i], &status);
          smpi_mpi_send((char *) buf + (i * increment), segment, datatype, to_intra,
                   (tag + i), comm);
        }
      }
      // case intermediate ROOT of each SMP
      else {
        for (i = 0; i < pipe_length; i++) {
          request_array[i] = smpi_mpi_irecv((char *) buf + (i * increment), segment, datatype,
                    from_inter, (tag + i), comm);
        }
        for (i = 0; i < pipe_length; i++) {
          smpi_mpi_wait(&request_array[i], &status);
          smpi_mpi_send((char *) buf + (i * increment), segment, datatype, to_inter,
                   (tag + i), comm);
          smpi_mpi_send((char *) buf + (i * increment), segment, datatype, to_intra,
                   (tag + i), comm);
        }
      }
    } else {                    // case last non-ROOT of each SMP
      if (((rank + 1) % NUM_CORE == 0) || (rank == (size - 1))) {
        for (i = 0; i < pipe_length; i++) {
          request_array[i] = smpi_mpi_irecv((char *) buf + (i * increment), segment, datatype,
                    from_intra, (tag + i), comm);
        }
        for (i = 0; i < pipe_length; i++) {
          smpi_mpi_wait(&request_array[i], &status);
        }
      }
      // case intermediate non-ROOT of each SMP
      else {
        for (i = 0; i < pipe_length; i++) {
          request_array[i] = smpi_mpi_irecv((char *) buf + (i * increment), segment, datatype,
                    from_intra, (tag + i), comm);
        }
        for (i = 0; i < pipe_length; i++) {
          smpi_mpi_wait(&request_array[i], &status);
          smpi_mpi_send((char *) buf + (i * increment), segment, datatype, to_intra,
                   (tag + i), comm);
        }
      }
    }
    free(request_array);
    free(status_array);
  }

  // when count is not divisible by block size, use default BCAST for the remainder
  if ((remainder != 0) && (count > segment)) {
    XBT_WARN("MPI_bcast_SMP_linear use default MPI_bcast.");	  	  	 
    smpi_mpi_bcast((char *) buf + (pipe_length * increment), remainder, datatype,
              root, comm);
  }

  return MPI_SUCCESS;
}
