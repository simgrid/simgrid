/* Copyright (c) 2007-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "simgrid/sg_config.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_mpi, smpi, "Logging specific to SMPI (mpi)");

/* MPI User level calls */

int MPI_Init(int *argc, char ***argv)
{
  return PMPI_Init(argc, argv);
}

int MPI_Finalize(void)
{
  return PMPI_Finalize();
}

int MPI_Finalized(int * flag)
{
  return PMPI_Finalized(flag);
}

int MPI_Init_thread(int *argc, char ***argv, int required, int *provided)
{
  return PMPI_Init_thread(argc, argv, required, provided);
}

int MPI_Query_thread(int *provided)
{
  return PMPI_Query_thread(provided);
}

int MPI_Is_thread_main(int *flag)
{
  return PMPI_Is_thread_main(flag);
}

int MPI_Abort(MPI_Comm comm, int errorcode)
{
  return PMPI_Abort(comm, errorcode);
}

double MPI_Wtime(void)
{
  return PMPI_Wtime();
}

double MPI_Wtick(void)
{
  return PMPI_Wtick();
}

int MPI_Address(void *location, MPI_Aint * address)
{
  return PMPI_Address(location, address);
}

int MPI_Get_address(void *location, MPI_Aint * address)
{
  return PMPI_Get_address(location, address);
}

int MPI_Type_free(MPI_Datatype * datatype)
{
  return PMPI_Type_free(datatype);
}

int MPI_Type_dup(MPI_Datatype  datatype, MPI_Datatype * newdatatype)
{
  return PMPI_Type_dup(datatype, newdatatype);
}

int MPI_Type_set_name(MPI_Datatype  datatype, char * name)
{
  return PMPI_Type_set_name(datatype, name);
}

int MPI_Type_get_name(MPI_Datatype  datatype, char * name, int* len)
{
  return PMPI_Type_get_name(datatype,name,len);
}

int MPI_Type_get_attr (MPI_Datatype type, int type_keyval, void *attribute_val, int* flag)
{
   return PMPI_Type_get_attr ( type, type_keyval, attribute_val, flag);
}

int MPI_Type_set_attr (MPI_Datatype type, int type_keyval, void *attribute_val)
{
   return PMPI_Type_set_attr ( type, type_keyval, attribute_val);
}

int MPI_Type_delete_attr (MPI_Datatype type, int type_keyval)
{
   return PMPI_Type_delete_attr (type,  type_keyval);
}

int MPI_Type_create_keyval(MPI_Type_copy_attr_function* copy_fn, MPI_Type_delete_attr_function* delete_fn, int* keyval,
                           void* extra_state)
{
   return PMPI_Type_create_keyval(copy_fn,  delete_fn,  keyval,  extra_state) ;
}

int MPI_Type_free_keyval(int* keyval) {
   return PMPI_Type_free_keyval( keyval);
}

int MPI_Pcontrol(const int level )
{
  return PMPI_Pcontrol(level);
}

int MPI_Type_size(MPI_Datatype datatype, int *size)
{
  return PMPI_Type_size(datatype, size);
}

int MPI_Type_get_extent(MPI_Datatype datatype, MPI_Aint * lb, MPI_Aint * extent)
{
  return PMPI_Type_get_extent(datatype, lb, extent);
}

int MPI_Type_get_true_extent(MPI_Datatype datatype, MPI_Aint * lb, MPI_Aint * extent)
{
  return PMPI_Type_get_true_extent(datatype, lb, extent);
}

int MPI_Type_extent(MPI_Datatype datatype, MPI_Aint * extent)
{
  return PMPI_Type_extent(datatype, extent);
}

int MPI_Type_lb(MPI_Datatype datatype, MPI_Aint * disp)
{
  return PMPI_Type_lb(datatype, disp);
}

int MPI_Type_ub(MPI_Datatype datatype, MPI_Aint * disp)
{
  return PMPI_Type_ub(datatype, disp);
}

int MPI_Op_create(MPI_User_function * function, int commute, MPI_Op * op)
{
  return PMPI_Op_create(function, commute, op);
}

int MPI_Op_free(MPI_Op * op)
{
  return PMPI_Op_free(op);
}

int MPI_Group_free(MPI_Group * group)
{
  return PMPI_Group_free(group);
}

int MPI_Group_size(MPI_Group group, int *size)
{
  return PMPI_Group_size(group, size);
}

int MPI_Group_rank(MPI_Group group, int *rank)
{
  return PMPI_Group_rank(group, rank);
}

int MPI_Group_translate_ranks(MPI_Group group1, int n, int *ranks1, MPI_Group group2, int *ranks2)
{
  return PMPI_Group_translate_ranks(group1, n, ranks1, group2, ranks2);
}

int MPI_Group_compare(MPI_Group group1, MPI_Group group2, int *result)
{
  return PMPI_Group_compare(group1, group2, result);
}

int MPI_Group_union(MPI_Group group1, MPI_Group group2, MPI_Group * newgroup)
{
  return PMPI_Group_union(group1, group2, newgroup);
}

int MPI_Group_intersection(MPI_Group group1, MPI_Group group2, MPI_Group * newgroup)
{
  return PMPI_Group_intersection(group1, group2, newgroup);
}

int MPI_Group_difference(MPI_Group group1, MPI_Group group2, MPI_Group * newgroup)
{
  return PMPI_Group_difference(group1, group2, newgroup);
}

int MPI_Group_incl(MPI_Group group, int n, int *ranks, MPI_Group * newgroup)
{
  return PMPI_Group_incl(group, n, ranks, newgroup);
}

int MPI_Group_excl(MPI_Group group, int n, int *ranks, MPI_Group * newgroup)
{
  return PMPI_Group_excl(group, n, ranks, newgroup);
}

int MPI_Group_range_incl(MPI_Group group, int n, int ranges[][3], MPI_Group * newgroup)
{
  return PMPI_Group_range_incl(group, n, ranges, newgroup);
}

int MPI_Group_range_excl(MPI_Group group, int n, int ranges[][3], MPI_Group * newgroup)
{
  return PMPI_Group_range_excl(group, n, ranges, newgroup);
}

int MPI_Comm_rank(MPI_Comm comm, int *rank)
{
  return PMPI_Comm_rank(comm, rank);
}

int MPI_Comm_size(MPI_Comm comm, int *size)
{
  return PMPI_Comm_size(comm, size);
}

int MPI_Comm_get_attr (MPI_Comm comm, int comm_keyval, void *attribute_val, int *flag)
{
  return PMPI_Comm_get_attr (comm, comm_keyval, attribute_val, flag);
}

int MPI_Comm_set_attr (MPI_Comm comm, int comm_keyval, void *attribute_val)
{
   return PMPI_Comm_set_attr ( comm, comm_keyval, attribute_val);
}

int MPI_Comm_delete_attr (MPI_Comm comm, int comm_keyval)
{
   return PMPI_Comm_delete_attr (comm,  comm_keyval);
}

int MPI_Comm_create_keyval(MPI_Comm_copy_attr_function* copy_fn, MPI_Comm_delete_attr_function* delete_fn, int* keyval,
                           void* extra_state)
{
   return PMPI_Comm_create_keyval(copy_fn,  delete_fn,  keyval,  extra_state) ;
}

int MPI_Comm_free_keyval(int* keyval) {
   return PMPI_Comm_free_keyval( keyval);
}

int MPI_Comm_get_name (MPI_Comm comm, char* name, int* len)
{
  return PMPI_Comm_get_name(comm, name, len);
}

int MPI_Comm_group(MPI_Comm comm, MPI_Group * group)
{
  return PMPI_Comm_group(comm, group);
}

int MPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int *result)
{
  return PMPI_Comm_compare(comm1, comm2, result);
}

int MPI_Comm_dup(MPI_Comm comm, MPI_Comm * newcomm)
{
  return PMPI_Comm_dup(comm, newcomm);
}

int MPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm * newcomm)
{
  return PMPI_Comm_create(comm, group, newcomm);
}

int MPI_Comm_free(MPI_Comm * comm)
{
  return PMPI_Comm_free(comm);
}

int MPI_Comm_disconnect(MPI_Comm * comm)
{
  return PMPI_Comm_disconnect(comm);
}

int MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm* comm_out)
{
  return PMPI_Comm_split(comm, color, key, comm_out);
}

int MPI_Send_init(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request * request)
{
  return PMPI_Send_init(buf, count, datatype, dst, tag, comm, request);
}

int MPI_Recv_init(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Request * request)
{
  return PMPI_Recv_init(buf, count, datatype, src, tag, comm, request);
}

int MPI_Start(MPI_Request * request)
{
  return PMPI_Start(request);
}

int MPI_Startall(int count, MPI_Request * requests)
{
  return PMPI_Startall(count, requests);
}

int MPI_Request_free(MPI_Request * request)
{
  return PMPI_Request_free(request);
}

int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Request * request)
{
  return PMPI_Irecv(buf, count, datatype, src, tag, comm, request);
}

int MPI_Isend(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request * request)
{
  return PMPI_Isend(buf, count, datatype, dst, tag, comm, request);
}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Status * status)
{
  return PMPI_Recv(buf, count, datatype, src, tag, comm, status);
}

int MPI_Send(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm)
{
  return PMPI_Send(buf, count, datatype, dst, tag, comm);
}

int MPI_Sendrecv(void *sendbuf, int sendcount, MPI_Datatype sendtype,int dst, int sendtag, void *recvbuf, int recvcount,
                 MPI_Datatype recvtype, int src, int recvtag, MPI_Comm comm, MPI_Status * status)
{
  return PMPI_Sendrecv(sendbuf, sendcount, sendtype, dst, sendtag, recvbuf, recvcount, recvtype, src, recvtag,
                       comm, status);
}

int MPI_Sendrecv_replace(void *buf, int count, MPI_Datatype datatype, int dst, int sendtag, int src, int recvtag,
                         MPI_Comm comm, MPI_Status * status)
{
  return PMPI_Sendrecv_replace(buf, count, datatype, dst, sendtag, src, recvtag, comm, status);
}

int MPI_Test(MPI_Request * request, int *flag, MPI_Status * status)
{
  return PMPI_Test(request, flag, status);
}

int MPI_Testany(int count, MPI_Request requests[], int *index, int *flag, MPI_Status * status)
{
  return PMPI_Testany(count, requests, index, flag, status);
}

int MPI_Wait(MPI_Request * request, MPI_Status * status)
{
  return PMPI_Wait(request, status);
}

int MPI_Waitany(int count, MPI_Request requests[], int *index, MPI_Status * status)
{
  return PMPI_Waitany(count, requests, index, status);
}

int MPI_Waitall(int count, MPI_Request requests[], MPI_Status status[])
{
  return PMPI_Waitall(count, requests, status);
}

int MPI_Waitsome(int incount, MPI_Request requests[], int *outcount, int *indices, MPI_Status status[])
{
  return PMPI_Waitsome(incount, requests, outcount, indices, status);
}

int MPI_Bcast(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
  return PMPI_Bcast(buf, count, datatype, root, comm);
}

int MPI_Barrier(MPI_Comm comm)
{
  return PMPI_Barrier(comm);
}

int MPI_Gather(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
               int root, MPI_Comm comm)
{
  return PMPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
}

int MPI_Gatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int *recvcounts, int *displs,
                MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  return PMPI_Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm);
}

int MPI_Allgather(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount,
                  MPI_Datatype recvtype, MPI_Comm comm)
{
  return PMPI_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

int MPI_Allgatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int *recvcounts, int *displs,
                   MPI_Datatype recvtype, MPI_Comm comm)
{
  return PMPI_Allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
}

int MPI_Scatter(void *sendbuf, int sendcount, MPI_Datatype sendtype,void *recvbuf, int recvcount, MPI_Datatype recvtype,
                int root, MPI_Comm comm)
{
  return PMPI_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
}

int MPI_Scatterv(void *sendbuf, int *sendcounts, int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount,
                 MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  return PMPI_Scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm);
}

int MPI_Reduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
  return PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
}

int MPI_Reduce_local(void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype, MPI_Op op){
  return PMPI_Reduce_local(inbuf, inoutbuf, count, datatype, op);
}

int MPI_Allreduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  return PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
}

int MPI_Scan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  return PMPI_Scan(sendbuf, recvbuf, count, datatype, op, comm);
}

int MPI_Reduce_scatter(void *sendbuf, void *recvbuf, int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  return PMPI_Reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm);
}

int MPI_Reduce_scatter_block(void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op,
                             MPI_Comm comm)
{
  return PMPI_Reduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, comm);
}

int MPI_Alltoall(void *sendbuf, int sendcount, MPI_Datatype sendtype,void *recvbuf, int recvcount,
                 MPI_Datatype recvtype, MPI_Comm comm)
{
  return PMPI_Alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

int MPI_Alltoallv(void *sendbuf, int *sendcounts, int *senddisps, MPI_Datatype sendtype, void *recvbuf, int *recvcounts,
                  int *recvdisps, MPI_Datatype recvtype, MPI_Comm comm)
{
  return PMPI_Alltoallv(sendbuf, sendcounts, senddisps, sendtype, recvbuf, recvcounts, recvdisps, recvtype, comm);
}

int MPI_Alloc_mem(MPI_Aint size, MPI_Info info, void *baseptr){
  return PMPI_Alloc_mem(size, info, baseptr);
}

int MPI_Free_mem(void *baseptr){
  return PMPI_Free_mem(baseptr);
}

int MPI_Get_processor_name(char *name, int *resultlen)
{
  return PMPI_Get_processor_name(name, resultlen);
}

int MPI_Get_count(MPI_Status * status, MPI_Datatype datatype, int *count)
{
  return PMPI_Get_count(status, datatype, count);
}

int MPI_Pack_size(int incount, MPI_Datatype datatype, MPI_Comm comm, int* size) {
  return PMPI_Pack_size(incount, datatype, comm, size);
}

int MPI_Cart_coords(MPI_Comm comm, int rank, int maxdims, int* coords) {
  return PMPI_Cart_coords(comm, rank, maxdims, coords);
}

int MPI_Cart_create(MPI_Comm comm_old, int ndims, int* dims, int* periods, int reorder, MPI_Comm* comm_cart) {
  return PMPI_Cart_create(comm_old, ndims, dims, periods, reorder, comm_cart);
}

int MPI_Cart_get(MPI_Comm comm, int maxdims, int* dims, int* periods, int* coords) {
  return PMPI_Cart_get(comm, maxdims, dims, periods, coords);
}

int MPI_Cart_map(MPI_Comm comm_old, int ndims, int* dims, int* periods, int* newrank) {
  return PMPI_Cart_map(comm_old, ndims, dims, periods, newrank);
}

int MPI_Cart_rank(MPI_Comm comm, int* coords, int* rank) {
  return PMPI_Cart_rank(comm, coords, rank);
}

int MPI_Cart_shift(MPI_Comm comm, int direction, int displ, int* source, int* dest) {
  return PMPI_Cart_shift(comm, direction, displ, source, dest);
}

int MPI_Cart_sub(MPI_Comm comm, int* remain_dims, MPI_Comm* comm_new) {
  return PMPI_Cart_sub(comm, remain_dims, comm_new);
}

int MPI_Cartdim_get(MPI_Comm comm, int* ndims) {
  return PMPI_Cartdim_get(comm, ndims);
}

int MPI_Graph_create(MPI_Comm comm_old, int nnodes, int* index, int* edges, int reorder, MPI_Comm* comm_graph) {
  return PMPI_Graph_create(comm_old, nnodes, index, edges, reorder, comm_graph);
}

int MPI_Graph_get(MPI_Comm comm, int maxindex, int maxedges, int* index, int* edges) {
  return PMPI_Graph_get(comm, maxindex, maxedges, index, edges);
}

int MPI_Graph_map(MPI_Comm comm_old, int nnodes, int* index, int* edges, int* newrank) {
  return PMPI_Graph_map(comm_old, nnodes, index, edges, newrank);
}

int MPI_Graph_neighbors(MPI_Comm comm, int rank, int maxneighbors, int* neighbors) {
  return PMPI_Graph_neighbors(comm, rank, maxneighbors, neighbors);
}

int MPI_Graph_neighbors_count(MPI_Comm comm, int rank, int* nneighbors) {
  return PMPI_Graph_neighbors_count(comm, rank, nneighbors);
}

int MPI_Graphdims_get(MPI_Comm comm, int* nnodes, int* nedges) {
  return PMPI_Graphdims_get(comm, nnodes, nedges);
}

int MPI_Topo_test(MPI_Comm comm, int* top_type) {
  return PMPI_Topo_test(comm, top_type);
}

int MPI_Error_class(int errorcode, int* errorclass) {
  return PMPI_Error_class(errorcode, errorclass);
}

int MPI_Errhandler_create(MPI_Handler_function* function, MPI_Errhandler* errhandler) {
  return PMPI_Errhandler_create(function, errhandler);
}

int MPI_Errhandler_free(MPI_Errhandler* errhandler) {
  return PMPI_Errhandler_free(errhandler);
}

int MPI_Errhandler_get(MPI_Comm comm, MPI_Errhandler* errhandler) {
  return PMPI_Errhandler_get(comm, errhandler);
}

int MPI_Error_string(int errorcode, char* string, int* resultlen) {
  return PMPI_Error_string(errorcode, string, resultlen);
}

int MPI_Errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler) {
  return PMPI_Errhandler_set(comm, errhandler);
}

int MPI_Comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler) {
  return PMPI_Comm_set_errhandler(comm, errhandler);
}

int MPI_Comm_get_errhandler(MPI_Comm comm, MPI_Errhandler* errhandler) {
  return PMPI_Errhandler_set(comm, errhandler);
}

int MPI_Win_get_group(MPI_Win  win, MPI_Group * group){
  return PMPI_Win_get_group(win, group);
}

int MPI_Win_set_errhandler(MPI_Win win, MPI_Errhandler errhandler) {
  return PMPI_Win_set_errhandler(win, errhandler);
}

int MPI_Type_contiguous(int count, MPI_Datatype old_type, MPI_Datatype* newtype) {
  return PMPI_Type_contiguous(count, old_type, newtype);
}

int MPI_Cancel(MPI_Request* request) {
  return PMPI_Cancel(request);
}

int MPI_Buffer_attach(void* buffer, int size) {
  return PMPI_Buffer_attach(buffer, size);
}

int MPI_Buffer_detach(void* buffer, int* size) {
  return PMPI_Buffer_detach(buffer, size);
}

int MPI_Testsome(int incount, MPI_Request* requests, int* outcount, int* indices, MPI_Status* statuses) {
  return PMPI_Testsome(incount, requests, outcount, indices, statuses);
}

int MPI_Comm_test_inter(MPI_Comm comm, int* flag) {
  return PMPI_Comm_test_inter(comm, flag);
}

int MPI_Unpack(void* inbuf, int insize, int* position, void* outbuf, int outcount, MPI_Datatype type, MPI_Comm comm) {
  return PMPI_Unpack(inbuf, insize, position, outbuf, outcount, type, comm);
}

int MPI_Pack_external_size(char *datarep, int incount, MPI_Datatype datatype, MPI_Aint *size){
  return PMPI_Pack_external_size(datarep, incount, datatype, size);
}

int MPI_Pack_external(char *datarep, void *inbuf, int incount, MPI_Datatype datatype, void *outbuf, MPI_Aint outcount,
                      MPI_Aint *position){
  return PMPI_Pack_external(datarep, inbuf, incount, datatype, outbuf, outcount, position);
}

int MPI_Unpack_external(char *datarep, void *inbuf, MPI_Aint insize, MPI_Aint *position, void *outbuf, int outcount,
                        MPI_Datatype datatype){
  return PMPI_Unpack_external( datarep, inbuf, insize, position, outbuf, outcount, datatype);
}

int MPI_Type_commit(MPI_Datatype* datatype) {
  return PMPI_Type_commit(datatype);
}

int MPI_Type_hindexed(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype old_type, MPI_Datatype* newtype) {
  return PMPI_Type_hindexed(count, blocklens, indices, old_type, newtype);
}

int MPI_Type_create_hindexed(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype old_type,
                             MPI_Datatype* new_type) {
  return PMPI_Type_create_hindexed(count, blocklens,indices,old_type,new_type);
}

int MPI_Type_create_hindexed_block(int count, int blocklength, MPI_Aint* indices, MPI_Datatype old_type,
                                   MPI_Datatype* newtype) {
  return PMPI_Type_create_hindexed_block(count, blocklength, indices, old_type, newtype);
}

int MPI_Type_hvector(int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype* newtype) {
  return PMPI_Type_hvector(count, blocklen, stride, old_type, newtype);
}

int MPI_Type_indexed(int count, int* blocklens, int* indices, MPI_Datatype old_type, MPI_Datatype* newtype) {
  return PMPI_Type_indexed(count, blocklens, indices, old_type, newtype);
}

int MPI_Type_create_indexed(int count, int* blocklens, int* indices, MPI_Datatype old_type, MPI_Datatype* newtype) {
  return PMPI_Type_create_indexed(count, blocklens, indices, old_type, newtype);
}

int MPI_Type_create_indexed_block(int count, int blocklength, int* indices,  MPI_Datatype old_type,
                                  MPI_Datatype *newtype){
  return PMPI_Type_create_indexed_block(count, blocklength, indices, old_type, newtype);
}

int MPI_Type_struct(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype* old_types, MPI_Datatype* newtype) {
  return PMPI_Type_struct(count, blocklens, indices, old_types, newtype);
}

int MPI_Type_create_struct(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype* old_types,
                           MPI_Datatype* newtype) {
  return PMPI_Type_create_struct(count, blocklens, indices, old_types, newtype);
}

int MPI_Type_vector(int count, int blocklen, int stride, MPI_Datatype old_type, MPI_Datatype* newtype) {
  return PMPI_Type_vector(count, blocklen, stride, old_type, newtype);
}

int MPI_Ssend(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
  return PMPI_Ssend(buf, count, datatype, dest, tag, comm);
}

int MPI_Ssend_init(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request){
  return PMPI_Ssend_init(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Intercomm_create(MPI_Comm local_comm, int local_leader, MPI_Comm peer_comm, int remote_leader, int tag,
                         MPI_Comm* comm_out) {
  return PMPI_Intercomm_create(local_comm, local_leader, peer_comm, remote_leader, tag, comm_out);
}

int MPI_Intercomm_merge(MPI_Comm comm, int high, MPI_Comm* comm_out) {
  return PMPI_Intercomm_merge(comm, high, comm_out);
}

int MPI_Bsend(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
  return PMPI_Bsend(buf, count, datatype, dest, tag, comm);
}

int MPI_Bsend_init(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request){
  return PMPI_Bsend_init(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Ibsend(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request) {
  return PMPI_Ibsend(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Comm_remote_group(MPI_Comm comm, MPI_Group* group) {
  return PMPI_Comm_remote_group(comm, group);
}

int MPI_Comm_remote_size(MPI_Comm comm, int* size) {
  return PMPI_Comm_remote_size(comm, size);
}

int MPI_Issend(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request) {
  return PMPI_Issend(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status* status) {
  return PMPI_Probe(source, tag, comm, status);
}

int MPI_Attr_delete(MPI_Comm comm, int keyval) {
  return PMPI_Attr_delete(comm, keyval);
}

int MPI_Attr_get(MPI_Comm comm, int keyval, void* attr_value, int* flag) {
  return PMPI_Attr_get(comm, keyval, attr_value, flag);
}

int MPI_Attr_put(MPI_Comm comm, int keyval, void* attr_value) {
  return PMPI_Attr_put(comm, keyval, attr_value);
}

int MPI_Rsend(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
  return PMPI_Rsend(buf, count, datatype, dest, tag, comm);
}

int MPI_Rsend_init(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request){
  return PMPI_Rsend_init(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Irsend(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request) {
  return PMPI_Irsend(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Keyval_create(MPI_Copy_function* copy_fn, MPI_Delete_function* delete_fn, int* keyval, void* extra_state) {
  return PMPI_Keyval_create(copy_fn, delete_fn, keyval, extra_state);
}

int MPI_Keyval_free(int* keyval) {
  return PMPI_Keyval_free(keyval);
}

int MPI_Test_cancelled(MPI_Status* status, int* flag) {
  return PMPI_Test_cancelled(status, flag);
}

int MPI_Pack(void* inbuf, int incount, MPI_Datatype type, void* outbuf, int outcount, int* position, MPI_Comm comm) {
  return PMPI_Pack(inbuf, incount, type, outbuf, outcount, position, comm);
}

int MPI_Testall(int count, MPI_Request* requests, int* flag, MPI_Status* statuses) {
  return PMPI_Testall(count, requests, flag, statuses);
}

int MPI_Get_elements(MPI_Status* status, MPI_Datatype datatype, int* elements) {
  return PMPI_Get_elements(status, datatype, elements);
}

int MPI_Dims_create(int nnodes, int ndims, int* dims) {
  return PMPI_Dims_create(nnodes, ndims, dims);
}

int MPI_Iprobe(int source, int tag, MPI_Comm comm, int* flag, MPI_Status* status) {
  return PMPI_Iprobe(source, tag, comm, flag, status);
}

int MPI_Initialized(int* flag) {
  return PMPI_Initialized(flag);
}

int MPI_Win_fence( int assert,  MPI_Win win){
   return PMPI_Win_fence( assert, win);
}

int MPI_Win_free( MPI_Win* win){
   return PMPI_Win_free(  win);
}

int MPI_Win_create( void *base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, MPI_Win *win){
  return PMPI_Win_create( base, size, disp_unit, info, comm,win);
}

int MPI_Win_set_name(MPI_Win  win, char * name)
{
  return PMPI_Win_set_name(win, name);
}

int MPI_Win_get_name(MPI_Win  win, char * name, int* len)
{
  return PMPI_Win_get_name(win,name,len);
}

int MPI_Win_complete(MPI_Win win){
  return PMPI_Win_complete(win);
}

int MPI_Win_lock(int lock_type, int rank, int assert, MPI_Win win) {
  return PMPI_Win_lock(lock_type, rank, assert, win);
}

int MPI_Win_post(MPI_Group group, int assert, MPI_Win win){
  return PMPI_Win_post(group, assert, win);
}

int MPI_Win_start(MPI_Group group, int assert, MPI_Win win){
  return PMPI_Win_start(group, assert, win);
}

int MPI_Win_test(MPI_Win win, int *flag){
  return PMPI_Win_test(win, flag);
}

int MPI_Win_unlock(int rank, MPI_Win win){
  return PMPI_Win_unlock(rank, win);
}

int MPI_Win_wait(MPI_Win win){
  return PMPI_Win_wait(win);
}

int MPI_Info_create( MPI_Info *info){
  return PMPI_Info_create( info);
}

int MPI_Info_set( MPI_Info info, char *key, char *value){
  return PMPI_Info_set( info, key, value);
}

int MPI_Info_free( MPI_Info *info){
  return PMPI_Info_free( info);
}

int MPI_Get( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
    MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win){
  return PMPI_Get(origin_addr,origin_count, origin_datatype,target_rank, target_disp, target_count,target_datatype,win);
}

int MPI_Put( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
    MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win){
  return PMPI_Put(origin_addr,origin_count, origin_datatype,target_rank,target_disp, target_count,target_datatype, win);
}

int MPI_Accumulate( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
    MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win){
  return PMPI_Accumulate( origin_addr,origin_count, origin_datatype,target_rank,
      target_disp, target_count,target_datatype,op, win);
}

int MPI_Type_get_envelope( MPI_Datatype datatype, int *num_integers,
                          int *num_addresses, int *num_datatypes, int *combiner){
  return PMPI_Type_get_envelope(  datatype, num_integers, num_addresses, num_datatypes, combiner);
}

int MPI_Type_get_contents(MPI_Datatype datatype, int max_integers, int max_addresses, int max_datatypes,
                          int* array_of_integers, MPI_Aint* array_of_addresses, MPI_Datatype *array_of_datatypes){
  return PMPI_Type_get_contents(datatype, max_integers, max_addresses,max_datatypes, array_of_integers,
                                array_of_addresses, array_of_datatypes);
}

int MPI_Type_create_darray(int size, int rank, int ndims, int* array_of_gsizes, int* array_of_distribs,
                            int* array_of_dargs, int* array_of_psizes,int order, MPI_Datatype oldtype,
                            MPI_Datatype *newtype) {
  return PMPI_Type_create_darray(size, rank, ndims, array_of_gsizes,array_of_distribs, array_of_dargs, array_of_psizes,
      order,  oldtype, newtype) ;
}

int MPI_Type_create_resized(MPI_Datatype oldtype,MPI_Aint lb, MPI_Aint extent, MPI_Datatype *newtype){
  return PMPI_Type_create_resized(oldtype,lb, extent, newtype);
}

int MPI_Type_create_subarray(int ndims,int *array_of_sizes, int *array_of_subsizes, int *array_of_starts, int order,
                             MPI_Datatype oldtype, MPI_Datatype *newtype){
  return PMPI_Type_create_subarray(ndims,array_of_sizes, array_of_subsizes, array_of_starts, order, oldtype, newtype);
}

int MPI_Type_match_size(int typeclass,int size,MPI_Datatype *datatype){
  return PMPI_Type_match_size(typeclass,size,datatype);
}

int MPI_Alltoallw( void *sendbuf, int *sendcnts, int *sdispls, MPI_Datatype *sendtypes,
                   void *recvbuf, int *recvcnts, int *rdispls, MPI_Datatype *recvtypes, MPI_Comm comm){
  return PMPI_Alltoallw( sendbuf, sendcnts, sdispls, sendtypes, recvbuf, recvcnts, rdispls, recvtypes, comm);
}

int MPI_Exscan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm){
  return PMPI_Exscan(sendbuf, recvbuf, count, datatype, op, comm);
}

int MPI_Comm_set_name (MPI_Comm comm, char* name){
  return PMPI_Comm_set_name (comm, name);
}

int MPI_Comm_dup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm * newcomm){
  return PMPI_Comm_dup_with_info(comm,info,newcomm);
}

int MPI_Comm_split_type(MPI_Comm comm, int split_type, int key, MPI_Info info, MPI_Comm *newcomm){
  return PMPI_Comm_split_type(comm, split_type, key, info, newcomm);
}

int MPI_Comm_set_info (MPI_Comm comm, MPI_Info info){
  return PMPI_Comm_set_info (comm, info);
}

int MPI_Comm_get_info (MPI_Comm comm, MPI_Info* info){
  return PMPI_Comm_get_info (comm, info);
}

int MPI_Info_get(MPI_Info info,char *key,int valuelen, char *value, int *flag){
  return PMPI_Info_get(info,key,valuelen, value, flag);
}

int MPI_Comm_create_errhandler( MPI_Comm_errhandler_fn *function, MPI_Errhandler *errhandler){
  return PMPI_Comm_create_errhandler( function, errhandler);
}

int MPI_Add_error_class( int *errorclass){
  return PMPI_Add_error_class( errorclass);
}

int MPI_Add_error_code(  int errorclass, int *errorcode){
  return PMPI_Add_error_code(errorclass, errorcode);
}

int MPI_Add_error_string( int errorcode, char *string){
  return PMPI_Add_error_string(errorcode, string);
}

int MPI_Comm_call_errhandler(MPI_Comm comm,int errorcode){
  return PMPI_Comm_call_errhandler(comm, errorcode);
}

int MPI_Info_dup(MPI_Info info, MPI_Info *newinfo){
  return PMPI_Info_dup(info, newinfo);
}

int MPI_Info_get_valuelen( MPI_Info info, char *key, int *valuelen, int *flag){
  return PMPI_Info_get_valuelen( info, key, valuelen, flag);
}

int MPI_Info_delete(MPI_Info info, char *key){
  return PMPI_Info_delete(info, key);
}

int MPI_Info_get_nkeys( MPI_Info info, int *nkeys){
  return PMPI_Info_get_nkeys(  info, nkeys);
}

int MPI_Info_get_nthkey( MPI_Info info, int n, char *key){
  return PMPI_Info_get_nthkey( info, n, key);
}

int MPI_Get_version (int *version,int *subversion){
  return PMPI_Get_version (version,subversion);
}

int MPI_Get_library_version (char *version,int *len){
  return PMPI_Get_library_version (version,len);
}

int MPI_Request_get_status( MPI_Request request, int *flag, MPI_Status *status){
  return PMPI_Request_get_status( request, flag, status);
}

int MPI_Grequest_start(MPI_Grequest_query_function *query_fn, MPI_Grequest_free_function *free_fn,
                       MPI_Grequest_cancel_function *cancel_fn, void *extra_state, MPI_Request *request){
  return PMPI_Grequest_start( query_fn, free_fn, cancel_fn, extra_state, request);
}

int MPI_Grequest_complete( MPI_Request request){
  return PMPI_Grequest_complete( request);
}

int MPI_Status_set_cancelled(MPI_Status *status,int flag){
  return PMPI_Status_set_cancelled(status,flag);
}

int MPI_Status_set_elements( MPI_Status *status, MPI_Datatype datatype, int count){
  return PMPI_Status_set_elements( status, datatype, count);
}

int MPI_Comm_connect( char *port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *newcomm){
  return PMPI_Comm_connect( port_name, info, root, comm, newcomm);
}

int MPI_Publish_name( char *service_name, MPI_Info info, char *port_name){
  return PMPI_Publish_name( service_name, info, port_name);
}

int MPI_Unpublish_name( char *service_name, MPI_Info info, char *port_name){
  return PMPI_Unpublish_name( service_name, info, port_name);
}

int MPI_Lookup_name( char *service_name, MPI_Info info, char *port_name){
  return PMPI_Lookup_name( service_name, info, port_name);
}

int MPI_Comm_join( int fd, MPI_Comm *intercomm){
  return PMPI_Comm_join( fd, intercomm);
}

int MPI_Open_port( MPI_Info info, char *port_name){
  return PMPI_Open_port( info,port_name);
}

int MPI_Close_port( char *port_name){
  return PMPI_Close_port( port_name);
}

int MPI_Comm_accept( char *port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *newcomm){
 return PMPI_Comm_accept( port_name, info, root, comm, newcomm);
}

int MPI_Comm_spawn( char *command, char **argv, int maxprocs, MPI_Info info, int root,
                    MPI_Comm comm, MPI_Comm *intercomm, int* array_of_errcodes){
  return PMPI_Comm_spawn( command, argv, maxprocs, info, root, comm, intercomm, array_of_errcodes);
}

int MPI_Comm_spawn_multiple(int count, char **array_of_commands, char*** array_of_argv, int* array_of_maxprocs,
                            MPI_Info* array_of_info, int root, MPI_Comm comm, MPI_Comm *intercomm,
                            int* array_of_errcodes){
  return PMPI_Comm_spawn_multiple( count, array_of_commands, array_of_argv, array_of_maxprocs,
                                  array_of_info, root, comm, intercomm, array_of_errcodes);
}

int MPI_Comm_get_parent( MPI_Comm *parent){
  return PMPI_Comm_get_parent( parent);
}

int MPI_Type_create_hvector(int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype* new_type) {
  return PMPI_Type_create_hvector(count, blocklen, stride, old_type, new_type);
}

MPI_Datatype MPI_Type_f2c(MPI_Fint datatype){
  return PMPI_Type_f2c(datatype);
}

MPI_Fint MPI_Type_c2f(MPI_Datatype datatype){
  return PMPI_Type_c2f( datatype);
}

MPI_Group MPI_Group_f2c(MPI_Fint group){
  return PMPI_Group_f2c( group);
}

MPI_Fint MPI_Group_c2f(MPI_Group group){
  return PMPI_Group_c2f(group);
}

MPI_Request MPI_Request_f2c(MPI_Fint request){
  return PMPI_Request_f2c(request);
}

MPI_Fint MPI_Request_c2f(MPI_Request request) {
  return PMPI_Request_c2f(request);
}

MPI_Win MPI_Win_f2c(MPI_Fint win){
  return PMPI_Win_f2c(win);
}

MPI_Fint MPI_Win_c2f(MPI_Win win){
  return PMPI_Win_c2f(win);
}

MPI_Op MPI_Op_f2c(MPI_Fint op){
  return PMPI_Op_f2c(op);
}

MPI_Fint MPI_Op_c2f(MPI_Op op){
  return PMPI_Op_c2f(op);
}

MPI_Comm MPI_Comm_f2c(MPI_Fint comm){
  return PMPI_Comm_f2c(comm);
}

MPI_Fint MPI_Comm_c2f(MPI_Comm comm){
  return PMPI_Comm_c2f(comm);
}

MPI_Info MPI_Info_f2c(MPI_Fint info){
  return PMPI_Info_f2c(info);
}

MPI_Fint MPI_Info_c2f(MPI_Info info){
  return PMPI_Info_c2f(info);
}

MPI_Errhandler MPI_Errhandler_f2c(MPI_Fint errhandler){
  return PMPI_Errhandler_f2c(errhandler);
}

MPI_Fint MPI_Errhandler_c2f(MPI_Errhandler errhandler){
  return PMPI_Errhandler_c2f(errhandler);
}
