#include "colls_private.h"

/*
  reduce
  Author: MPICH
 */

int smpi_coll_tuned_reduce_scatter_gather(void *sendbuf, void *recvbuf,
                                          int count, MPI_Datatype datatype,
                                          MPI_Op op, int root, MPI_Comm comm)
{
  MPI_Status status;
  int comm_size, rank, type_size, pof2, rem, newrank;
  int mask, *cnts, *disps, i, j, send_idx = 0;
  int recv_idx, last_idx = 0, newdst;
  int dst, send_cnt, recv_cnt, newroot, newdst_tree_root;
  int newroot_tree_root, new_count;
  int tag = 4321;
  void *send_ptr, *recv_ptr, *tmp_buf;

  cnts = NULL;
  disps = NULL;

  MPI_Aint extent;

  if (count == 0)
    return 0;
  rank = smpi_comm_rank(comm);
  comm_size = smpi_comm_size(comm);

  extent = smpi_datatype_get_extent(datatype);
  type_size = smpi_datatype_size(datatype);

  /* find nearest power-of-two less than or equal to comm_size */
  pof2 = 1;
  while (pof2 <= comm_size)
    pof2 <<= 1;
  pof2 >>= 1;

  if (count < comm_size) {
    new_count = comm_size;
    send_ptr = (void *) xbt_malloc(new_count * extent);
    recv_ptr = (void *) xbt_malloc(new_count * extent);
    tmp_buf = (void *) xbt_malloc(new_count * extent);
    memcpy(send_ptr, sendbuf, extent * new_count);

    //if ((rank != root))
    smpi_mpi_sendrecv(send_ptr, new_count, datatype, rank, tag,
                 recv_ptr, new_count, datatype, rank, tag, comm, &status);

    rem = comm_size - pof2;
    if (rank < 2 * rem) {
      if (rank % 2 != 0) {
        /* odd */
        smpi_mpi_send(recv_ptr, new_count, datatype, rank - 1, tag, comm);
        newrank = -1;
      } else {
        smpi_mpi_recv(tmp_buf, count, datatype, rank + 1, tag, comm, &status);
        smpi_op_apply(op, tmp_buf, recv_ptr, &new_count, &datatype);
        newrank = rank / 2;
      }
    } else                      /* rank >= 2*rem */
      newrank = rank - rem;

    cnts = (int *) xbt_malloc(pof2 * sizeof(int));
    disps = (int *) xbt_malloc(pof2 * sizeof(int));

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
        smpi_mpi_sendrecv((char *) recv_ptr +
                     disps[send_idx] * extent,
                     send_cnt, datatype,
                     dst, tag,
                     (char *) tmp_buf +
                     disps[recv_idx] * extent,
                     recv_cnt, datatype, dst, tag, comm, &status);

        /* tmp_buf contains data received in this step.
           recvbuf contains data accumulated so far */

        smpi_op_apply(op, (char *) tmp_buf + disps[recv_idx] * extent,
                       (char *) recv_ptr + disps[recv_idx] * extent,
                       &recv_cnt, &datatype);

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

          smpi_mpi_recv(recv_ptr, cnts[0], datatype, 0, tag, comm, &status);

          newrank = 0;
          send_idx = 0;
          last_idx = 2;
        } else if (newrank == 0) {
          smpi_mpi_send(recv_ptr, cnts[0], datatype, root, tag, comm);
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
          smpi_mpi_send((char *) recv_ptr +
                   disps[send_idx] * extent,
                   send_cnt, datatype, dst, tag, comm);
          break;
        } else {
          smpi_mpi_recv((char *) recv_ptr +
                   disps[recv_idx] * extent,
                   recv_cnt, datatype, dst, tag, comm, &status);
        }

        if (newrank > newdst)
          send_idx = recv_idx;

        mask >>= 1;
        j--;
      }
    }
    memcpy(recvbuf, recv_ptr, extent * count);
    free(send_ptr);
    free(recv_ptr);
  }


  else if (count >= comm_size) {
    tmp_buf = (void *) xbt_malloc(count * extent);

    //if ((rank != root))
    smpi_mpi_sendrecv(sendbuf, count, datatype, rank, tag,
                 recvbuf, count, datatype, rank, tag, comm, &status);

    rem = comm_size - pof2;
    if (rank < 2 * rem) {
      if (rank % 2 != 0) {      /* odd */
        smpi_mpi_send(recvbuf, count, datatype, rank - 1, tag, comm);
        newrank = -1;
      }

      else {
        smpi_mpi_recv(tmp_buf, count, datatype, rank + 1, tag, comm, &status);
        smpi_op_apply(op, tmp_buf, recvbuf, &count, &datatype);
        newrank = rank / 2;
      }
    } else                      /* rank >= 2*rem */
      newrank = rank - rem;

    cnts = (int *) xbt_malloc(pof2 * sizeof(int));
    disps = (int *) xbt_malloc(pof2 * sizeof(int));

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
        smpi_mpi_sendrecv((char *) recvbuf +
                     disps[send_idx] * extent,
                     send_cnt, datatype,
                     dst, tag,
                     (char *) tmp_buf +
                     disps[recv_idx] * extent,
                     recv_cnt, datatype, dst, tag, comm, &status);

        /* tmp_buf contains data received in this step.
           recvbuf contains data accumulated so far */

        smpi_op_apply(op, (char *) tmp_buf + disps[recv_idx] * extent,
                       (char *) recvbuf + disps[recv_idx] * extent,
                       &recv_cnt, &datatype);

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

          smpi_mpi_recv(recvbuf, cnts[0], datatype, 0, tag, comm, &status);

          newrank = 0;
          send_idx = 0;
          last_idx = 2;
        } else if (newrank == 0) {
          smpi_mpi_send(recvbuf, cnts[0], datatype, root, tag, comm);
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
          smpi_mpi_send((char *) recvbuf +
                   disps[send_idx] * extent,
                   send_cnt, datatype, dst, tag, comm);
          break;
        } else {
          smpi_mpi_recv((char *) recvbuf +
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
  if (cnts)
    free(cnts);
  if (disps)
    free(disps);

  return 0;
}
