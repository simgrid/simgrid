int
alltoall_native(void * send_buff, int send_count,
		MPI_Datatype send_type, void * recv_buff,
		int recv_count, MPI_Datatype recv_type,
		MPI_Comm comm)
{
  return MPI_Alltoall(send_buff, send_count, send_type, recv_buff, recv_count,
		      recv_type, comm);
}
