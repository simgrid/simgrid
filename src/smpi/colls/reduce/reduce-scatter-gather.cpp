/* Copyright (c) 2013-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"

/*
  reduce
  Author: MPICH
 */
namespace simgrid{
namespace smpi{
int reduce__scatter_gather(const void *sendbuf, void *recvbuf,
                           int count, MPI_Datatype datatype,
                           MPI_Op op, int root, MPI_Comm comm)
{
  MPI_Status status;
  int comm_size, rank, pof2, rem, newrank;
  int mask, *cnts, *disps, i, j, send_idx = 0;
  int recv_idx, last_idx = 0, newdst;
  int dst, send_cnt, recv_cnt, newroot, newdst_tree_root;
  int newroot_tree_root, new_count;
  int tag = COLL_TAG_REDUCE,temporary_buffer=0;
  unsigned char *send_ptr, *recv_ptr, *tmp_buf;

  cnts  = nullptr;
  disps = nullptr;

  MPI_Aint extent;

  if (count == 0)
    return 0;
  rank = comm->rank();
  comm_size = comm->size();



  extent = datatype->get_extent();
  /* If I'm not the root, then my recvbuf may not be valid, therefore
  I have to allocate a temporary one */
  if (rank != root && not recvbuf) {
    temporary_buffer=1;
    recvbuf = (void *)smpi_get_tmp_recvbuffer(count * extent);
  }
  /* find nearest power-of-two less than or equal to comm_size */
  pof2 = 1;
  while (pof2 <= comm_size)
    pof2 <<= 1;
  pof2 >>= 1;

  if (count < comm_size) {
    new_count = comm_size;
    send_ptr  = smpi_get_tmp_sendbuffer(new_count * extent);
    recv_ptr  = smpi_get_tmp_recvbuffer(new_count * extent);
    tmp_buf   = smpi_get_tmp_sendbuffer(new_count * extent);
    memcpy(send_ptr, sendbuf != MPI_IN_PLACE ? sendbuf : recvbuf, extent * count);

    //if ((rank != root))
    Request::sendrecv(send_ptr, new_count, datatype, rank, tag,
                 recv_ptr, new_count, datatype, rank, tag, comm, &status);

    rem = comm_size - pof2;
    if (rank < 2 * rem) {
      if (rank % 2 != 0) {
        /* odd */
        Request::send(recv_ptr, new_count, datatype, rank - 1, tag, comm);
        newrank = -1;
      } else {
        Request::recv(tmp_buf, count, datatype, rank + 1, tag, comm, &status);
        if(op!=MPI_OP_NULL) op->apply( tmp_buf, recv_ptr, &new_count, datatype);
        newrank = rank / 2;
      }
    } else                      /* rank >= 2*rem */
      newrank = rank - rem;

    cnts  = new int[pof2];
    disps = new int[pof2];

    if (newrank != -1) {
      for (i = 0; i < (pof2 - 1); i++)
        cnts[i] = new_count / pof2;
      cnts[pof2 - 1] = new_count - (new_count / pof2) * (pof2 - 1);

      disps[0] = 0;
      for (i = 1; i < pof2; i++)
        disps[i] = disps[i - 1] + cnts[i - 1];

      mask = 0x1;
      send_idx = recv_idx = 0;
      last_idx = pof2;
      while (mask < pof2) {
        newdst = newrank ^ mask;
        /* find real rank of dest */
        dst = (newdst < rem) ? newdst * 2 : newdst + rem;

        send_cnt = recv_cnt = 0;
        if (newrank < newdst) {
          send_idx = recv_idx + pof2 / (mask * 2);
          for (i = send_idx; i < last_idx; i++)
            send_cnt += cnts[i];
          for (i = recv_idx; i < send_idx; i++)
            recv_cnt += cnts[i];
        } else {
          recv_idx = send_idx + pof2 / (mask * 2);
          for (i = send_idx; i < recv_idx; i++)
            send_cnt += cnts[i];
          for (i = recv_idx; i < last_idx; i++)
            recv_cnt += cnts[i];
        }

        /* Send data from recvbuf. Recv into tmp_buf */
        Request::sendrecv(recv_ptr + disps[send_idx] * extent, send_cnt, datatype, dst, tag,
                          tmp_buf + disps[recv_idx] * extent, recv_cnt, datatype, dst, tag, comm, &status);

        /* tmp_buf contains data received in this step.
           recvbuf contains data accumulated so far */

        if (op != MPI_OP_NULL)
          op->apply(tmp_buf + disps[recv_idx] * extent, recv_ptr + disps[recv_idx] * extent, &recv_cnt, datatype);

        /* update send_idx for next iteration */
        send_idx = recv_idx;
        mask <<= 1;

        if (mask < pof2)
          last_idx = recv_idx + pof2 / mask;
      }
    }

    /* now do the gather to root */

    if (root < 2 * rem) {
      if (root % 2 != 0) {
        if (rank == root) {
          /* recv */
          for (i = 0; i < (pof2 - 1); i++)
            cnts[i] = new_count / pof2;
          cnts[pof2 - 1] = new_count - (new_count / pof2) * (pof2 - 1);

          disps[0] = 0;
          for (i = 1; i < pof2; i++)
            disps[i] = disps[i - 1] + cnts[i - 1];

          Request::recv(recv_ptr, cnts[0], datatype, 0, tag, comm, &status);

          newrank = 0;
          send_idx = 0;
          last_idx = 2;
        } else if (newrank == 0) {
          Request::send(recv_ptr, cnts[0], datatype, root, tag, comm);
          newrank = -1;
        }
        newroot = 0;
      } else
        newroot = root / 2;
    } else
      newroot = root - rem;

    if (newrank != -1) {
      j = 0;
      mask = 0x1;
      while (mask < pof2) {
        mask <<= 1;
        j++;
      }
      mask >>= 1;
      j--;
      while (mask > 0) {
        newdst = newrank ^ mask;

        /* find real rank of dest */
        dst = (newdst < rem) ? newdst * 2 : newdst + rem;

        if ((newdst == 0) && (root < 2 * rem) && (root % 2 != 0))
          dst = root;
        newdst_tree_root = newdst >> j;
        newdst_tree_root <<= j;

        newroot_tree_root = newroot >> j;
        newroot_tree_root <<= j;

        send_cnt = recv_cnt = 0;
        if (newrank < newdst) {
          /* update last_idx except on first iteration */
          if (mask != pof2 / 2)
            last_idx = last_idx + pof2 / (mask * 2);

          recv_idx = send_idx + pof2 / (mask * 2);
          for (i = send_idx; i < recv_idx; i++)
            send_cnt += cnts[i];
          for (i = recv_idx; i < last_idx; i++)
            recv_cnt += cnts[i];
        } else {
          recv_idx = send_idx - pof2 / (mask * 2);
          for (i = send_idx; i < last_idx; i++)
            send_cnt += cnts[i];
          for (i = recv_idx; i < send_idx; i++)
            recv_cnt += cnts[i];
        }

        if (newdst_tree_root == newroot_tree_root) {
          Request::send(recv_ptr + disps[send_idx] * extent, send_cnt, datatype, dst, tag, comm);
          break;
        } else {
          Request::recv(recv_ptr + disps[recv_idx] * extent, recv_cnt, datatype, dst, tag, comm, &status);
        }

        if (newrank > newdst)
          send_idx = recv_idx;

        mask >>= 1;
        j--;
      }
    }
    memcpy(recvbuf, recv_ptr, extent * count);
    smpi_free_tmp_buffer(send_ptr);
    smpi_free_tmp_buffer(recv_ptr);
  }


  else /* (count >= comm_size) */ {
    tmp_buf = smpi_get_tmp_sendbuffer(count * extent);

    //if ((rank != root))
    Request::sendrecv(sendbuf != MPI_IN_PLACE ? sendbuf : recvbuf, count, datatype, rank, tag,
                 recvbuf, count, datatype, rank, tag, comm, &status);

    rem = comm_size - pof2;
    if (rank < 2 * rem) {
      if (rank % 2 != 0) {      /* odd */
        Request::send(recvbuf, count, datatype, rank - 1, tag, comm);
        newrank = -1;
      }

      else {
        Request::recv(tmp_buf, count, datatype, rank + 1, tag, comm, &status);
        if(op!=MPI_OP_NULL) op->apply( tmp_buf, recvbuf, &count, datatype);
        newrank = rank / 2;
      }
    } else                      /* rank >= 2*rem */
      newrank = rank - rem;

    cnts  = new int[pof2];
    disps = new int[pof2];

    if (newrank != -1) {
      for (i = 0; i < (pof2 - 1); i++)
        cnts[i] = count / pof2;
      cnts[pof2 - 1] = count - (count / pof2) * (pof2 - 1);

      disps[0] = 0;
      for (i = 1; i < pof2; i++)
        disps[i] = disps[i - 1] + cnts[i - 1];

      mask = 0x1;
      send_idx = recv_idx = 0;
      last_idx = pof2;
      while (mask < pof2) {
        newdst = newrank ^ mask;
        /* find real rank of dest */
        dst = (newdst < rem) ? newdst * 2 : newdst + rem;

        send_cnt = recv_cnt = 0;
        if (newrank < newdst) {
          send_idx = recv_idx + pof2 / (mask * 2);
          for (i = send_idx; i < last_idx; i++)
            send_cnt += cnts[i];
          for (i = recv_idx; i < send_idx; i++)
            recv_cnt += cnts[i];
        } else {
          recv_idx = send_idx + pof2 / (mask * 2);
          for (i = send_idx; i < recv_idx; i++)
            send_cnt += cnts[i];
          for (i = recv_idx; i < last_idx; i++)
            recv_cnt += cnts[i];
        }

        /* Send data from recvbuf. Recv into tmp_buf */
        Request::sendrecv(static_cast<char*>(recvbuf) + disps[send_idx] * extent, send_cnt, datatype, dst, tag,
                          tmp_buf + disps[recv_idx] * extent, recv_cnt, datatype, dst, tag, comm, &status);

        /* tmp_buf contains data received in this step.
           recvbuf contains data accumulated so far */

        if (op != MPI_OP_NULL)
          op->apply(tmp_buf + disps[recv_idx] * extent, static_cast<char*>(recvbuf) + disps[recv_idx] * extent,
                    &recv_cnt, datatype);

        /* update send_idx for next iteration */
        send_idx = recv_idx;
        mask <<= 1;

        if (mask < pof2)
          last_idx = recv_idx + pof2 / mask;
      }
    }

    /* now do the gather to root */

    if (root < 2 * rem) {
      if (root % 2 != 0) {
        if (rank == root) {     /* recv */
          for (i = 0; i < (pof2 - 1); i++)
            cnts[i] = count / pof2;
          cnts[pof2 - 1] = count - (count / pof2) * (pof2 - 1);

          disps[0] = 0;
          for (i = 1; i < pof2; i++)
            disps[i] = disps[i - 1] + cnts[i - 1];

          Request::recv(recvbuf, cnts[0], datatype, 0, tag, comm, &status);

          newrank = 0;
          send_idx = 0;
          last_idx = 2;
        } else if (newrank == 0) {
          Request::send(recvbuf, cnts[0], datatype, root, tag, comm);
          newrank = -1;
        }
        newroot = 0;
      } else
        newroot = root / 2;
    } else
      newroot = root - rem;

    if (newrank != -1) {
      j = 0;
      mask = 0x1;
      while (mask < pof2) {
        mask <<= 1;
        j++;
      }
      mask >>= 1;
      j--;
      while (mask > 0) {
        newdst = newrank ^ mask;

        /* find real rank of dest */
        dst = (newdst < rem) ? newdst * 2 : newdst + rem;

        if ((newdst == 0) && (root < 2 * rem) && (root % 2 != 0))
          dst = root;
        newdst_tree_root = newdst >> j;
        newdst_tree_root <<= j;

        newroot_tree_root = newroot >> j;
        newroot_tree_root <<= j;

        send_cnt = recv_cnt = 0;
        if (newrank < newdst) {
          /* update last_idx except on first iteration */
          if (mask != pof2 / 2)
            last_idx = last_idx + pof2 / (mask * 2);

          recv_idx = send_idx + pof2 / (mask * 2);
          for (i = send_idx; i < recv_idx; i++)
            send_cnt += cnts[i];
          for (i = recv_idx; i < last_idx; i++)
            recv_cnt += cnts[i];
        } else {
          recv_idx = send_idx - pof2 / (mask * 2);
          for (i = send_idx; i < last_idx; i++)
            send_cnt += cnts[i];
          for (i = recv_idx; i < send_idx; i++)
            recv_cnt += cnts[i];
        }

        if (newdst_tree_root == newroot_tree_root) {
          Request::send((char *) recvbuf +
                   disps[send_idx] * extent,
                   send_cnt, datatype, dst, tag, comm);
          break;
        } else {
          Request::recv((char *) recvbuf +
                   disps[recv_idx] * extent,
                   recv_cnt, datatype, dst, tag, comm, &status);
        }

        if (newrank > newdst)
          send_idx = recv_idx;

        mask >>= 1;
        j--;
      }
    }
  }
  if (tmp_buf)
    smpi_free_tmp_buffer(tmp_buf);
  if (temporary_buffer == 1)
    smpi_free_tmp_buffer(static_cast<unsigned char*>(recvbuf));
  delete[] cnts;
  delete[] disps;

  return 0;
}
}
}
