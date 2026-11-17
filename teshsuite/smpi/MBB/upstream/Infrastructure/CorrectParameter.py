#! /usr/bin/python3
from Infrastructure import MPICall
from Infrastructure.Instruction import Instruction
# from Infrastructure.MPICallFactory import MPICallFactory
from Infrastructure.AllocCall import AllocCall, get_free


class CorrectParameterFactory:
    # default params
    buf_size = "10"
    recvbuf_size = "10*nprocs"
    dtype = ['int', 'MPI_INT']
    buf_size_bytes = f"{buf_size}*sizeof({dtype[0]})"
    recvbuf_size_bytes = f"{recvbuf_size}*sizeof({dtype[0]})"
    tag = 0
    buf_var_name = "buf"
    recvbuf_var_name = "recv_buf"
    winbuf_var_name = "winbuf"

    def __init__(self):
        pass

    def get_buffer_alloc(self) -> AllocCall:
        return AllocCall(self.dtype[0], self.buf_size, self.buf_var_name, use_malloc=False)

    def get_recvbuffer_alloc(self) -> AllocCall:
        return AllocCall(self.dtype[0], self.recvbuf_size, self.recvbuf_var_name, use_malloc=False)

    def get_buffer_free(self) -> Instruction:
        return get_free(AllocCall(self.dtype[0], self.buf_size, self.buf_var_name, use_malloc=False))
    
    def get_recvbuffer_free(self) -> Instruction:
        return get_free(AllocCall(self.dtype[0], self.recvbuf_size, self.recvbuf_var_name, use_malloc=False))

    def get(self, param: str) -> str:
        if param in ["BUFFER", "buf", "buffer", "sendbuf", "recvbuf", "origin_addr"]:
            return self.buf_var_name
        if param in ["COUNT", "count", "sendcount", "recvcount", "origin_count", "target_count", "result_count"]:
            return str(self.buf_size)
        if param in ["COUNT", "count", "sendcount", "recvcount", "origin_count", "target_count", "result_count"]:
            return str(self.buf_size)
        if param in ["sendcounts", "recvcounts"]:
            return str(self.buf_size)
        if param in ["DATATYPE", "datatype", "sendtype", "recvtype", "origin_datatype", "target_datatype",
                     "result_datatype"]:
            return self.dtype[1]
        if param in ["DEST", "dest", "rank", "target_rank"]:
            return "0"
        if param in ["SRC", "source"]:
            return "1"
        if param in ["RANK", "root"]:
            return "0"
        if param in ["TAG", "tag", "sendtag", "recvtag"]:
            return str(self.tag)
        if param in "stringtag":
            return "\"" + str(self.tag) + "\""
        if param in ["COMM", "comm"]:
            return "MPI_COMM_WORLD"
        if param in ["newcomm", "newintercomm"]:
            return "mpi_comm_0"
        if param in ["STATUS", "status"]:
            return "MPI_STATUS_IGNORE"
        if param in ["OPERATION", "op"]:
            return "MPI_SUM"
        if param in ["INFO", "info"]:
            return "MPI_INFO_NULL"
        if param in ["PARTITIONS", "partitions"]:
            return "1"
        if param in ["PARTITION", "partition"]:
            return "0"
        if param in ["REQUEST", "request"]:
            return "&mpi_request_0"
        if param in ["GROUP", "group"]:
            return "&mpi_group_0"
        if param in ["color"]:
            return "1"
        if param in ["message"]:
            return "&mpi_message_0"
        if param in ["flag"]:
            return "&int_0"
        if param in ["split_type"]:
            return "MPI_COMM_TYPE_SHARED"
        if param in ["key"]:
            return "rank"
        if param in ["errhandler"]:
            return "MPI_ERRORS_ARE_FATAL"
        if param in ["local_comm"]:
            return "MPI_COMM_SELF"
        if param in ["local_leader"]:
            return "0"
        if param in ["peer_comm"]:
            return "MPI_COMM_WORLD"
        if param in ["remote_leader"]:
            return "0"
        if param in ["target_disp"]:
            return "0"
        if param in ["win"]:
            return "mpi_win_0"
        if param in ["lock_type"]:
            return "MPI_LOCK_EXCLUSIVE"
        if param in ["assert"]:
            return "0"
        if param in ["baseptr"]:
            return "&" + self.winbuf_var_name
        if param in ["base"]:
            return self.winbuf_var_name
        if param in ["size"]:
            return self.buf_size_bytes
        if param in ["disp_unit"]:
            return "sizeof(int)"
        if param in ["displs", "sdispls", "rdispls"]:
            return "(int*)malloc(sizeof(int)*10)"
        if param in ["info"]:
            return "MPI_INFO_NULL"
        if param in ["result_addr"]:
            return "resultbuf"
        if param in ["compare_addr"]:
            return "comparebuf"
        if param in ["comm_cart"]:
            return "&mpi_comm_0"
        if param in ["comm_old"]:
            return "MPI_COMM_WORLD"
        if param in ["ndims", "maxdims"]:
            return "2"
        if param in ["dims"]:
            return "dims"
        if param in ["coords"]:
            return "coords"
        if param in ["periods"]:
            return "periods"
        if param in ["reorder"]:
            return "0"
        print("Not Implemented: " + param)
        assert False, "Param not known"

    def get_initializer(self, variable_type: str) -> str:
        if variable_type == "int":
            return "0"
        if variable_type == "MPI_Request":
            return "MPI_REQUEST_NULL"
        if variable_type == "MPI_Comm":
            return "MPI_COMM_NULL"
        if variable_type == "MPI_Message":
            return "MPI_MESSAGE_NULL"
        if variable_type == "MPI_Group":
            return "MPI_GROUP_NULL"
        if variable_type == "MPI_Datatype":
            return "MPI_DATATYPE_NULL"
        # TODO implement other types
        print("Not Implemented: " + variable_type)
        assert False, "Param not known"


# todo also for send and non default args
def get_matching_recv(call: MPICall) -> MPICall:
    correct_params = CorrectParameterFactory()
    recv = MPICallFactory().mpi_recv(
        correct_params.get("BUFFER"),
        correct_params.get("COUNT"),
        correct_params.get("DATATYPE"),
        correct_params.get("SRC"),
        correct_params.get("TAG"),
        correct_params.get("COMM"),
        correct_params.get("STATUS", "MPI_Recv"),
    )

    return recv
