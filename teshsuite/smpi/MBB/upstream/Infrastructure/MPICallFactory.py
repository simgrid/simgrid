#! /usr/bin/python3
from collections import OrderedDict

from Infrastructure.MPICall import MPICall

class MPICallFactory:

    @staticmethod
    def get(func: str, *args):
        f_to_call = getattr(MPICallFactory, func)
        return f_to_call(*args)


    @staticmethod
    def mpi_abort(*args):
        return MPICall("MPI_Abort", OrderedDict([("comm", args[0]), ("errorcode", args[1]), ]), "1.0")

    @staticmethod
    def mpi_accumulate(*args):
        return MPICall("MPI_Accumulate", OrderedDict([("origin_addr", args[0]), ("origin_count", args[1]), ("origin_datatype", args[2]), ("target_rank", args[3]), ("target_disp", args[4]), ("target_count", args[5]), ("target_datatype", args[6]), ("op", args[7]), ("win", args[8]), ]), "2.0")

    @staticmethod
    def mpi_add_error_class(*args):
        return MPICall("MPI_Add_error_class", OrderedDict([("errorclass", args[0]), ]), "2.0")

    @staticmethod
    def mpi_add_error_code(*args):
        return MPICall("MPI_Add_error_code", OrderedDict([("errorclass", args[0]), ("errorcode", args[1]), ]), "2.0")

    @staticmethod
    def mpi_add_error_string(*args):
        return MPICall("MPI_Add_error_string", OrderedDict([("errorcode", args[0]), ("string", args[1]), ]), "2.0")

    @staticmethod
    def mpi_aint_add(*args):
        return MPICall("MPI_Aint_add", OrderedDict([("base", args[0]), ("disp", args[1]), ]), "3.1")

    @staticmethod
    def mpi_aint_diff(*args):
        return MPICall("MPI_Aint_diff", OrderedDict([("addr1", args[0]), ("addr2", args[1]), ]), "3.1")

    @staticmethod
    def mpi_allgather(*args):
        return MPICall("MPI_Allgather", OrderedDict([("sendbuf", args[0]), ("sendcount", args[1]), ("sendtype", args[2]), ("recvbuf", args[3]), ("recvcount", args[4]), ("recvtype", args[5]), ("comm", args[6]), ]), "1.0")

    @staticmethod
    def mpi_allgather_init(*args):
        return MPICall("MPI_Allgather_init", OrderedDict([("sendbuf", args[0]), ("sendcount", args[1]), ("sendtype", args[2]), ("recvbuf", args[3]), ("recvcount", args[4]), ("recvtype", args[5]), ("comm", args[6]), ("info", args[7]), ("request", args[8]), ]), "4.0")

    @staticmethod
    def mpi_allgatherv(*args):
        return MPICall("MPI_Allgatherv", OrderedDict([("sendbuf", args[0]), ("sendcount", args[1]), ("sendtype", args[2]), ("recvbuf", args[3]), ("recvcounts", args[4]), ("displs", args[5]), ("recvtype", args[6]), ("comm", args[7]), ]), "1.0")

    @staticmethod
    def mpi_allgatherv_init(*args):
        return MPICall("MPI_Allgatherv_init", OrderedDict([("sendbuf", args[0]), ("sendcount", args[1]), ("sendtype", args[2]), ("recvbuf", args[3]), ("recvcounts", args[4]), ("displs", args[5]), ("recvtype", args[6]), ("comm", args[7]), ("info", args[8]), ("request", args[9]), ]), "4.0")

    @staticmethod
    def mpi_alloc_mem(*args):
        return MPICall("MPI_Alloc_mem", OrderedDict([("size", args[0]), ("info", args[1]), ("baseptr", args[2]), ]), "2.0")

    @staticmethod
    def mpi_allreduce(*args):
        return MPICall("MPI_Allreduce", OrderedDict([("sendbuf", args[0]), ("recvbuf", args[1]), ("count", args[2]), ("datatype", args[3]), ("op", args[4]), ("comm", args[5]), ]), "1.0")

    @staticmethod
    def mpi_allreduce_init(*args):
        return MPICall("MPI_Allreduce_init", OrderedDict([("sendbuf", args[0]), ("recvbuf", args[1]), ("count", args[2]), ("datatype", args[3]), ("op", args[4]), ("comm", args[5]), ("info", args[6]), ("request", args[7]), ]), "4.0")

    @staticmethod
    def mpi_alltoall(*args):
        return MPICall("MPI_Alltoall", OrderedDict([("sendbuf", args[0]), ("sendcount", args[1]), ("sendtype", args[2]), ("recvbuf", args[3]), ("recvcount", args[4]), ("recvtype", args[5]), ("comm", args[6]), ]), "1.0")

    @staticmethod
    def mpi_alltoall_init(*args):
        return MPICall("MPI_Alltoall_init", OrderedDict([("sendbuf", args[0]), ("sendcount", args[1]), ("sendtype", args[2]), ("recvbuf", args[3]), ("recvcount", args[4]), ("recvtype", args[5]), ("comm", args[6]), ("info", args[7]), ("request", args[8]), ]), "4.0")

    @staticmethod
    def mpi_alltoallv(*args):
        return MPICall("MPI_Alltoallv", OrderedDict([("sendbuf", args[0]), ("sendcounts", args[1]), ("sdispls", args[2]), ("sendtype", args[3]), ("recvbuf", args[4]), ("recvcounts", args[5]), ("rdispls", args[6]), ("recvtype", args[7]), ("comm", args[8]), ]), "1.0")

    @staticmethod
    def mpi_alltoallv_init(*args):
        return MPICall("MPI_Alltoallv_init", OrderedDict([("sendbuf", args[0]), ("sendcounts", args[1]), ("sdispls", args[2]), ("sendtype", args[3]), ("recvbuf", args[4]), ("recvcounts", args[5]), ("rdispls", args[6]), ("recvtype", args[7]), ("comm", args[8]), ("info", args[9]), ("request", args[10]), ]), "4.0")

    @staticmethod
    def mpi_alltoallw(*args):
        return MPICall("MPI_Alltoallw", OrderedDict([("sendbuf", args[0]), ("sendcounts", args[1]), ("sdispls", args[2]), ("sendtypes", args[3]), ("recvbuf", args[4]), ("recvcounts", args[5]), ("rdispls", args[6]), ("recvtypes", args[7]), ("comm", args[8]), ]), "2.0")

    @staticmethod
    def mpi_alltoallw_init(*args):
        return MPICall("MPI_Alltoallw_init", OrderedDict([("sendbuf", args[0]), ("sendcounts", args[1]), ("sdispls", args[2]), ("sendtypes", args[3]), ("recvbuf", args[4]), ("recvcounts", args[5]), ("rdispls", args[6]), ("recvtypes", args[7]), ("comm", args[8]), ("info", args[9]), ("request", args[10]), ]), "4.0")

    @staticmethod
    def mpi_attr_delete(*args):
        return MPICall("MPI_Attr_delete", OrderedDict([("comm", args[0]), ("keyval", args[1]), ]), "1.0")

    @staticmethod
    def mpi_attr_get(*args):
        return MPICall("MPI_Attr_get", OrderedDict([("comm", args[0]), ("keyval", args[1]), ("attribute_val", args[2]), ("flag", args[3]), ]), "1.0")

    @staticmethod
    def mpi_attr_put(*args):
        return MPICall("MPI_Attr_put", OrderedDict([("comm", args[0]), ("keyval", args[1]), ("attribute_val", args[2]), ]), "1.0")

    @staticmethod
    def mpi_barrier(*args):
        return MPICall("MPI_Barrier", OrderedDict([("comm", args[0]), ]), "1.0")

    @staticmethod
    def mpi_barrier_init(*args):
        return MPICall("MPI_Barrier_init", OrderedDict([("comm", args[0]), ("info", args[1]), ("request", args[2]), ]), "4.0")

    @staticmethod
    def mpi_bcast(*args):
        return MPICall("MPI_Bcast", OrderedDict([("buffer", args[0]), ("count", args[1]), ("datatype", args[2]), ("root", args[3]), ("comm", args[4]), ]), "1.0")

    @staticmethod
    def mpi_bcast_init(*args):
        return MPICall("MPI_Bcast_init", OrderedDict([("buffer", args[0]), ("count", args[1]), ("datatype", args[2]), ("root", args[3]), ("comm", args[4]), ("info", args[5]), ("request", args[6]), ]), "4.0")

    @staticmethod
    def mpi_bsend(*args):
        return MPICall("MPI_Bsend", OrderedDict([("buf", args[0]), ("count", args[1]), ("datatype", args[2]), ("dest", args[3]), ("tag", args[4]), ("comm", args[5]), ]), "1.0")

    @staticmethod
    def mpi_bsend_init(*args):
        return MPICall("MPI_Bsend_init", OrderedDict([("buf", args[0]), ("count", args[1]), ("datatype", args[2]), ("dest", args[3]), ("tag", args[4]), ("comm", args[5]), ("request", args[6]), ]), "1.0")

    @staticmethod
    def mpi_buffer_attach(*args):
        return MPICall("MPI_Buffer_attach", OrderedDict([("buffer", args[0]), ("size", args[1]), ]), "1.0")

    @staticmethod
    def mpi_buffer_detach(*args):
        return MPICall("MPI_Buffer_detach", OrderedDict([("buffer_addr", args[0]), ("size", args[1]), ]), "1.0")

    @staticmethod
    def mpi_cancel(*args):
        return MPICall("MPI_Cancel", OrderedDict([("request", args[0]), ]), "1.0")

    @staticmethod
    def mpi_cart_coords(*args):
        return MPICall("MPI_Cart_coords", OrderedDict([("comm", args[0]), ("rank", args[1]), ("maxdims", args[2]), ("coords", args[3]), ]), "1.0")

    @staticmethod
    def mpi_cart_create(*args):
        return MPICall("MPI_Cart_create", OrderedDict([("comm_old", args[0]), ("ndims", args[1]), ("dims", args[2]), ("periods", args[3]), ("reorder", args[4]), ("comm_cart", args[5]), ]), "1.0")

    @staticmethod
    def mpi_cart_get(*args):
        return MPICall("MPI_Cart_get", OrderedDict([("comm_cart", args[0]), ("maxdims", args[1]), ("dims", args[2]), ("periods", args[3]), ("coords", args[4]), ]), "1.0")

    @staticmethod
    def mpi_cart_map(*args):
        return MPICall("MPI_Cart_map", OrderedDict([("comm", args[0]), ("ndims", args[1]), ("dims", args[2]), ("periods", args[3]), ("newrank", args[4]), ]), "1.0")

    @staticmethod
    def mpi_cart_rank(*args):
        return MPICall("MPI_Cart_rank", OrderedDict([("comm", args[0]), ("coords", args[1]), ("rank", args[2]), ]), "1.0")

    @staticmethod
    def mpi_cart_shift(*args):
        return MPICall("MPI_Cart_shift", OrderedDict([("comm", args[0]), ("direction", args[1]), ("disp", args[2]), ("rank_source", args[3]), ("rank_dest", args[4]), ]), "1.0")

    @staticmethod
    def mpi_cart_sub(*args):
        return MPICall("MPI_Cart_sub", OrderedDict([("comm", args[0]), ("remain_dims", args[1]), ("newcomm", args[2]), ]), "1.0")

    @staticmethod
    def mpi_cartdim_get(*args):
        return MPICall("MPI_Cartdim_get", OrderedDict([("comm", args[0]), ("ndims", args[1]), ]), "1.0")

    @staticmethod
    def mpi_close_port(*args):
        return MPICall("MPI_Close_port", OrderedDict([("port_name", args[0]), ]), "2.0")

    @staticmethod
    def mpi_comm_accept(*args):
        return MPICall("MPI_Comm_accept", OrderedDict([("port_name", args[0]), ("info", args[1]), ("root", args[2]), ("comm", args[3]), ("newcomm", args[4]), ]), "2.0")

    @staticmethod
    def mpi_comm_c2f(*args):
        return MPICall("MPI_Comm_c2f", OrderedDict([("comm", args[0]), ]), "2.0")

    @staticmethod
    def mpi_comm_call_errhandler(*args):
        return MPICall("MPI_Comm_call_errhandler", OrderedDict([("comm", args[0]), ("errorcode", args[1]), ]), "2.0")

    @staticmethod
    def mpi_comm_compare(*args):
        return MPICall("MPI_Comm_compare", OrderedDict([("comm1", args[0]), ("comm2", args[1]), ("result", args[2]), ]), "1.0")

    @staticmethod
    def mpi_comm_connect(*args):
        return MPICall("MPI_Comm_connect", OrderedDict([("port_name", args[0]), ("info", args[1]), ("root", args[2]), ("comm", args[3]), ("newcomm", args[4]), ]), "2.0")

    @staticmethod
    def mpi_comm_copy_attr_function(*args):
        return MPICall("MPI_Comm_copy_attr_function", OrderedDict([("oldcomm", args[0]), ("comm_keyval", args[1]), ("extra_state", args[2]), ("attribute_val_in", args[3]), ("attribute_val_out", args[4]), ("flag", args[5]), ]), "2.0")

    @staticmethod
    def mpi_comm_create(*args):
        return MPICall("MPI_Comm_create", OrderedDict([("comm", args[0]), ("group", args[1]), ("newcomm", args[2]), ]), "1.0")

    @staticmethod
    def mpi_comm_create_errhandler(*args):
        return MPICall("MPI_Comm_create_errhandler", OrderedDict([("comm_errhandler_fn", args[0]), ("errhandler", args[1]), ]), "2.0")

    @staticmethod
    def mpi_comm_create_from_group(*args):
        return MPICall("MPI_Comm_create_from_group", OrderedDict([("group", args[0]), ("stringtag", args[1]), ("info", args[2]), ("errhandler", args[3]), ("newcomm", args[4]), ]), "4.0")

    @staticmethod
    def mpi_comm_create_group(*args):
        return MPICall("MPI_Comm_create_group", OrderedDict([("comm", args[0]), ("group", args[1]), ("tag", args[2]), ("newcomm", args[3]), ]), "3.0")

    @staticmethod
    def mpi_comm_create_keyval(*args):
        return MPICall("MPI_Comm_create_keyval", OrderedDict([("comm_copy_attr_fn", args[0]), ("comm_delete_attr_fn", args[1]), ("comm_keyval", args[2]), ("extra_state", args[3]), ]), "2.0")

    @staticmethod
    def mpi_comm_delete_attr(*args):
        return MPICall("MPI_Comm_delete_attr", OrderedDict([("comm", args[0]), ("comm_keyval", args[1]), ]), "2.0")

    @staticmethod
    def mpi_comm_delete_attr_function(*args):
        return MPICall("MPI_Comm_delete_attr_function", OrderedDict([("comm", args[0]), ("comm_keyval", args[1]), ("attribute_val", args[2]), ("extra_state", args[3]), ]), "2.0")

    @staticmethod
    def mpi_comm_disconnect(*args):
        return MPICall("MPI_Comm_disconnect", OrderedDict([("comm", args[0]), ]), "2.0")

    @staticmethod
    def mpi_comm_dup(*args):
        return MPICall("MPI_Comm_dup", OrderedDict([("comm", args[0]), ("newcomm", args[1]), ]), "1.0")

    @staticmethod
    def mpi_comm_dup_fn(*args):
        return MPICall("MPI_COMM_DUP_FN", OrderedDict([("oldcomm", args[0]), ("comm_keyval", args[1]), ("extra_state", args[2]), ("attribute_val_in", args[3]), ("attribute_val_out", args[4]), ("flag", args[5]), ]), "4.0")

    @staticmethod
    def mpi_comm_dup_with_info(*args):
        return MPICall("MPI_Comm_dup_with_info", OrderedDict([("comm", args[0]), ("info", args[1]), ("newcomm", args[2]), ]), "3.0")

    @staticmethod
    def mpi_comm_errhandler_function(*args):
        return MPICall("MPI_Comm_errhandler_function", OrderedDict([("comm", args[0]), ("error_code", args[1]), ("varargs", args[2]), ]), "2.2")

    @staticmethod
    def mpi_comm_f2c(*args):
        return MPICall("MPI_Comm_f2c", OrderedDict([("comm", args[0]), ]), "2.0")

    @staticmethod
    def mpi_comm_free(*args):
        return MPICall("MPI_Comm_free", OrderedDict([("comm", args[0]), ]), "1.0")

    @staticmethod
    def mpi_comm_free_keyval(*args):
        return MPICall("MPI_Comm_free_keyval", OrderedDict([("comm_keyval", args[0]), ]), "2.0")

    @staticmethod
    def mpi_comm_get_attr(*args):
        return MPICall("MPI_Comm_get_attr", OrderedDict([("comm", args[0]), ("comm_keyval", args[1]), ("attribute_val", args[2]), ("flag", args[3]), ]), "2.0")

    @staticmethod
    def mpi_comm_get_errhandler(*args):
        return MPICall("MPI_Comm_get_errhandler", OrderedDict([("comm", args[0]), ("errhandler", args[1]), ]), "2.0")

    @staticmethod
    def mpi_comm_get_info(*args):
        return MPICall("MPI_Comm_get_info", OrderedDict([("comm", args[0]), ("info_used", args[1]), ]), "3.0")

    @staticmethod
    def mpi_comm_get_name(*args):
        return MPICall("MPI_Comm_get_name", OrderedDict([("comm", args[0]), ("comm_name", args[1]), ("resultlen", args[2]), ]), "2.0")

    @staticmethod
    def mpi_comm_get_parent(*args):
        return MPICall("MPI_Comm_get_parent", OrderedDict([("parent", args[0]), ]), "2.0")

    @staticmethod
    def mpi_comm_group(*args):
        return MPICall("MPI_Comm_group", OrderedDict([("comm", args[0]), ("group", args[1]), ]), "1.0")

    @staticmethod
    def mpi_comm_idup(*args):
        return MPICall("MPI_Comm_idup", OrderedDict([("comm", args[0]), ("newcomm", args[1]), ("request", args[2]), ]), "3.0")

    @staticmethod
    def mpi_comm_idup_with_info(*args):
        return MPICall("MPI_Comm_idup_with_info", OrderedDict([("comm", args[0]), ("info", args[1]), ("newcomm", args[2]), ("request", args[3]), ]), "4.0")

    @staticmethod
    def mpi_comm_join(*args):
        return MPICall("MPI_Comm_join", OrderedDict([("fd", args[0]), ("intercomm", args[1]), ]), "2.0")

    @staticmethod
    def mpi_comm_null_copy_fn(*args):
        return MPICall("MPI_COMM_NULL_COPY_FN", OrderedDict([("oldcomm", args[0]), ("comm_keyval", args[1]), ("extra_state", args[2]), ("attribute_val_in", args[3]), ("attribute_val_out", args[4]), ("flag", args[5]), ]), "4.0")

    @staticmethod
    def mpi_comm_null_delete_fn(*args):
        return MPICall("MPI_COMM_NULL_DELETE_FN", OrderedDict([("comm", args[0]), ("comm_keyval", args[1]), ("attribute_val", args[2]), ("extra_state", args[3]), ]), "4.0")

    @staticmethod
    def mpi_comm_rank(*args):
        return MPICall("MPI_Comm_rank", OrderedDict([("comm", args[0]), ("rank", args[1]), ]), "1.0")

    @staticmethod
    def mpi_comm_remote_group(*args):
        return MPICall("MPI_Comm_remote_group", OrderedDict([("comm", args[0]), ("group", args[1]), ]), "1.0")

    @staticmethod
    def mpi_comm_remote_size(*args):
        return MPICall("MPI_Comm_remote_size", OrderedDict([("comm", args[0]), ("size", args[1]), ]), "1.0")

    @staticmethod
    def mpi_comm_set_attr(*args):
        return MPICall("MPI_Comm_set_attr", OrderedDict([("comm", args[0]), ("comm_keyval", args[1]), ("attribute_val", args[2]), ]), "2.0")

    @staticmethod
    def mpi_comm_set_errhandler(*args):
        return MPICall("MPI_Comm_set_errhandler", OrderedDict([("comm", args[0]), ("errhandler", args[1]), ]), "2.0")

    @staticmethod
    def mpi_comm_set_info(*args):
        return MPICall("MPI_Comm_set_info", OrderedDict([("comm", args[0]), ("info", args[1]), ]), "3.0")

    @staticmethod
    def mpi_comm_set_name(*args):
        return MPICall("MPI_Comm_set_name", OrderedDict([("comm", args[0]), ("comm_name", args[1]), ]), "2.0")

    @staticmethod
    def mpi_comm_size(*args):
        return MPICall("MPI_Comm_size", OrderedDict([("comm", args[0]), ("size", args[1]), ]), "1.0")

    @staticmethod
    def mpi_comm_spawn(*args):
        return MPICall("MPI_Comm_spawn", OrderedDict([("command", args[0]), ("argv", args[1]), ("maxprocs", args[2]), ("info", args[3]), ("root", args[4]), ("comm", args[5]), ("intercomm", args[6]), ("array_of_errcodes", args[7]), ]), "2.0")

    @staticmethod
    def mpi_comm_spawn_multiple(*args):
        return MPICall("MPI_Comm_spawn_multiple", OrderedDict([("count", args[0]), ("array_of_commands", args[1]), ("array_of_argv", args[2]), ("array_of_maxprocs", args[3]), ("array_of_info", args[4]), ("root", args[5]), ("comm", args[6]), ("intercomm", args[7]), ("array_of_errcodes", args[8]), ]), "2.0")

    @staticmethod
    def mpi_comm_split(*args):
        return MPICall("MPI_Comm_split", OrderedDict([("comm", args[0]), ("color", args[1]), ("key", args[2]), ("newcomm", args[3]), ]), "1.0")

    @staticmethod
    def mpi_comm_split_type(*args):
        return MPICall("MPI_Comm_split_type", OrderedDict([("comm", args[0]), ("split_type", args[1]), ("key", args[2]), ("info", args[3]), ("newcomm", args[4]), ]), "3.0")

    @staticmethod
    def mpi_comm_test_inter(*args):
        return MPICall("MPI_Comm_test_inter", OrderedDict([("comm", args[0]), ("flag", args[1]), ]), "1.0")

    @staticmethod
    def mpi_compare_and_swap(*args):
        return MPICall("MPI_Compare_and_swap", OrderedDict([("origin_addr", args[0]), ("compare_addr", args[1]), ("result_addr", args[2]), ("datatype", args[3]), ("target_rank", args[4]), ("target_disp", args[5]), ("win", args[6]), ]), "3.0")

    @staticmethod
    def mpi_conversion_fn_null(*args):
        return MPICall("MPI_CONVERSION_FN_NULL", OrderedDict([("userbuf", args[0]), ("datatype", args[1]), ("count", args[2]), ("filebuf", args[3]), ("position", args[4]), ("extra_state", args[5]), ]), "4.0")

    @staticmethod
    def mpi_copy_function(*args):
        return MPICall("MPI_Copy_function", OrderedDict([("oldcomm", args[0]), ("keyval", args[1]), ("extra_state", args[2]), ("attribute_val_in", args[3]), ("attribute_val_out", args[4]), ("flag", args[5]), ]), "1.0")

    @staticmethod
    def mpi_datarep_conversion_function(*args):
        return MPICall("MPI_Datarep_conversion_function", OrderedDict([("userbuf", args[0]), ("datatype", args[1]), ("count", args[2]), ("filebuf", args[3]), ("position", args[4]), ("extra_state", args[5]), ]), "4.0")

    @staticmethod
    def mpi_datarep_extent_function(*args):
        return MPICall("MPI_Datarep_extent_function", OrderedDict([("datatype", args[0]), ("extent", args[1]), ("extra_state", args[2]), ]), "4.0")

    @staticmethod
    def mpi_delete_function(*args):
        return MPICall("MPI_Delete_function", OrderedDict([("comm", args[0]), ("keyval", args[1]), ("attribute_val", args[2]), ("extra_state", args[3]), ]), "1.0")

    @staticmethod
    def mpi_dims_create(*args):
        return MPICall("MPI_Dims_create", OrderedDict([("nnodes", args[0]), ("ndims", args[1]), ("dims", args[2]), ]), "1.0")

    @staticmethod
    def mpi_dist_graph_create(*args):
        return MPICall("MPI_Dist_graph_create", OrderedDict([("comm_old", args[0]), ("n", args[1]), ("sources", args[2]), ("degrees", args[3]), ("destinations", args[4]), ("weights", args[5]), ("info", args[6]), ("reorder", args[7]), ("comm_dist_graph", args[8]), ]), "2.2")

    @staticmethod
    def mpi_dist_graph_create_adjacent(*args):
        return MPICall("MPI_Dist_graph_create_adjacent", OrderedDict([("comm_old", args[0]), ("indegree", args[1]), ("sources", args[2]), ("sourceweights", args[3]), ("outdegree", args[4]), ("destinations", args[5]), ("destweights", args[6]), ("info", args[7]), ("reorder", args[8]), ("comm_dist_graph", args[9]), ]), "2.2")

    @staticmethod
    def mpi_dist_graph_neighbors(*args):
        return MPICall("MPI_Dist_graph_neighbors", OrderedDict([("comm", args[0]), ("maxindegree", args[1]), ("sources", args[2]), ("sourceweights", args[3]), ("maxoutdegree", args[4]), ("destinations", args[5]), ("destweights", args[6]), ]), "2.2")

    @staticmethod
    def mpi_dist_graph_neighbors_count(*args):
        return MPICall("MPI_Dist_graph_neighbors_count", OrderedDict([("comm", args[0]), ("indegree", args[1]), ("outdegree", args[2]), ("weighted", args[3]), ]), "2.2")

    @staticmethod
    def mpi_dup_fn(*args):
        return MPICall("MPI_DUP_FN", OrderedDict([("oldcomm", args[0]), ("keyval", args[1]), ("extra_state", args[2]), ("attribute_val_in", args[3]), ("attribute_val_out", args[4]), ("flag", args[5]), ]), "2.2")

    @staticmethod
    def mpi_errhandler_c2f(*args):
        return MPICall("MPI_Errhandler_c2f", OrderedDict([("errhandler", args[0]), ]), "4.0")

    @staticmethod
    def mpi_errhandler_f2c(*args):
        return MPICall("MPI_Errhandler_f2c", OrderedDict([("errhandler", args[0]), ]), "4.0")

    @staticmethod
    def mpi_errhandler_free(*args):
        return MPICall("MPI_Errhandler_free", OrderedDict([("errhandler", args[0]), ]), "1.0")

    @staticmethod
    def mpi_error_class(*args):
        return MPICall("MPI_Error_class", OrderedDict([("errorcode", args[0]), ("errorclass", args[1]), ]), "1.0")

    @staticmethod
    def mpi_error_string(*args):
        return MPICall("MPI_Error_string", OrderedDict([("errorcode", args[0]), ("string", args[1]), ("resultlen", args[2]), ]), "1.0")

    @staticmethod
    def mpi_exscan(*args):
        return MPICall("MPI_Exscan", OrderedDict([("sendbuf", args[0]), ("recvbuf", args[1]), ("count", args[2]), ("datatype", args[3]), ("op", args[4]), ("comm", args[5]), ]), "2.0")

    @staticmethod
    def mpi_exscan_init(*args):
        return MPICall("MPI_Exscan_init", OrderedDict([("sendbuf", args[0]), ("recvbuf", args[1]), ("count", args[2]), ("datatype", args[3]), ("op", args[4]), ("comm", args[5]), ("info", args[6]), ("request", args[7]), ]), "4.0")

    @staticmethod
    def mpi_f_sync_reg(*args):
        return MPICall("MPI_F_sync_reg", OrderedDict([("buf", args[0]), ]), "4.0")

    @staticmethod
    def mpi_fetch_and_op(*args):
        return MPICall("MPI_Fetch_and_op", OrderedDict([("origin_addr", args[0]), ("result_addr", args[1]), ("datatype", args[2]), ("target_rank", args[3]), ("target_disp", args[4]), ("op", args[5]), ("win", args[6]), ]), "3.0")

    @staticmethod
    def mpi_file_c2f(*args):
        return MPICall("MPI_File_c2f", OrderedDict([("file", args[0]), ]), "2.0")

    @staticmethod
    def mpi_file_call_errhandler(*args):
        return MPICall("MPI_File_call_errhandler", OrderedDict([("fh", args[0]), ("errorcode", args[1]), ]), "2.0")

    @staticmethod
    def mpi_file_close(*args):
        return MPICall("MPI_File_close", OrderedDict([("fh", args[0]), ]), "2.0")

    @staticmethod
    def mpi_file_create_errhandler(*args):
        return MPICall("MPI_File_create_errhandler", OrderedDict([("file_errhandler_fn", args[0]), ("errhandler", args[1]), ]), "2.0")

    @staticmethod
    def mpi_file_delete(*args):
        return MPICall("MPI_File_delete", OrderedDict([("filename", args[0]), ("info", args[1]), ]), "2.0")

    @staticmethod
    def mpi_file_errhandler_function(*args):
        return MPICall("MPI_File_errhandler_function", OrderedDict([("file", args[0]), ("error_code", args[1]), ("varargs", args[2]), ]), "4.0")

    @staticmethod
    def mpi_file_f2c(*args):
        return MPICall("MPI_File_f2c", OrderedDict([("file", args[0]), ]), "2.0")

    @staticmethod
    def mpi_file_get_amode(*args):
        return MPICall("MPI_File_get_amode", OrderedDict([("fh", args[0]), ("amode", args[1]), ]), "2.0")

    @staticmethod
    def mpi_file_get_atomicity(*args):
        return MPICall("MPI_File_get_atomicity", OrderedDict([("fh", args[0]), ("flag", args[1]), ]), "2.0")

    @staticmethod
    def mpi_file_get_byte_offset(*args):
        return MPICall("MPI_File_get_byte_offset", OrderedDict([("fh", args[0]), ("offset", args[1]), ("disp", args[2]), ]), "2.0")

    @staticmethod
    def mpi_file_get_errhandler(*args):
        return MPICall("MPI_File_get_errhandler", OrderedDict([("file", args[0]), ("errhandler", args[1]), ]), "2.0")

    @staticmethod
    def mpi_file_get_group(*args):
        return MPICall("MPI_File_get_group", OrderedDict([("fh", args[0]), ("group", args[1]), ]), "2.0")

    @staticmethod
    def mpi_file_get_info(*args):
        return MPICall("MPI_File_get_info", OrderedDict([("fh", args[0]), ("info_used", args[1]), ]), "2.0")

    @staticmethod
    def mpi_file_get_position(*args):
        return MPICall("MPI_File_get_position", OrderedDict([("fh", args[0]), ("offset", args[1]), ]), "2.0")

    @staticmethod
    def mpi_file_get_position_shared(*args):
        return MPICall("MPI_File_get_position_shared", OrderedDict([("fh", args[0]), ("offset", args[1]), ]), "2.0")

    @staticmethod
    def mpi_file_get_size(*args):
        return MPICall("MPI_File_get_size", OrderedDict([("fh", args[0]), ("size", args[1]), ]), "2.0")

    @staticmethod
    def mpi_file_get_type_extent(*args):
        return MPICall("MPI_File_get_type_extent", OrderedDict([("fh", args[0]), ("datatype", args[1]), ("extent", args[2]), ]), "2.0")

    @staticmethod
    def mpi_file_get_view(*args):
        return MPICall("MPI_File_get_view", OrderedDict([("fh", args[0]), ("disp", args[1]), ("etype", args[2]), ("filetype", args[3]), ("datarep", args[4]), ]), "2.0")

    @staticmethod
    def mpi_file_iread(*args):
        return MPICall("MPI_File_iread", OrderedDict([("fh", args[0]), ("buf", args[1]), ("count", args[2]), ("datatype", args[3]), ("request", args[4]), ]), "2.0")

    @staticmethod
    def mpi_file_iread_all(*args):
        return MPICall("MPI_File_iread_all", OrderedDict([("fh", args[0]), ("buf", args[1]), ("count", args[2]), ("datatype", args[3]), ("request", args[4]), ]), "3.1")

    @staticmethod
    def mpi_file_iread_at(*args):
        return MPICall("MPI_File_iread_at", OrderedDict([("fh", args[0]), ("offset", args[1]), ("buf", args[2]), ("count", args[3]), ("datatype", args[4]), ("request", args[5]), ]), "2.0")

    @staticmethod
    def mpi_file_iread_at_all(*args):
        return MPICall("MPI_File_iread_at_all", OrderedDict([("fh", args[0]), ("offset", args[1]), ("buf", args[2]), ("count", args[3]), ("datatype", args[4]), ("request", args[5]), ]), "3.1")

    @staticmethod
    def mpi_file_iread_shared(*args):
        return MPICall("MPI_File_iread_shared", OrderedDict([("fh", args[0]), ("buf", args[1]), ("count", args[2]), ("datatype", args[3]), ("request", args[4]), ]), "2.0")

    @staticmethod
    def mpi_file_iwrite(*args):
        return MPICall("MPI_File_iwrite", OrderedDict([("fh", args[0]), ("buf", args[1]), ("count", args[2]), ("datatype", args[3]), ("request", args[4]), ]), "2.0")

    @staticmethod
    def mpi_file_iwrite_all(*args):
        return MPICall("MPI_File_iwrite_all", OrderedDict([("fh", args[0]), ("buf", args[1]), ("count", args[2]), ("datatype", args[3]), ("request", args[4]), ]), "3.1")

    @staticmethod
    def mpi_file_iwrite_at(*args):
        return MPICall("MPI_File_iwrite_at", OrderedDict([("fh", args[0]), ("offset", args[1]), ("buf", args[2]), ("count", args[3]), ("datatype", args[4]), ("request", args[5]), ]), "2.0")

    @staticmethod
    def mpi_file_iwrite_at_all(*args):
        return MPICall("MPI_File_iwrite_at_all", OrderedDict([("fh", args[0]), ("offset", args[1]), ("buf", args[2]), ("count", args[3]), ("datatype", args[4]), ("request", args[5]), ]), "3.1")

    @staticmethod
    def mpi_file_iwrite_shared(*args):
        return MPICall("MPI_File_iwrite_shared", OrderedDict([("fh", args[0]), ("buf", args[1]), ("count", args[2]), ("datatype", args[3]), ("request", args[4]), ]), "2.0")

    @staticmethod
    def mpi_file_open(*args):
        return MPICall("MPI_File_open", OrderedDict([("comm", args[0]), ("filename", args[1]), ("amode", args[2]), ("info", args[3]), ("fh", args[4]), ]), "2.0")

    @staticmethod
    def mpi_file_preallocate(*args):
        return MPICall("MPI_File_preallocate", OrderedDict([("fh", args[0]), ("size", args[1]), ]), "2.0")

    @staticmethod
    def mpi_file_read(*args):
        return MPICall("MPI_File_read", OrderedDict([("fh", args[0]), ("buf", args[1]), ("count", args[2]), ("datatype", args[3]), ("status", args[4]), ]), "2.0")

    @staticmethod
    def mpi_file_read_all(*args):
        return MPICall("MPI_File_read_all", OrderedDict([("fh", args[0]), ("buf", args[1]), ("count", args[2]), ("datatype", args[3]), ("status", args[4]), ]), "2.0")

    @staticmethod
    def mpi_file_read_all_begin(*args):
        return MPICall("MPI_File_read_all_begin", OrderedDict([("fh", args[0]), ("buf", args[1]), ("count", args[2]), ("datatype", args[3]), ]), "2.0")

    @staticmethod
    def mpi_file_read_all_end(*args):
        return MPICall("MPI_File_read_all_end", OrderedDict([("fh", args[0]), ("buf", args[1]), ("status", args[2]), ]), "2.0")

    @staticmethod
    def mpi_file_read_at(*args):
        return MPICall("MPI_File_read_at", OrderedDict([("fh", args[0]), ("offset", args[1]), ("buf", args[2]), ("count", args[3]), ("datatype", args[4]), ("status", args[5]), ]), "2.0")

    @staticmethod
    def mpi_file_read_at_all(*args):
        return MPICall("MPI_File_read_at_all", OrderedDict([("fh", args[0]), ("offset", args[1]), ("buf", args[2]), ("count", args[3]), ("datatype", args[4]), ("status", args[5]), ]), "2.0")

    @staticmethod
    def mpi_file_read_at_all_begin(*args):
        return MPICall("MPI_File_read_at_all_begin", OrderedDict([("fh", args[0]), ("offset", args[1]), ("buf", args[2]), ("count", args[3]), ("datatype", args[4]), ]), "2.0")

    @staticmethod
    def mpi_file_read_at_all_end(*args):
        return MPICall("MPI_File_read_at_all_end", OrderedDict([("fh", args[0]), ("buf", args[1]), ("status", args[2]), ]), "2.0")

    @staticmethod
    def mpi_file_read_ordered(*args):
        return MPICall("MPI_File_read_ordered", OrderedDict([("fh", args[0]), ("buf", args[1]), ("count", args[2]), ("datatype", args[3]), ("status", args[4]), ]), "2.0")

    @staticmethod
    def mpi_file_read_ordered_begin(*args):
        return MPICall("MPI_File_read_ordered_begin", OrderedDict([("fh", args[0]), ("buf", args[1]), ("count", args[2]), ("datatype", args[3]), ]), "2.0")

    @staticmethod
    def mpi_file_read_ordered_end(*args):
        return MPICall("MPI_File_read_ordered_end", OrderedDict([("fh", args[0]), ("buf", args[1]), ("status", args[2]), ]), "2.0")

    @staticmethod
    def mpi_file_read_shared(*args):
        return MPICall("MPI_File_read_shared", OrderedDict([("fh", args[0]), ("buf", args[1]), ("count", args[2]), ("datatype", args[3]), ("status", args[4]), ]), "2.0")

    @staticmethod
    def mpi_file_seek(*args):
        return MPICall("MPI_File_seek", OrderedDict([("fh", args[0]), ("offset", args[1]), ("whence", args[2]), ]), "2.0")

    @staticmethod
    def mpi_file_seek_shared(*args):
        return MPICall("MPI_File_seek_shared", OrderedDict([("fh", args[0]), ("offset", args[1]), ("whence", args[2]), ]), "2.0")

    @staticmethod
    def mpi_file_set_atomicity(*args):
        return MPICall("MPI_File_set_atomicity", OrderedDict([("fh", args[0]), ("flag", args[1]), ]), "2.0")

    @staticmethod
    def mpi_file_set_errhandler(*args):
        return MPICall("MPI_File_set_errhandler", OrderedDict([("file", args[0]), ("errhandler", args[1]), ]), "2.0")

    @staticmethod
    def mpi_file_set_info(*args):
        return MPICall("MPI_File_set_info", OrderedDict([("fh", args[0]), ("info", args[1]), ]), "2.0")

    @staticmethod
    def mpi_file_set_size(*args):
        return MPICall("MPI_File_set_size", OrderedDict([("fh", args[0]), ("size", args[1]), ]), "2.0")

    @staticmethod
    def mpi_file_set_view(*args):
        return MPICall("MPI_File_set_view", OrderedDict([("fh", args[0]), ("disp", args[1]), ("etype", args[2]), ("filetype", args[3]), ("datarep", args[4]), ("info", args[5]), ]), "2.0")

    @staticmethod
    def mpi_file_sync(*args):
        return MPICall("MPI_File_sync", OrderedDict([("fh", args[0]), ]), "2.0")

    @staticmethod
    def mpi_file_write(*args):
        return MPICall("MPI_File_write", OrderedDict([("fh", args[0]), ("buf", args[1]), ("count", args[2]), ("datatype", args[3]), ("status", args[4]), ]), "2.0")

    @staticmethod
    def mpi_file_write_all(*args):
        return MPICall("MPI_File_write_all", OrderedDict([("fh", args[0]), ("buf", args[1]), ("count", args[2]), ("datatype", args[3]), ("status", args[4]), ]), "2.0")

    @staticmethod
    def mpi_file_write_all_begin(*args):
        return MPICall("MPI_File_write_all_begin", OrderedDict([("fh", args[0]), ("buf", args[1]), ("count", args[2]), ("datatype", args[3]), ]), "2.0")

    @staticmethod
    def mpi_file_write_all_end(*args):
        return MPICall("MPI_File_write_all_end", OrderedDict([("fh", args[0]), ("buf", args[1]), ("status", args[2]), ]), "2.0")

    @staticmethod
    def mpi_file_write_at(*args):
        return MPICall("MPI_File_write_at", OrderedDict([("fh", args[0]), ("offset", args[1]), ("buf", args[2]), ("count", args[3]), ("datatype", args[4]), ("status", args[5]), ]), "2.0")

    @staticmethod
    def mpi_file_write_at_all(*args):
        return MPICall("MPI_File_write_at_all", OrderedDict([("fh", args[0]), ("offset", args[1]), ("buf", args[2]), ("count", args[3]), ("datatype", args[4]), ("status", args[5]), ]), "2.0")

    @staticmethod
    def mpi_file_write_at_all_begin(*args):
        return MPICall("MPI_File_write_at_all_begin", OrderedDict([("fh", args[0]), ("offset", args[1]), ("buf", args[2]), ("count", args[3]), ("datatype", args[4]), ]), "2.0")

    @staticmethod
    def mpi_file_write_at_all_end(*args):
        return MPICall("MPI_File_write_at_all_end", OrderedDict([("fh", args[0]), ("buf", args[1]), ("status", args[2]), ]), "2.0")

    @staticmethod
    def mpi_file_write_ordered(*args):
        return MPICall("MPI_File_write_ordered", OrderedDict([("fh", args[0]), ("buf", args[1]), ("count", args[2]), ("datatype", args[3]), ("status", args[4]), ]), "2.0")

    @staticmethod
    def mpi_file_write_ordered_begin(*args):
        return MPICall("MPI_File_write_ordered_begin", OrderedDict([("fh", args[0]), ("buf", args[1]), ("count", args[2]), ("datatype", args[3]), ]), "2.0")

    @staticmethod
    def mpi_file_write_ordered_end(*args):
        return MPICall("MPI_File_write_ordered_end", OrderedDict([("fh", args[0]), ("buf", args[1]), ("status", args[2]), ]), "2.0")

    @staticmethod
    def mpi_file_write_shared(*args):
        return MPICall("MPI_File_write_shared", OrderedDict([("fh", args[0]), ("buf", args[1]), ("count", args[2]), ("datatype", args[3]), ("status", args[4]), ]), "2.0")

    @staticmethod
    def mpi_finalize(*args):
        return MPICall("MPI_Finalize", OrderedDict([]), "1.0")

    @staticmethod
    def mpi_finalized(*args):
        return MPICall("MPI_Finalized", OrderedDict([("flag", args[0]), ]), "2.0")

    @staticmethod
    def mpi_free_mem(*args):
        return MPICall("MPI_Free_mem", OrderedDict([("base", args[0]), ]), "2.0")

    @staticmethod
    def mpi_gather(*args):
        return MPICall("MPI_Gather", OrderedDict([("sendbuf", args[0]), ("sendcount", args[1]), ("sendtype", args[2]), ("recvbuf", args[3]), ("recvcount", args[4]), ("recvtype", args[5]), ("root", args[6]), ("comm", args[7]), ]), "1.0")

    @staticmethod
    def mpi_gather_init(*args):
        return MPICall("MPI_Gather_init", OrderedDict([("sendbuf", args[0]), ("sendcount", args[1]), ("sendtype", args[2]), ("recvbuf", args[3]), ("recvcount", args[4]), ("recvtype", args[5]), ("root", args[6]), ("comm", args[7]), ("info", args[8]), ("request", args[9]), ]), "4.0")

    @staticmethod
    def mpi_gatherv(*args):
        return MPICall("MPI_Gatherv", OrderedDict([("sendbuf", args[0]), ("sendcount", args[1]), ("sendtype", args[2]), ("recvbuf", args[3]), ("recvcounts", args[4]), ("displs", args[5]), ("recvtype", args[6]), ("root", args[7]), ("comm", args[8]), ]), "1.0")

    @staticmethod
    def mpi_gatherv_init(*args):
        return MPICall("MPI_Gatherv_init", OrderedDict([("sendbuf", args[0]), ("sendcount", args[1]), ("sendtype", args[2]), ("recvbuf", args[3]), ("recvcounts", args[4]), ("displs", args[5]), ("recvtype", args[6]), ("root", args[7]), ("comm", args[8]), ("info", args[9]), ("request", args[10]), ]), "4.0")

    @staticmethod
    def mpi_get(*args):
        return MPICall("MPI_Get", OrderedDict([("origin_addr", args[0]), ("origin_count", args[1]), ("origin_datatype", args[2]), ("target_rank", args[3]), ("target_disp", args[4]), ("target_count", args[5]), ("target_datatype", args[6]), ("win", args[7]), ]), "2.0")

    @staticmethod
    def mpi_get_accumulate(*args):
        return MPICall("MPI_Get_accumulate", OrderedDict([("origin_addr", args[0]), ("origin_count", args[1]), ("origin_datatype", args[2]), ("result_addr", args[3]), ("result_count", args[4]), ("result_datatype", args[5]), ("target_rank", args[6]), ("target_disp", args[7]), ("target_count", args[8]), ("target_datatype", args[9]), ("op", args[10]), ("win", args[11]), ]), "3.0")

    @staticmethod
    def mpi_get_address(*args):
        return MPICall("MPI_Get_address", OrderedDict([("location", args[0]), ("address", args[1]), ]), "2.0")

    @staticmethod
    def mpi_get_count(*args):
        return MPICall("MPI_Get_count", OrderedDict([("status", args[0]), ("datatype", args[1]), ("count", args[2]), ]), "1.0")

    @staticmethod
    def mpi_get_elements(*args):
        return MPICall("MPI_Get_elements", OrderedDict([("status", args[0]), ("datatype", args[1]), ("count", args[2]), ]), "1.0")

    @staticmethod
    def mpi_get_elements_x(*args):
        return MPICall("MPI_Get_elements_x", OrderedDict([("status", args[0]), ("datatype", args[1]), ("count", args[2]), ]), "3.0")

    @staticmethod
    def mpi_get_library_version(*args):
        return MPICall("MPI_Get_library_version", OrderedDict([("version", args[0]), ("resultlen", args[1]), ]), "3.0")

    @staticmethod
    def mpi_get_processor_name(*args):
        return MPICall("MPI_Get_processor_name", OrderedDict([("name", args[0]), ("resultlen", args[1]), ]), "1.0")

    @staticmethod
    def mpi_get_version(*args):
        return MPICall("MPI_Get_version", OrderedDict([("version", args[0]), ("subversion", args[1]), ]), "1.0")

    @staticmethod
    def mpi_graph_create(*args):
        return MPICall("MPI_Graph_create", OrderedDict([("comm_old", args[0]), ("nnodes", args[1]), ("index", args[2]), ("edges", args[3]), ("reorder", args[4]), ("comm_graph", args[5]), ]), "1.0")

    @staticmethod
    def mpi_graph_get(*args):
        return MPICall("MPI_Graph_get", OrderedDict([("comm", args[0]), ("maxindex", args[1]), ("maxedges", args[2]), ("index", args[3]), ("edges", args[4]), ]), "1.0")

    @staticmethod
    def mpi_graph_map(*args):
        return MPICall("MPI_Graph_map", OrderedDict([("comm", args[0]), ("nnodes", args[1]), ("index", args[2]), ("edges", args[3]), ("newrank", args[4]), ]), "1.0")

    @staticmethod
    def mpi_graph_neighbors(*args):
        return MPICall("MPI_Graph_neighbors", OrderedDict([("comm", args[0]), ("rank", args[1]), ("maxneighbors", args[2]), ("neighbors", args[3]), ]), "1.0")

    @staticmethod
    def mpi_graph_neighbors_count(*args):
        return MPICall("MPI_Graph_neighbors_count", OrderedDict([("comm", args[0]), ("rank", args[1]), ("nneighbors", args[2]), ]), "1.0")

    @staticmethod
    def mpi_graphdims_get(*args):
        return MPICall("MPI_Graphdims_get", OrderedDict([("comm", args[0]), ("nnodes", args[1]), ("nedges", args[2]), ]), "1.0")

    @staticmethod
    def mpi_grequest_cancel_function(*args):
        return MPICall("MPI_Grequest_cancel_function", OrderedDict([("extra_state", args[0]), ("complete", args[1]), ]), "2.0")

    @staticmethod
    def mpi_grequest_complete(*args):
        return MPICall("MPI_Grequest_complete", OrderedDict([("request", args[0]), ]), "2.0")

    @staticmethod
    def mpi_grequest_free_function(*args):
        return MPICall("MPI_Grequest_free_function", OrderedDict([("extra_state", args[0]), ]), "2.0")

    @staticmethod
    def mpi_grequest_query_function(*args):
        return MPICall("MPI_Grequest_query_function", OrderedDict([("extra_state", args[0]), ("status", args[1]), ]), "2.0")

    @staticmethod
    def mpi_grequest_start(*args):
        return MPICall("MPI_Grequest_start", OrderedDict([("query_fn", args[0]), ("free_fn", args[1]), ("cancel_fn", args[2]), ("extra_state", args[3]), ("request", args[4]), ]), "2.0")

    @staticmethod
    def mpi_group_c2f(*args):
        return MPICall("MPI_Group_c2f", OrderedDict([("group", args[0]), ]), "2.0")

    @staticmethod
    def mpi_group_compare(*args):
        return MPICall("MPI_Group_compare", OrderedDict([("group1", args[0]), ("group2", args[1]), ("result", args[2]), ]), "1.0")

    @staticmethod
    def mpi_group_difference(*args):
        return MPICall("MPI_Group_difference", OrderedDict([("group1", args[0]), ("group2", args[1]), ("newgroup", args[2]), ]), "1.0")

    @staticmethod
    def mpi_group_excl(*args):
        return MPICall("MPI_Group_excl", OrderedDict([("group", args[0]), ("n", args[1]), ("ranks", args[2]), ("newgroup", args[3]), ]), "1.0")

    @staticmethod
    def mpi_group_f2c(*args):
        return MPICall("MPI_Group_f2c", OrderedDict([("group", args[0]), ]), "2.0")

    @staticmethod
    def mpi_group_free(*args):
        return MPICall("MPI_Group_free", OrderedDict([("group", args[0]), ]), "1.0")

    @staticmethod
    def mpi_group_from_session_pset(*args):
        return MPICall("MPI_Group_from_session_pset", OrderedDict([("session", args[0]), ("pset_name", args[1]), ("newgroup", args[2]), ]), "4.0")

    @staticmethod
    def mpi_group_incl(*args):
        return MPICall("MPI_Group_incl", OrderedDict([("group", args[0]), ("n", args[1]), ("ranks", args[2]), ("newgroup", args[3]), ]), "1.0")

    @staticmethod
    def mpi_group_intersection(*args):
        return MPICall("MPI_Group_intersection", OrderedDict([("group1", args[0]), ("group2", args[1]), ("newgroup", args[2]), ]), "1.0")

    @staticmethod
    def mpi_group_range_excl(*args):
        return MPICall("MPI_Group_range_excl", OrderedDict([("group", args[0]), ("n", args[1]), ("ranges", args[2]), ("newgroup", args[3]), ]), "1.0")

    @staticmethod
    def mpi_group_range_incl(*args):
        return MPICall("MPI_Group_range_incl", OrderedDict([("group", args[0]), ("n", args[1]), ("ranges", args[2]), ("newgroup", args[3]), ]), "1.0")

    @staticmethod
    def mpi_group_rank(*args):
        return MPICall("MPI_Group_rank", OrderedDict([("group", args[0]), ("rank", args[1]), ]), "1.0")

    @staticmethod
    def mpi_group_size(*args):
        return MPICall("MPI_Group_size", OrderedDict([("group", args[0]), ("size", args[1]), ]), "1.0")

    @staticmethod
    def mpi_group_translate_ranks(*args):
        return MPICall("MPI_Group_translate_ranks", OrderedDict([("group1", args[0]), ("n", args[1]), ("ranks1", args[2]), ("group2", args[3]), ("ranks2", args[4]), ]), "1.0")

    @staticmethod
    def mpi_group_union(*args):
        return MPICall("MPI_Group_union", OrderedDict([("group1", args[0]), ("group2", args[1]), ("newgroup", args[2]), ]), "1.0")

    @staticmethod
    def mpi_iallgather(*args):
        return MPICall("MPI_Iallgather", OrderedDict([("sendbuf", args[0]), ("sendcount", args[1]), ("sendtype", args[2]), ("recvbuf", args[3]), ("recvcount", args[4]), ("recvtype", args[5]), ("comm", args[6]), ("request", args[7]), ]), "3.0")

    @staticmethod
    def mpi_iallgatherv(*args):
        return MPICall("MPI_Iallgatherv", OrderedDict([("sendbuf", args[0]), ("sendcount", args[1]), ("sendtype", args[2]), ("recvbuf", args[3]), ("recvcounts", args[4]), ("displs", args[5]), ("recvtype", args[6]), ("comm", args[7]), ("request", args[8]), ]), "3.0")

    @staticmethod
    def mpi_iallreduce(*args):
        return MPICall("MPI_Iallreduce", OrderedDict([("sendbuf", args[0]), ("recvbuf", args[1]), ("count", args[2]), ("datatype", args[3]), ("op", args[4]), ("comm", args[5]), ("request", args[6]), ]), "3.0")

    @staticmethod
    def mpi_ialltoall(*args):
        return MPICall("MPI_Ialltoall", OrderedDict([("sendbuf", args[0]), ("sendcount", args[1]), ("sendtype", args[2]), ("recvbuf", args[3]), ("recvcount", args[4]), ("recvtype", args[5]), ("comm", args[6]), ("request", args[7]), ]), "3.0")

    @staticmethod
    def mpi_ialltoallv(*args):
        return MPICall("MPI_Ialltoallv", OrderedDict([("sendbuf", args[0]), ("sendcounts", args[1]), ("sdispls", args[2]), ("sendtype", args[3]), ("recvbuf", args[4]), ("recvcounts", args[5]), ("rdispls", args[6]), ("recvtype", args[7]), ("comm", args[8]), ("request", args[9]), ]), "3.0")

    @staticmethod
    def mpi_ialltoallw(*args):
        return MPICall("MPI_Ialltoallw", OrderedDict([("sendbuf", args[0]), ("sendcounts", args[1]), ("sdispls", args[2]), ("sendtypes", args[3]), ("recvbuf", args[4]), ("recvcounts", args[5]), ("rdispls", args[6]), ("recvtypes", args[7]), ("comm", args[8]), ("request", args[9]), ]), "3.0")

    @staticmethod
    def mpi_ibarrier(*args):
        return MPICall("MPI_Ibarrier", OrderedDict([("comm", args[0]), ("request", args[1]), ]), "3.0")

    @staticmethod
    def mpi_ibcast(*args):
        return MPICall("MPI_Ibcast", OrderedDict([("buffer", args[0]), ("count", args[1]), ("datatype", args[2]), ("root", args[3]), ("comm", args[4]), ("request", args[5]), ]), "3.0")

    @staticmethod
    def mpi_ibsend(*args):
        return MPICall("MPI_Ibsend", OrderedDict([("buf", args[0]), ("count", args[1]), ("datatype", args[2]), ("dest", args[3]), ("tag", args[4]), ("comm", args[5]), ("request", args[6]), ]), "1.0")

    @staticmethod
    def mpi_iexscan(*args):
        return MPICall("MPI_Iexscan", OrderedDict([("sendbuf", args[0]), ("recvbuf", args[1]), ("count", args[2]), ("datatype", args[3]), ("op", args[4]), ("comm", args[5]), ("request", args[6]), ]), "3.0")

    @staticmethod
    def mpi_igather(*args):
        return MPICall("MPI_Igather", OrderedDict([("sendbuf", args[0]), ("sendcount", args[1]), ("sendtype", args[2]), ("recvbuf", args[3]), ("recvcount", args[4]), ("recvtype", args[5]), ("root", args[6]), ("comm", args[7]), ("request", args[8]), ]), "3.0")

    @staticmethod
    def mpi_igatherv(*args):
        return MPICall("MPI_Igatherv", OrderedDict([("sendbuf", args[0]), ("sendcount", args[1]), ("sendtype", args[2]), ("recvbuf", args[3]), ("recvcounts", args[4]), ("displs", args[5]), ("recvtype", args[6]), ("root", args[7]), ("comm", args[8]), ("request", args[9]), ]), "3.0")

    @staticmethod
    def mpi_improbe(*args):
        return MPICall("MPI_Improbe", OrderedDict([("source", args[0]), ("tag", args[1]), ("comm", args[2]), ("flag", args[3]), ("message", args[4]), ("status", args[5]), ]), "3.0")

    @staticmethod
    def mpi_imrecv(*args):
        return MPICall("MPI_Imrecv", OrderedDict([("buf", args[0]), ("count", args[1]), ("datatype", args[2]), ("message", args[3]), ("request", args[4]), ]), "3.0")

    @staticmethod
    def mpi_ineighbor_allgather(*args):
        return MPICall("MPI_Ineighbor_allgather", OrderedDict([("sendbuf", args[0]), ("sendcount", args[1]), ("sendtype", args[2]), ("recvbuf", args[3]), ("recvcount", args[4]), ("recvtype", args[5]), ("comm", args[6]), ("request", args[7]), ]), "3.0")

    @staticmethod
    def mpi_ineighbor_allgatherv(*args):
        return MPICall("MPI_Ineighbor_allgatherv", OrderedDict([("sendbuf", args[0]), ("sendcount", args[1]), ("sendtype", args[2]), ("recvbuf", args[3]), ("recvcounts", args[4]), ("displs", args[5]), ("recvtype", args[6]), ("comm", args[7]), ("request", args[8]), ]), "3.0")

    @staticmethod
    def mpi_ineighbor_alltoall(*args):
        return MPICall("MPI_Ineighbor_alltoall", OrderedDict([("sendbuf", args[0]), ("sendcount", args[1]), ("sendtype", args[2]), ("recvbuf", args[3]), ("recvcount", args[4]), ("recvtype", args[5]), ("comm", args[6]), ("request", args[7]), ]), "3.0")

    @staticmethod
    def mpi_ineighbor_alltoallv(*args):
        return MPICall("MPI_Ineighbor_alltoallv", OrderedDict([("sendbuf", args[0]), ("sendcounts", args[1]), ("sdispls", args[2]), ("sendtype", args[3]), ("recvbuf", args[4]), ("recvcounts", args[5]), ("rdispls", args[6]), ("recvtype", args[7]), ("comm", args[8]), ("request", args[9]), ]), "3.0")

    @staticmethod
    def mpi_ineighbor_alltoallw(*args):
        return MPICall("MPI_Ineighbor_alltoallw", OrderedDict([("sendbuf", args[0]), ("sendcounts", args[1]), ("sdispls", args[2]), ("sendtypes", args[3]), ("recvbuf", args[4]), ("recvcounts", args[5]), ("rdispls", args[6]), ("recvtypes", args[7]), ("comm", args[8]), ("request", args[9]), ]), "3.0")

    @staticmethod
    def mpi_info_c2f(*args):
        return MPICall("MPI_Info_c2f", OrderedDict([("info", args[0]), ]), "2.0")

    @staticmethod
    def mpi_info_create(*args):
        return MPICall("MPI_Info_create", OrderedDict([("info", args[0]), ]), "2.0")

    @staticmethod
    def mpi_info_create_env(*args):
        return MPICall("MPI_Info_create_env", OrderedDict([("argc", args[0]), ("argv", args[1]), ("info", args[2]), ]), "4.0")

    @staticmethod
    def mpi_info_delete(*args):
        return MPICall("MPI_Info_delete", OrderedDict([("info", args[0]), ("key", args[1]), ]), "2.0")

    @staticmethod
    def mpi_info_dup(*args):
        return MPICall("MPI_Info_dup", OrderedDict([("info", args[0]), ("newinfo", args[1]), ]), "2.0")

    @staticmethod
    def mpi_info_f2c(*args):
        return MPICall("MPI_Info_f2c", OrderedDict([("info", args[0]), ]), "2.0")

    @staticmethod
    def mpi_info_free(*args):
        return MPICall("MPI_Info_free", OrderedDict([("info", args[0]), ]), "2.0")

    @staticmethod
    def mpi_info_get(*args):
        return MPICall("MPI_Info_get", OrderedDict([("info", args[0]), ("key", args[1]), ("valuelen", args[2]), ("value", args[3]), ("flag", args[4]), ]), "2.0")

    @staticmethod
    def mpi_info_get_nkeys(*args):
        return MPICall("MPI_Info_get_nkeys", OrderedDict([("info", args[0]), ("nkeys", args[1]), ]), "2.0")

    @staticmethod
    def mpi_info_get_nthkey(*args):
        return MPICall("MPI_Info_get_nthkey", OrderedDict([("info", args[0]), ("n", args[1]), ("key", args[2]), ]), "2.0")

    @staticmethod
    def mpi_info_get_string(*args):
        return MPICall("MPI_Info_get_string", OrderedDict([("info", args[0]), ("key", args[1]), ("buflen", args[2]), ("value", args[3]), ("flag", args[4]), ]), "4.0")

    @staticmethod
    def mpi_info_get_valuelen(*args):
        return MPICall("MPI_Info_get_valuelen", OrderedDict([("info", args[0]), ("key", args[1]), ("valuelen", args[2]), ("flag", args[3]), ]), "2.0")

    @staticmethod
    def mpi_info_set(*args):
        return MPICall("MPI_Info_set", OrderedDict([("info", args[0]), ("key", args[1]), ("value", args[2]), ]), "2.0")

    @staticmethod
    def mpi_init(*args):
        return MPICall("MPI_Init", OrderedDict([("argc", args[0]), ("argv", args[1]), ]), "1.0")

    @staticmethod
    def mpi_init_thread(*args):
        return MPICall("MPI_Init_thread", OrderedDict([("argc", args[0]), ("argv", args[1]), ("required", args[2]), ("provided", args[3]), ]), "2.0")

    @staticmethod
    def mpi_initialized(*args):
        return MPICall("MPI_Initialized", OrderedDict([("flag", args[0]), ]), "1.0")

    @staticmethod
    def mpi_intercomm_create(*args):
        return MPICall("MPI_Intercomm_create", OrderedDict([("local_comm", args[0]), ("local_leader", args[1]), ("peer_comm", args[2]), ("remote_leader", args[3]), ("tag", args[4]), ("newintercomm", args[5]), ]), "1.0")

    @staticmethod
    def mpi_intercomm_create_from_groups(*args):
        return MPICall("MPI_Intercomm_create_from_groups", OrderedDict([("local_group", args[0]), ("local_leader", args[1]), ("remote_group", args[2]), ("remote_leader", args[3]), ("stringtag", args[4]), ("info", args[5]), ("errhandler", args[6]), ("newintercomm", args[7]), ]), "4.0")

    @staticmethod
    def mpi_intercomm_merge(*args):
        return MPICall("MPI_Intercomm_merge", OrderedDict([("intercomm", args[0]), ("high", args[1]), ("newintracomm", args[2]), ]), "1.0")

    @staticmethod
    def mpi_iprobe(*args):
        return MPICall("MPI_Iprobe", OrderedDict([("source", args[0]), ("tag", args[1]), ("comm", args[2]), ("flag", args[3]), ("status", args[4]), ]), "1.0")

    @staticmethod
    def mpi_irecv(*args):
        return MPICall("MPI_Irecv", OrderedDict([("buf", args[0]), ("count", args[1]), ("datatype", args[2]), ("source", args[3]), ("tag", args[4]), ("comm", args[5]), ("request", args[6]), ]), "1.0")

    @staticmethod
    def mpi_ireduce(*args):
        return MPICall("MPI_Ireduce", OrderedDict([("sendbuf", args[0]), ("recvbuf", args[1]), ("count", args[2]), ("datatype", args[3]), ("op", args[4]), ("root", args[5]), ("comm", args[6]), ("request", args[7]), ]), "3.0")

    @staticmethod
    def mpi_ireduce_scatter(*args):
        return MPICall("MPI_Ireduce_scatter", OrderedDict([("sendbuf", args[0]), ("recvbuf", args[1]), ("recvcounts", args[2]), ("datatype", args[3]), ("op", args[4]), ("comm", args[5]), ("request", args[6]), ]), "3.0")

    @staticmethod
    def mpi_ireduce_scatter_block(*args):
        return MPICall("MPI_Ireduce_scatter_block", OrderedDict([("sendbuf", args[0]), ("recvbuf", args[1]), ("recvcount", args[2]), ("datatype", args[3]), ("op", args[4]), ("comm", args[5]), ("request", args[6]), ]), "3.0")

    @staticmethod
    def mpi_irsend(*args):
        return MPICall("MPI_Irsend", OrderedDict([("buf", args[0]), ("count", args[1]), ("datatype", args[2]), ("dest", args[3]), ("tag", args[4]), ("comm", args[5]), ("request", args[6]), ]), "1.0")

    @staticmethod
    def mpi_is_thread_main(*args):
        return MPICall("MPI_Is_thread_main", OrderedDict([("flag", args[0]), ]), "2.0")

    @staticmethod
    def mpi_iscan(*args):
        return MPICall("MPI_Iscan", OrderedDict([("sendbuf", args[0]), ("recvbuf", args[1]), ("count", args[2]), ("datatype", args[3]), ("op", args[4]), ("comm", args[5]), ("request", args[6]), ]), "3.0")

    @staticmethod
    def mpi_iscatter(*args):
        return MPICall("MPI_Iscatter", OrderedDict([("sendbuf", args[0]), ("sendcount", args[1]), ("sendtype", args[2]), ("recvbuf", args[3]), ("recvcount", args[4]), ("recvtype", args[5]), ("root", args[6]), ("comm", args[7]), ("request", args[8]), ]), "3.0")

    @staticmethod
    def mpi_iscatterv(*args):
        return MPICall("MPI_Iscatterv", OrderedDict([("sendbuf", args[0]), ("sendcounts", args[1]), ("displs", args[2]), ("sendtype", args[3]), ("recvbuf", args[4]), ("recvcount", args[5]), ("recvtype", args[6]), ("root", args[7]), ("comm", args[8]), ("request", args[9]), ]), "3.0")

    @staticmethod
    def mpi_isend(*args):
        return MPICall("MPI_Isend", OrderedDict([("buf", args[0]), ("count", args[1]), ("datatype", args[2]), ("dest", args[3]), ("tag", args[4]), ("comm", args[5]), ("request", args[6]), ]), "1.0")

    @staticmethod
    def mpi_isendrecv(*args):
        return MPICall("MPI_Isendrecv", OrderedDict([("sendbuf", args[0]), ("sendcount", args[1]), ("sendtype", args[2]), ("dest", args[3]), ("sendtag", args[4]), ("recvbuf", args[5]), ("recvcount", args[6]), ("recvtype", args[7]), ("source", args[8]), ("recvtag", args[9]), ("comm", args[10]), ("request", args[11]), ]), "4.0")

    @staticmethod
    def mpi_isendrecv_replace(*args):
        return MPICall("MPI_Isendrecv_replace", OrderedDict([("buf", args[0]), ("count", args[1]), ("datatype", args[2]), ("dest", args[3]), ("sendtag", args[4]), ("source", args[5]), ("recvtag", args[6]), ("comm", args[7]), ("request", args[8]), ]), "4.0")

    @staticmethod
    def mpi_issend(*args):
        return MPICall("MPI_Issend", OrderedDict([("buf", args[0]), ("count", args[1]), ("datatype", args[2]), ("dest", args[3]), ("tag", args[4]), ("comm", args[5]), ("request", args[6]), ]), "1.0")

    @staticmethod
    def mpi_keyval_create(*args):
        return MPICall("MPI_Keyval_create", OrderedDict([("copy_fn", args[0]), ("delete_fn", args[1]), ("keyval", args[2]), ("extra_state", args[3]), ]), "1.0")

    @staticmethod
    def mpi_keyval_free(*args):
        return MPICall("MPI_Keyval_free", OrderedDict([("keyval", args[0]), ]), "1.0")

    @staticmethod
    def mpi_lookup_name(*args):
        return MPICall("MPI_Lookup_name", OrderedDict([("service_name", args[0]), ("info", args[1]), ("port_name", args[2]), ]), "2.0")

    @staticmethod
    def mpi_message_c2f(*args):
        return MPICall("MPI_Message_c2f", OrderedDict([("message", args[0]), ]), "4.0")

    @staticmethod
    def mpi_message_f2c(*args):
        return MPICall("MPI_Message_f2c", OrderedDict([("message", args[0]), ]), "4.0")

    @staticmethod
    def mpi_mprobe(*args):
        return MPICall("MPI_Mprobe", OrderedDict([("source", args[0]), ("tag", args[1]), ("comm", args[2]), ("message", args[3]), ("status", args[4]), ]), "3.0")

    @staticmethod
    def mpi_mrecv(*args):
        return MPICall("MPI_Mrecv", OrderedDict([("buf", args[0]), ("count", args[1]), ("datatype", args[2]), ("message", args[3]), ("status", args[4]), ]), "3.0")

    @staticmethod
    def mpi_neighbor_allgather(*args):
        return MPICall("MPI_Neighbor_allgather", OrderedDict([("sendbuf", args[0]), ("sendcount", args[1]), ("sendtype", args[2]), ("recvbuf", args[3]), ("recvcount", args[4]), ("recvtype", args[5]), ("comm", args[6]), ]), "3.0")

    @staticmethod
    def mpi_neighbor_allgather_init(*args):
        return MPICall("MPI_Neighbor_allgather_init", OrderedDict([("sendbuf", args[0]), ("sendcount", args[1]), ("sendtype", args[2]), ("recvbuf", args[3]), ("recvcount", args[4]), ("recvtype", args[5]), ("comm", args[6]), ("info", args[7]), ("request", args[8]), ]), "4.0")

    @staticmethod
    def mpi_neighbor_allgatherv(*args):
        return MPICall("MPI_Neighbor_allgatherv", OrderedDict([("sendbuf", args[0]), ("sendcount", args[1]), ("sendtype", args[2]), ("recvbuf", args[3]), ("recvcounts", args[4]), ("displs", args[5]), ("recvtype", args[6]), ("comm", args[7]), ]), "3.0")

    @staticmethod
    def mpi_neighbor_allgatherv_init(*args):
        return MPICall("MPI_Neighbor_allgatherv_init", OrderedDict([("sendbuf", args[0]), ("sendcount", args[1]), ("sendtype", args[2]), ("recvbuf", args[3]), ("recvcounts", args[4]), ("displs", args[5]), ("recvtype", args[6]), ("comm", args[7]), ("info", args[8]), ("request", args[9]), ]), "4.0")

    @staticmethod
    def mpi_neighbor_alltoall(*args):
        return MPICall("MPI_Neighbor_alltoall", OrderedDict([("sendbuf", args[0]), ("sendcount", args[1]), ("sendtype", args[2]), ("recvbuf", args[3]), ("recvcount", args[4]), ("recvtype", args[5]), ("comm", args[6]), ]), "3.0")

    @staticmethod
    def mpi_neighbor_alltoall_init(*args):
        return MPICall("MPI_Neighbor_alltoall_init", OrderedDict([("sendbuf", args[0]), ("sendcount", args[1]), ("sendtype", args[2]), ("recvbuf", args[3]), ("recvcount", args[4]), ("recvtype", args[5]), ("comm", args[6]), ("info", args[7]), ("request", args[8]), ]), "4.0")

    @staticmethod
    def mpi_neighbor_alltoallv(*args):
        return MPICall("MPI_Neighbor_alltoallv", OrderedDict([("sendbuf", args[0]), ("sendcounts", args[1]), ("sdispls", args[2]), ("sendtype", args[3]), ("recvbuf", args[4]), ("recvcounts", args[5]), ("rdispls", args[6]), ("recvtype", args[7]), ("comm", args[8]), ]), "3.0")

    @staticmethod
    def mpi_neighbor_alltoallv_init(*args):
        return MPICall("MPI_Neighbor_alltoallv_init", OrderedDict([("sendbuf", args[0]), ("sendcounts", args[1]), ("sdispls", args[2]), ("sendtype", args[3]), ("recvbuf", args[4]), ("recvcounts", args[5]), ("rdispls", args[6]), ("recvtype", args[7]), ("comm", args[8]), ("info", args[9]), ("request", args[10]), ]), "4.0")

    @staticmethod
    def mpi_neighbor_alltoallw(*args):
        return MPICall("MPI_Neighbor_alltoallw", OrderedDict([("sendbuf", args[0]), ("sendcounts", args[1]), ("sdispls", args[2]), ("sendtypes", args[3]), ("recvbuf", args[4]), ("recvcounts", args[5]), ("rdispls", args[6]), ("recvtypes", args[7]), ("comm", args[8]), ]), "3.0")

    @staticmethod
    def mpi_neighbor_alltoallw_init(*args):
        return MPICall("MPI_Neighbor_alltoallw_init", OrderedDict([("sendbuf", args[0]), ("sendcounts", args[1]), ("sdispls", args[2]), ("sendtypes", args[3]), ("recvbuf", args[4]), ("recvcounts", args[5]), ("rdispls", args[6]), ("recvtypes", args[7]), ("comm", args[8]), ("info", args[9]), ("request", args[10]), ]), "4.0")

    @staticmethod
    def mpi_null_copy_fn(*args):
        return MPICall("MPI_NULL_COPY_FN", OrderedDict([("oldcomm", args[0]), ("keyval", args[1]), ("extra_state", args[2]), ("attribute_val_in", args[3]), ("attribute_val_out", args[4]), ("flag", args[5]), ]), "4.0")

    @staticmethod
    def mpi_null_delete_fn(*args):
        return MPICall("MPI_NULL_DELETE_FN", OrderedDict([("comm", args[0]), ("keyval", args[1]), ("attribute_val", args[2]), ("extra_state", args[3]), ]), "4.0")

    @staticmethod
    def mpi_op_c2f(*args):
        return MPICall("MPI_Op_c2f", OrderedDict([("op", args[0]), ]), "2.0")

    @staticmethod
    def mpi_op_commutative(*args):
        return MPICall("MPI_Op_commutative", OrderedDict([("op", args[0]), ("commute", args[1]), ]), "2.2")

    @staticmethod
    def mpi_op_create(*args):
        return MPICall("MPI_Op_create", OrderedDict([("user_fn", args[0]), ("commute", args[1]), ("op", args[2]), ]), "1.0")

    @staticmethod
    def mpi_op_f2c(*args):
        return MPICall("MPI_Op_f2c", OrderedDict([("op", args[0]), ]), "2.0")

    @staticmethod
    def mpi_op_free(*args):
        return MPICall("MPI_Op_free", OrderedDict([("op", args[0]), ]), "1.0")

    @staticmethod
    def mpi_open_port(*args):
        return MPICall("MPI_Open_port", OrderedDict([("info", args[0]), ("port_name", args[1]), ]), "2.0")

    @staticmethod
    def mpi_pack(*args):
        return MPICall("MPI_Pack", OrderedDict([("inbuf", args[0]), ("incount", args[1]), ("datatype", args[2]), ("outbuf", args[3]), ("outsize", args[4]), ("position", args[5]), ("comm", args[6]), ]), "1.0")

    @staticmethod
    def mpi_pack_external(*args):
        return MPICall("MPI_Pack_external", OrderedDict([("datarep", args[0]), ("inbuf", args[1]), ("incount", args[2]), ("datatype", args[3]), ("outbuf", args[4]), ("outsize", args[5]), ("position", args[6]), ]), "2.0")

    @staticmethod
    def mpi_pack_external_size(*args):
        return MPICall("MPI_Pack_external_size", OrderedDict([("datarep", args[0]), ("incount", args[1]), ("datatype", args[2]), ("size", args[3]), ]), "2.0")

    @staticmethod
    def mpi_pack_size(*args):
        return MPICall("MPI_Pack_size", OrderedDict([("incount", args[0]), ("datatype", args[1]), ("comm", args[2]), ("size", args[3]), ]), "1.0")

    @staticmethod
    def mpi_parrived(*args):
        return MPICall("MPI_Parrived", OrderedDict([("request", args[0]), ("partition", args[1]), ("flag", args[2]), ]), "4.0")

    @staticmethod
    def mpi_pcontrol(*args):
        return MPICall("MPI_Pcontrol", OrderedDict([("level", args[0]), ("varargs", args[1]), ]), "1.0")

    @staticmethod
    def mpi_pready(*args):
        return MPICall("MPI_Pready", OrderedDict([("partition", args[0]), ("request", args[1]), ]), "4.0")

    @staticmethod
    def mpi_pready_list(*args):
        return MPICall("MPI_Pready_list", OrderedDict([("length", args[0]), ("array_of_partitions", args[1]), ("request", args[2]), ]), "4.0")

    @staticmethod
    def mpi_pready_range(*args):
        return MPICall("MPI_Pready_range", OrderedDict([("partition_low", args[0]), ("partition_high", args[1]), ("request", args[2]), ]), "4.0")

    @staticmethod
    def mpi_precv_init(*args):
        return MPICall("MPI_Precv_init", OrderedDict([("buf", args[0]), ("partitions", args[1]), ("count", args[2]), ("datatype", args[3]), ("source", args[4]), ("tag", args[5]), ("comm", args[6]), ("info", args[7]), ("request", args[8]), ]), "4.0")

    @staticmethod
    def mpi_probe(*args):
        return MPICall("MPI_Probe", OrderedDict([("source", args[0]), ("tag", args[1]), ("comm", args[2]), ("status", args[3]), ]), "1.0")

    @staticmethod
    def mpi_psend_init(*args):
        return MPICall("MPI_Psend_init", OrderedDict([("buf", args[0]), ("partitions", args[1]), ("count", args[2]), ("datatype", args[3]), ("dest", args[4]), ("tag", args[5]), ("comm", args[6]), ("info", args[7]), ("request", args[8]), ]), "4.0")

    @staticmethod
    def mpi_publish_name(*args):
        return MPICall("MPI_Publish_name", OrderedDict([("service_name", args[0]), ("info", args[1]), ("port_name", args[2]), ]), "2.0")

    @staticmethod
    def mpi_put(*args):
        return MPICall("MPI_Put", OrderedDict([("origin_addr", args[0]), ("origin_count", args[1]), ("origin_datatype", args[2]), ("target_rank", args[3]), ("target_disp", args[4]), ("target_count", args[5]), ("target_datatype", args[6]), ("win", args[7]), ]), "2.0")

    @staticmethod
    def mpi_query_thread(*args):
        return MPICall("MPI_Query_thread", OrderedDict([("provided", args[0]), ]), "2.0")

    @staticmethod
    def mpi_raccumulate(*args):
        return MPICall("MPI_Raccumulate", OrderedDict([("origin_addr", args[0]), ("origin_count", args[1]), ("origin_datatype", args[2]), ("target_rank", args[3]), ("target_disp", args[4]), ("target_count", args[5]), ("target_datatype", args[6]), ("op", args[7]), ("win", args[8]), ("request", args[9]), ]), "3.0")

    @staticmethod
    def mpi_recv(*args):
        return MPICall("MPI_Recv", OrderedDict([("buf", args[0]), ("count", args[1]), ("datatype", args[2]), ("source", args[3]), ("tag", args[4]), ("comm", args[5]), ("status", args[6]), ]), "1.0")

    @staticmethod
    def mpi_recv_init(*args):
        return MPICall("MPI_Recv_init", OrderedDict([("buf", args[0]), ("count", args[1]), ("datatype", args[2]), ("source", args[3]), ("tag", args[4]), ("comm", args[5]), ("request", args[6]), ]), "1.0")

    @staticmethod
    def mpi_reduce(*args):
        return MPICall("MPI_Reduce", OrderedDict([("sendbuf", args[0]), ("recvbuf", args[1]), ("count", args[2]), ("datatype", args[3]), ("op", args[4]), ("root", args[5]), ("comm", args[6]), ]), "1.0")

    @staticmethod
    def mpi_reduce_init(*args):
        return MPICall("MPI_Reduce_init", OrderedDict([("sendbuf", args[0]), ("recvbuf", args[1]), ("count", args[2]), ("datatype", args[3]), ("op", args[4]), ("root", args[5]), ("comm", args[6]), ("info", args[7]), ("request", args[8]), ]), "4.0")

    @staticmethod
    def mpi_reduce_local(*args):
        return MPICall("MPI_Reduce_local", OrderedDict([("inbuf", args[0]), ("inoutbuf", args[1]), ("count", args[2]), ("datatype", args[3]), ("op", args[4]), ]), "2.2")

    @staticmethod
    def mpi_reduce_scatter(*args):
        return MPICall("MPI_Reduce_scatter", OrderedDict([("sendbuf", args[0]), ("recvbuf", args[1]), ("recvcounts", args[2]), ("datatype", args[3]), ("op", args[4]), ("comm", args[5]), ]), "1.0")

    @staticmethod
    def mpi_reduce_scatter_block(*args):
        return MPICall("MPI_Reduce_scatter_block", OrderedDict([("sendbuf", args[0]), ("recvbuf", args[1]), ("recvcount", args[2]), ("datatype", args[3]), ("op", args[4]), ("comm", args[5]), ]), "2.2")

    @staticmethod
    def mpi_reduce_scatter_block_init(*args):
        return MPICall("MPI_Reduce_scatter_block_init", OrderedDict([("sendbuf", args[0]), ("recvbuf", args[1]), ("recvcount", args[2]), ("datatype", args[3]), ("op", args[4]), ("comm", args[5]), ("info", args[6]), ("request", args[7]), ]), "4.0")

    @staticmethod
    def mpi_reduce_scatter_init(*args):
        return MPICall("MPI_Reduce_scatter_init", OrderedDict([("sendbuf", args[0]), ("recvbuf", args[1]), ("recvcounts", args[2]), ("datatype", args[3]), ("op", args[4]), ("comm", args[5]), ("info", args[6]), ("request", args[7]), ]), "4.0")

    @staticmethod
    def mpi_register_datarep(*args):
        return MPICall("MPI_Register_datarep", OrderedDict([("datarep", args[0]), ("read_conversion_fn", args[1]), ("write_conversion_fn", args[2]), ("dtype_file_extent_fn", args[3]), ("extra_state", args[4]), ]), "2.0")

    @staticmethod
    def mpi_request_c2f(*args):
        return MPICall("MPI_Request_c2f", OrderedDict([("request", args[0]), ]), "2.0")

    @staticmethod
    def mpi_request_f2c(*args):
        return MPICall("MPI_Request_f2c", OrderedDict([("request", args[0]), ]), "2.0")

    @staticmethod
    def mpi_request_free(*args):
        return MPICall("MPI_Request_free", OrderedDict([("request", args[0]), ]), "1.0")

    @staticmethod
    def mpi_request_get_status(*args):
        return MPICall("MPI_Request_get_status", OrderedDict([("request", args[0]), ("flag", args[1]), ("status", args[2]), ]), "2.0")

    @staticmethod
    def mpi_rget(*args):
        return MPICall("MPI_Rget", OrderedDict([("origin_addr", args[0]), ("origin_count", args[1]), ("origin_datatype", args[2]), ("target_rank", args[3]), ("target_disp", args[4]), ("target_count", args[5]), ("target_datatype", args[6]), ("win", args[7]), ("request", args[8]), ]), "3.0")

    @staticmethod
    def mpi_rget_accumulate(*args):
        return MPICall("MPI_Rget_accumulate", OrderedDict([("origin_addr", args[0]), ("origin_count", args[1]), ("origin_datatype", args[2]), ("result_addr", args[3]), ("result_count", args[4]), ("result_datatype", args[5]), ("target_rank", args[6]), ("target_disp", args[7]), ("target_count", args[8]), ("target_datatype", args[9]), ("op", args[10]), ("win", args[11]), ("request", args[12]), ]), "3.0")

    @staticmethod
    def mpi_rput(*args):
        return MPICall("MPI_Rput", OrderedDict([("origin_addr", args[0]), ("origin_count", args[1]), ("origin_datatype", args[2]), ("target_rank", args[3]), ("target_disp", args[4]), ("target_count", args[5]), ("target_datatype", args[6]), ("win", args[7]), ("request", args[8]), ]), "3.0")

    @staticmethod
    def mpi_rsend(*args):
        return MPICall("MPI_Rsend", OrderedDict([("buf", args[0]), ("count", args[1]), ("datatype", args[2]), ("dest", args[3]), ("tag", args[4]), ("comm", args[5]), ]), "1.0")

    @staticmethod
    def mpi_rsend_init(*args):
        return MPICall("MPI_Rsend_init", OrderedDict([("buf", args[0]), ("count", args[1]), ("datatype", args[2]), ("dest", args[3]), ("tag", args[4]), ("comm", args[5]), ("request", args[6]), ]), "1.0")

    @staticmethod
    def mpi_scan(*args):
        return MPICall("MPI_Scan", OrderedDict([("sendbuf", args[0]), ("recvbuf", args[1]), ("count", args[2]), ("datatype", args[3]), ("op", args[4]), ("comm", args[5]), ]), "1.0")

    @staticmethod
    def mpi_scan_init(*args):
        return MPICall("MPI_Scan_init", OrderedDict([("sendbuf", args[0]), ("recvbuf", args[1]), ("count", args[2]), ("datatype", args[3]), ("op", args[4]), ("comm", args[5]), ("info", args[6]), ("request", args[7]), ]), "4.0")

    @staticmethod
    def mpi_scatter(*args):
        return MPICall("MPI_Scatter", OrderedDict([("sendbuf", args[0]), ("sendcount", args[1]), ("sendtype", args[2]), ("recvbuf", args[3]), ("recvcount", args[4]), ("recvtype", args[5]), ("root", args[6]), ("comm", args[7]), ]), "1.0")

    @staticmethod
    def mpi_scatter_init(*args):
        return MPICall("MPI_Scatter_init", OrderedDict([("sendbuf", args[0]), ("sendcount", args[1]), ("sendtype", args[2]), ("recvbuf", args[3]), ("recvcount", args[4]), ("recvtype", args[5]), ("root", args[6]), ("comm", args[7]), ("info", args[8]), ("request", args[9]), ]), "4.0")

    @staticmethod
    def mpi_scatterv(*args):
        return MPICall("MPI_Scatterv", OrderedDict([("sendbuf", args[0]), ("sendcounts", args[1]), ("displs", args[2]), ("sendtype", args[3]), ("recvbuf", args[4]), ("recvcount", args[5]), ("recvtype", args[6]), ("root", args[7]), ("comm", args[8]), ]), "1.0")

    @staticmethod
    def mpi_scatterv_init(*args):
        return MPICall("MPI_Scatterv_init", OrderedDict([("sendbuf", args[0]), ("sendcounts", args[1]), ("displs", args[2]), ("sendtype", args[3]), ("recvbuf", args[4]), ("recvcount", args[5]), ("recvtype", args[6]), ("root", args[7]), ("comm", args[8]), ("info", args[9]), ("request", args[10]), ]), "4.0")

    @staticmethod
    def mpi_send(*args):
        return MPICall("MPI_Send", OrderedDict([("buf", args[0]), ("count", args[1]), ("datatype", args[2]), ("dest", args[3]), ("tag", args[4]), ("comm", args[5]), ]), "1.0")

    @staticmethod
    def mpi_send_init(*args):
        return MPICall("MPI_Send_init", OrderedDict([("buf", args[0]), ("count", args[1]), ("datatype", args[2]), ("dest", args[3]), ("tag", args[4]), ("comm", args[5]), ("request", args[6]), ]), "1.0")

    @staticmethod
    def mpi_sendrecv(*args):
        return MPICall("MPI_Sendrecv", OrderedDict([("sendbuf", args[0]), ("sendcount", args[1]), ("sendtype", args[2]), ("dest", args[3]), ("sendtag", args[4]), ("recvbuf", args[5]), ("recvcount", args[6]), ("recvtype", args[7]), ("source", args[8]), ("recvtag", args[9]), ("comm", args[10]), ("status", args[11]), ]), "1.0")

    @staticmethod
    def mpi_sendrecv_replace(*args):
        return MPICall("MPI_Sendrecv_replace", OrderedDict([("buf", args[0]), ("count", args[1]), ("datatype", args[2]), ("dest", args[3]), ("sendtag", args[4]), ("source", args[5]), ("recvtag", args[6]), ("comm", args[7]), ("status", args[8]), ]), "1.0")

    @staticmethod
    def mpi_session_c2f(*args):
        return MPICall("MPI_Session_c2f", OrderedDict([("session", args[0]), ]), "4.0")

    @staticmethod
    def mpi_session_call_errhandler(*args):
        return MPICall("MPI_Session_call_errhandler", OrderedDict([("session", args[0]), ("errorcode", args[1]), ]), "4.0")

    @staticmethod
    def mpi_session_create_errhandler(*args):
        return MPICall("MPI_Session_create_errhandler", OrderedDict([("session_errhandler_fn", args[0]), ("errhandler", args[1]), ]), "4.0")

    @staticmethod
    def mpi_session_errhandler_function(*args):
        return MPICall("MPI_Session_errhandler_function", OrderedDict([("session", args[0]), ("error_code", args[1]), ("varargs", args[2]), ]), "4.0")

    @staticmethod
    def mpi_session_f2c(*args):
        return MPICall("MPI_Session_f2c", OrderedDict([("session", args[0]), ]), "4.0")

    @staticmethod
    def mpi_session_finalize(*args):
        return MPICall("MPI_Session_finalize", OrderedDict([("session", args[0]), ]), "4.0")

    @staticmethod
    def mpi_session_get_errhandler(*args):
        return MPICall("MPI_Session_get_errhandler", OrderedDict([("session", args[0]), ("errhandler", args[1]), ]), "4.0")

    @staticmethod
    def mpi_session_get_info(*args):
        return MPICall("MPI_Session_get_info", OrderedDict([("session", args[0]), ("info_used", args[1]), ]), "4.0")

    @staticmethod
    def mpi_session_get_nth_pset(*args):
        return MPICall("MPI_Session_get_nth_pset", OrderedDict([("session", args[0]), ("info", args[1]), ("n", args[2]), ("pset_len", args[3]), ("pset_name", args[4]), ]), "4.0")

    @staticmethod
    def mpi_session_get_num_psets(*args):
        return MPICall("MPI_Session_get_num_psets", OrderedDict([("session", args[0]), ("info", args[1]), ("npset_names", args[2]), ]), "4.0")

    @staticmethod
    def mpi_session_get_pset_info(*args):
        return MPICall("MPI_Session_get_pset_info", OrderedDict([("session", args[0]), ("pset_name", args[1]), ("info", args[2]), ]), "4.0")

    @staticmethod
    def mpi_session_init(*args):
        return MPICall("MPI_Session_init", OrderedDict([("info", args[0]), ("errhandler", args[1]), ("session", args[2]), ]), "4.0")

    @staticmethod
    def mpi_session_set_errhandler(*args):
        return MPICall("MPI_Session_set_errhandler", OrderedDict([("session", args[0]), ("errhandler", args[1]), ]), "4.0")

    @staticmethod
    def mpi_sizeof(*args):
        return MPICall("MPI_Sizeof", OrderedDict([("x", args[0]), ("size", args[1]), ]), "2.0")

    @staticmethod
    def mpi_ssend(*args):
        return MPICall("MPI_Ssend", OrderedDict([("buf", args[0]), ("count", args[1]), ("datatype", args[2]), ("dest", args[3]), ("tag", args[4]), ("comm", args[5]), ]), "1.0")

    @staticmethod
    def mpi_ssend_init(*args):
        return MPICall("MPI_Ssend_init", OrderedDict([("buf", args[0]), ("count", args[1]), ("datatype", args[2]), ("dest", args[3]), ("tag", args[4]), ("comm", args[5]), ("request", args[6]), ]), "1.0")

    @staticmethod
    def mpi_start(*args):
        return MPICall("MPI_Start", OrderedDict([("request", args[0]), ]), "1.0")

    @staticmethod
    def mpi_startall(*args):
        return MPICall("MPI_Startall", OrderedDict([("count", args[0]), ("array_of_requests", args[1]), ]), "1.0")

    @staticmethod
    def mpi_status_c2f(*args):
        return MPICall("MPI_Status_c2f", OrderedDict([("c_status", args[0]), ("f_status", args[1]), ]), "2.0")

    @staticmethod
    def mpi_status_c2f08(*args):
        return MPICall("MPI_Status_c2f08", OrderedDict([("c_status", args[0]), ("f08_status", args[1]), ]), "4.0")

    @staticmethod
    def mpi_status_f082c(*args):
        return MPICall("MPI_Status_f082c", OrderedDict([("f08_status", args[0]), ("c_status", args[1]), ]), "4.0")

    @staticmethod
    def mpi_status_f082f(*args):
        return MPICall("MPI_Status_f082f", OrderedDict([("f08_status", args[0]), ("f_status", args[1]), ]), "4.0")

    @staticmethod
    def mpi_status_f2c(*args):
        return MPICall("MPI_Status_f2c", OrderedDict([("f_status", args[0]), ("c_status", args[1]), ]), "2.0")

    @staticmethod
    def mpi_status_f2f08(*args):
        return MPICall("MPI_Status_f2f08", OrderedDict([("f_status", args[0]), ("f08_status", args[1]), ]), "4.0")

    @staticmethod
    def mpi_status_set_cancelled(*args):
        return MPICall("MPI_Status_set_cancelled", OrderedDict([("status", args[0]), ("flag", args[1]), ]), "2.0")

    @staticmethod
    def mpi_status_set_elements(*args):
        return MPICall("MPI_Status_set_elements", OrderedDict([("status", args[0]), ("datatype", args[1]), ("count", args[2]), ]), "2.0")

    @staticmethod
    def mpi_status_set_elements_x(*args):
        return MPICall("MPI_Status_set_elements_x", OrderedDict([("status", args[0]), ("datatype", args[1]), ("count", args[2]), ]), "3.0")

    @staticmethod
    def mpi_t_category_changed(*args):
        return MPICall("MPI_T_category_changed", OrderedDict([("update_number", args[0]), ]), "3.0")

    @staticmethod
    def mpi_t_category_get_categories(*args):
        return MPICall("MPI_T_category_get_categories", OrderedDict([("cat_index", args[0]), ("len", args[1]), ("indices", args[2]), ]), "3.0")

    @staticmethod
    def mpi_t_category_get_cvars(*args):
        return MPICall("MPI_T_category_get_cvars", OrderedDict([("cat_index", args[0]), ("len", args[1]), ("indices", args[2]), ]), "3.0")

    @staticmethod
    def mpi_t_category_get_events(*args):
        return MPICall("MPI_T_category_get_events", OrderedDict([("cat_index", args[0]), ("len", args[1]), ("indices", args[2]), ]), "4.0")

    @staticmethod
    def mpi_t_category_get_index(*args):
        return MPICall("MPI_T_category_get_index", OrderedDict([("name", args[0]), ("cat_index", args[1]), ]), "3.0")

    @staticmethod
    def mpi_t_category_get_info(*args):
        return MPICall("MPI_T_category_get_info", OrderedDict([("cat_index", args[0]), ("name", args[1]), ("name_len", args[2]), ("desc", args[3]), ("desc_len", args[4]), ("num_cvars", args[5]), ("num_pvars", args[6]), ("num_categories", args[7]), ]), "3.0")

    @staticmethod
    def mpi_t_category_get_num(*args):
        return MPICall("MPI_T_category_get_num", OrderedDict([("num_cat", args[0]), ]), "3.0")

    @staticmethod
    def mpi_t_category_get_num_events(*args):
        return MPICall("MPI_T_category_get_num_events", OrderedDict([("cat_index", args[0]), ("num_events", args[1]), ]), "4.0")

    @staticmethod
    def mpi_t_category_get_pvars(*args):
        return MPICall("MPI_T_category_get_pvars", OrderedDict([("cat_index", args[0]), ("len", args[1]), ("indices", args[2]), ]), "3.0")

    @staticmethod
    def mpi_t_cvar_get_index(*args):
        return MPICall("MPI_T_cvar_get_index", OrderedDict([("name", args[0]), ("cvar_index", args[1]), ]), "3.0")

    @staticmethod
    def mpi_t_cvar_get_info(*args):
        return MPICall("MPI_T_cvar_get_info", OrderedDict([("cvar_index", args[0]), ("name", args[1]), ("name_len", args[2]), ("verbosity", args[3]), ("datatype", args[4]), ("enumtype", args[5]), ("desc", args[6]), ("desc_len", args[7]), ("bind", args[8]), ("scope", args[9]), ]), "3.0")

    @staticmethod
    def mpi_t_cvar_get_num(*args):
        return MPICall("MPI_T_cvar_get_num", OrderedDict([("num_cvar", args[0]), ]), "3.0")

    @staticmethod
    def mpi_t_cvar_handle_alloc(*args):
        return MPICall("MPI_T_cvar_handle_alloc", OrderedDict([("cvar_index", args[0]), ("obj_handle", args[1]), ("handle", args[2]), ("count", args[3]), ]), "3.0")

    @staticmethod
    def mpi_t_cvar_handle_free(*args):
        return MPICall("MPI_T_cvar_handle_free", OrderedDict([("handle", args[0]), ]), "3.0")

    @staticmethod
    def mpi_t_cvar_read(*args):
        return MPICall("MPI_T_cvar_read", OrderedDict([("handle", args[0]), ("buf", args[1]), ]), "3.0")

    @staticmethod
    def mpi_t_cvar_write(*args):
        return MPICall("MPI_T_cvar_write", OrderedDict([("handle", args[0]), ("buf", args[1]), ]), "3.0")

    @staticmethod
    def mpi_t_enum_get_info(*args):
        return MPICall("MPI_T_enum_get_info", OrderedDict([("enumtype", args[0]), ("num", args[1]), ("name", args[2]), ("name_len", args[3]), ]), "3.0")

    @staticmethod
    def mpi_t_enum_get_item(*args):
        return MPICall("MPI_T_enum_get_item", OrderedDict([("enumtype", args[0]), ("index", args[1]), ("value", args[2]), ("name", args[3]), ("name_len", args[4]), ]), "3.0")

    @staticmethod
    def mpi_t_event_callback_get_info(*args):
        return MPICall("MPI_T_event_callback_get_info", OrderedDict([("event_registration", args[0]), ("cb_safety", args[1]), ("info_used", args[2]), ]), "4.0")

    @staticmethod
    def mpi_t_event_callback_set_info(*args):
        return MPICall("MPI_T_event_callback_set_info", OrderedDict([("event_registration", args[0]), ("cb_safety", args[1]), ("info", args[2]), ]), "4.0")

    @staticmethod
    def mpi_t_event_cb_function(*args):
        return MPICall("MPI_T_event_cb_function", OrderedDict([("event_instance", args[0]), ("event_registration", args[1]), ("cb_safety", args[2]), ("user_data", args[3]), ]), "4.0")

    @staticmethod
    def mpi_t_event_copy(*args):
        return MPICall("MPI_T_event_copy", OrderedDict([("event_instance", args[0]), ("buffer", args[1]), ]), "4.0")

    @staticmethod
    def mpi_t_event_dropped_cb_function(*args):
        return MPICall("MPI_T_event_dropped_cb_function", OrderedDict([("count", args[0]), ("event_registration", args[1]), ("source_index", args[2]), ("cb_safety", args[3]), ("user_data", args[4]), ]), "4.0")

    @staticmethod
    def mpi_t_event_free_cb_function(*args):
        return MPICall("MPI_T_event_free_cb_function", OrderedDict([("event_registration", args[0]), ("cb_safety", args[1]), ("user_data", args[2]), ]), "4.0")

    @staticmethod
    def mpi_t_event_get_index(*args):
        return MPICall("MPI_T_event_get_index", OrderedDict([("name", args[0]), ("event_index", args[1]), ]), "4.0")

    @staticmethod
    def mpi_t_event_get_info(*args):
        return MPICall("MPI_T_event_get_info", OrderedDict([("event_index", args[0]), ("name", args[1]), ("name_len", args[2]), ("verbosity", args[3]), ("array_of_datatypes", args[4]), ("array_of_displacements", args[5]), ("num_elements", args[6]), ("enumtype", args[7]), ("info", args[8]), ("desc", args[9]), ("desc_len", args[10]), ("bind", args[11]), ]), "4.0")

    @staticmethod
    def mpi_t_event_get_num(*args):
        return MPICall("MPI_T_event_get_num", OrderedDict([("num_events", args[0]), ]), "4.0")

    @staticmethod
    def mpi_t_event_get_source(*args):
        return MPICall("MPI_T_event_get_source", OrderedDict([("event_instance", args[0]), ("source_index", args[1]), ]), "4.0")

    @staticmethod
    def mpi_t_event_get_timestamp(*args):
        return MPICall("MPI_T_event_get_timestamp", OrderedDict([("event_instance", args[0]), ("event_timestamp", args[1]), ]), "4.0")

    @staticmethod
    def mpi_t_event_handle_alloc(*args):
        return MPICall("MPI_T_event_handle_alloc", OrderedDict([("event_index", args[0]), ("obj_handle", args[1]), ("info", args[2]), ("event_registration", args[3]), ]), "4.0")

    @staticmethod
    def mpi_t_event_handle_free(*args):
        return MPICall("MPI_T_event_handle_free", OrderedDict([("event_registration", args[0]), ("user_data", args[1]), ("free_cb_function", args[2]), ]), "4.0")

    @staticmethod
    def mpi_t_event_handle_get_info(*args):
        return MPICall("MPI_T_event_handle_get_info", OrderedDict([("event_registration", args[0]), ("info_used", args[1]), ]), "4.0")

    @staticmethod
    def mpi_t_event_handle_set_info(*args):
        return MPICall("MPI_T_event_handle_set_info", OrderedDict([("event_registration", args[0]), ("info", args[1]), ]), "4.0")

    @staticmethod
    def mpi_t_event_read(*args):
        return MPICall("MPI_T_event_read", OrderedDict([("event_instance", args[0]), ("element_index", args[1]), ("buffer", args[2]), ]), "4.0")

    @staticmethod
    def mpi_t_event_register_callback(*args):
        return MPICall("MPI_T_event_register_callback", OrderedDict([("event_registration", args[0]), ("cb_safety", args[1]), ("info", args[2]), ("user_data", args[3]), ("event_cb_function", args[4]), ]), "4.0")

    @staticmethod
    def mpi_t_event_set_dropped_handler(*args):
        return MPICall("MPI_T_event_set_dropped_handler", OrderedDict([("event_registration", args[0]), ("dropped_cb_function", args[1]), ]), "4.0")

    @staticmethod
    def mpi_t_finalize(*args):
        return MPICall("MPI_T_finalize", OrderedDict([]), "3.0")

    @staticmethod
    def mpi_t_init_thread(*args):
        return MPICall("MPI_T_init_thread", OrderedDict([("required", args[0]), ("provided", args[1]), ]), "3.0")

    @staticmethod
    def mpi_t_pvar_get_index(*args):
        return MPICall("MPI_T_pvar_get_index", OrderedDict([("name", args[0]), ("var_class", args[1]), ("pvar_index", args[2]), ]), "3.0")

    @staticmethod
    def mpi_t_pvar_get_info(*args):
        return MPICall("MPI_T_pvar_get_info", OrderedDict([("pvar_index", args[0]), ("name", args[1]), ("name_len", args[2]), ("verbosity", args[3]), ("var_class", args[4]), ("datatype", args[5]), ("enumtype", args[6]), ("desc", args[7]), ("desc_len", args[8]), ("bind", args[9]), ("readonly", args[10]), ("continuous", args[11]), ("atomic", args[12]), ]), "3.0")

    @staticmethod
    def mpi_t_pvar_get_num(*args):
        return MPICall("MPI_T_pvar_get_num", OrderedDict([("num_pvar", args[0]), ]), "3.0")

    @staticmethod
    def mpi_t_pvar_handle_alloc(*args):
        return MPICall("MPI_T_pvar_handle_alloc", OrderedDict([("pe_session", args[0]), ("pvar_index", args[1]), ("obj_handle", args[2]), ("handle", args[3]), ("count", args[4]), ]), "3.0")

    @staticmethod
    def mpi_t_pvar_handle_free(*args):
        return MPICall("MPI_T_pvar_handle_free", OrderedDict([("pe_session", args[0]), ("handle", args[1]), ]), "3.0")

    @staticmethod
    def mpi_t_pvar_read(*args):
        return MPICall("MPI_T_pvar_read", OrderedDict([("pe_session", args[0]), ("handle", args[1]), ("buf", args[2]), ]), "3.0")

    @staticmethod
    def mpi_t_pvar_readreset(*args):
        return MPICall("MPI_T_pvar_readreset", OrderedDict([("pe_session", args[0]), ("handle", args[1]), ("buf", args[2]), ]), "3.0")

    @staticmethod
    def mpi_t_pvar_reset(*args):
        return MPICall("MPI_T_pvar_reset", OrderedDict([("pe_session", args[0]), ("handle", args[1]), ]), "3.0")

    @staticmethod
    def mpi_t_pvar_session_create(*args):
        return MPICall("MPI_T_pvar_session_create", OrderedDict([("pe_session", args[0]), ]), "3.0")

    @staticmethod
    def mpi_t_pvar_session_free(*args):
        return MPICall("MPI_T_pvar_session_free", OrderedDict([("pe_session", args[0]), ]), "3.0")

    @staticmethod
    def mpi_t_pvar_start(*args):
        return MPICall("MPI_T_pvar_start", OrderedDict([("pe_session", args[0]), ("handle", args[1]), ]), "3.0")

    @staticmethod
    def mpi_t_pvar_stop(*args):
        return MPICall("MPI_T_pvar_stop", OrderedDict([("pe_session", args[0]), ("handle", args[1]), ]), "3.0")

    @staticmethod
    def mpi_t_pvar_write(*args):
        return MPICall("MPI_T_pvar_write", OrderedDict([("pe_session", args[0]), ("handle", args[1]), ("buf", args[2]), ]), "3.0")

    @staticmethod
    def mpi_t_source_get_info(*args):
        return MPICall("MPI_T_source_get_info", OrderedDict([("source_index", args[0]), ("name", args[1]), ("name_len", args[2]), ("desc", args[3]), ("desc_len", args[4]), ("ordering", args[5]), ("ticks_per_second", args[6]), ("max_ticks", args[7]), ("info", args[8]), ]), "4.0")

    @staticmethod
    def mpi_t_source_get_num(*args):
        return MPICall("MPI_T_source_get_num", OrderedDict([("num_sources", args[0]), ]), "4.0")

    @staticmethod
    def mpi_t_source_get_timestamp(*args):
        return MPICall("MPI_T_source_get_timestamp", OrderedDict([("source_index", args[0]), ("timestamp", args[1]), ]), "4.0")

    @staticmethod
    def mpi_test(*args):
        return MPICall("MPI_Test", OrderedDict([("request", args[0]), ("flag", args[1]), ("status", args[2]), ]), "1.0")

    @staticmethod
    def mpi_test_cancelled(*args):
        return MPICall("MPI_Test_cancelled", OrderedDict([("status", args[0]), ("flag", args[1]), ]), "1.0")

    @staticmethod
    def mpi_testall(*args):
        return MPICall("MPI_Testall", OrderedDict([("count", args[0]), ("array_of_requests", args[1]), ("flag", args[2]), ("array_of_statuses", args[3]), ]), "1.0")

    @staticmethod
    def mpi_testany(*args):
        return MPICall("MPI_Testany", OrderedDict([("count", args[0]), ("array_of_requests", args[1]), ("index", args[2]), ("flag", args[3]), ("status", args[4]), ]), "1.0")

    @staticmethod
    def mpi_testsome(*args):
        return MPICall("MPI_Testsome", OrderedDict([("incount", args[0]), ("array_of_requests", args[1]), ("outcount", args[2]), ("array_of_indices", args[3]), ("array_of_statuses", args[4]), ]), "1.0")

    @staticmethod
    def mpi_topo_test(*args):
        return MPICall("MPI_Topo_test", OrderedDict([("comm", args[0]), ("status", args[1]), ]), "1.0")

    @staticmethod
    def mpi_type_c2f(*args):
        return MPICall("MPI_Type_c2f", OrderedDict([("datatype", args[0]), ]), "2.0")

    @staticmethod
    def mpi_type_commit(*args):
        return MPICall("MPI_Type_commit", OrderedDict([("datatype", args[0]), ]), "1.0")

    @staticmethod
    def mpi_type_contiguous(*args):
        return MPICall("MPI_Type_contiguous", OrderedDict([("count", args[0]), ("oldtype", args[1]), ("newtype", args[2]), ]), "1.0")

    @staticmethod
    def mpi_type_copy_attr_function(*args):
        return MPICall("MPI_Type_copy_attr_function", OrderedDict([("oldtype", args[0]), ("type_keyval", args[1]), ("extra_state", args[2]), ("attribute_val_in", args[3]), ("attribute_val_out", args[4]), ("flag", args[5]), ]), "4.0")

    @staticmethod
    def mpi_type_create_darray(*args):
        return MPICall("MPI_Type_create_darray", OrderedDict([("size", args[0]), ("rank", args[1]), ("ndims", args[2]), ("array_of_gsizes", args[3]), ("array_of_distribs", args[4]), ("array_of_dargs", args[5]), ("array_of_psizes", args[6]), ("order", args[7]), ("oldtype", args[8]), ("newtype", args[9]), ]), "2.0")

    @staticmethod
    def mpi_type_create_f90_complex(*args):
        return MPICall("MPI_Type_create_f90_complex", OrderedDict([("p", args[0]), ("r", args[1]), ("newtype", args[2]), ]), "2.0")

    @staticmethod
    def mpi_type_create_f90_integer(*args):
        return MPICall("MPI_Type_create_f90_integer", OrderedDict([("r", args[0]), ("newtype", args[1]), ]), "2.0")

    @staticmethod
    def mpi_type_create_f90_real(*args):
        return MPICall("MPI_Type_create_f90_real", OrderedDict([("p", args[0]), ("r", args[1]), ("newtype", args[2]), ]), "2.0")

    @staticmethod
    def mpi_type_create_hindexed(*args):
        return MPICall("MPI_Type_create_hindexed", OrderedDict([("count", args[0]), ("array_of_blocklengths", args[1]), ("array_of_displacements", args[2]), ("oldtype", args[3]), ("newtype", args[4]), ]), "2.0")

    @staticmethod
    def mpi_type_create_hindexed_block(*args):
        return MPICall("MPI_Type_create_hindexed_block", OrderedDict([("count", args[0]), ("blocklength", args[1]), ("array_of_displacements", args[2]), ("oldtype", args[3]), ("newtype", args[4]), ]), "3.0")

    @staticmethod
    def mpi_type_create_hvector(*args):
        return MPICall("MPI_Type_create_hvector", OrderedDict([("count", args[0]), ("blocklength", args[1]), ("stride", args[2]), ("oldtype", args[3]), ("newtype", args[4]), ]), "2.0")

    @staticmethod
    def mpi_type_create_indexed_block(*args):
        return MPICall("MPI_Type_create_indexed_block", OrderedDict([("count", args[0]), ("blocklength", args[1]), ("array_of_displacements", args[2]), ("oldtype", args[3]), ("newtype", args[4]), ]), "2.0")

    @staticmethod
    def mpi_type_create_keyval(*args):
        return MPICall("MPI_Type_create_keyval", OrderedDict([("type_copy_attr_fn", args[0]), ("type_delete_attr_fn", args[1]), ("type_keyval", args[2]), ("extra_state", args[3]), ]), "2.0")

    @staticmethod
    def mpi_type_create_resized(*args):
        return MPICall("MPI_Type_create_resized", OrderedDict([("oldtype", args[0]), ("lb", args[1]), ("extent", args[2]), ("newtype", args[3]), ]), "2.0")

    @staticmethod
    def mpi_type_create_struct(*args):
        return MPICall("MPI_Type_create_struct", OrderedDict([("count", args[0]), ("array_of_blocklengths", args[1]), ("array_of_displacements", args[2]), ("array_of_types", args[3]), ("newtype", args[4]), ]), "2.0")

    @staticmethod
    def mpi_type_create_subarray(*args):
        return MPICall("MPI_Type_create_subarray", OrderedDict([("ndims", args[0]), ("array_of_sizes", args[1]), ("array_of_subsizes", args[2]), ("array_of_starts", args[3]), ("order", args[4]), ("oldtype", args[5]), ("newtype", args[6]), ]), "2.0")

    @staticmethod
    def mpi_type_delete_attr(*args):
        return MPICall("MPI_Type_delete_attr", OrderedDict([("datatype", args[0]), ("type_keyval", args[1]), ]), "2.0")

    @staticmethod
    def mpi_type_delete_attr_function(*args):
        return MPICall("MPI_Type_delete_attr_function", OrderedDict([("datatype", args[0]), ("type_keyval", args[1]), ("attribute_val", args[2]), ("extra_state", args[3]), ]), "4.0")

    @staticmethod
    def mpi_type_dup(*args):
        return MPICall("MPI_Type_dup", OrderedDict([("oldtype", args[0]), ("newtype", args[1]), ]), "2.0")

    @staticmethod
    def mpi_type_dup_fn(*args):
        return MPICall("MPI_TYPE_DUP_FN", OrderedDict([("oldtype", args[0]), ("type_keyval", args[1]), ("extra_state", args[2]), ("attribute_val_in", args[3]), ("attribute_val_out", args[4]), ("flag", args[5]), ]), "4.0")

    @staticmethod
    def mpi_type_f2c(*args):
        return MPICall("MPI_Type_f2c", OrderedDict([("datatype", args[0]), ]), "2.0")

    @staticmethod
    def mpi_type_free(*args):
        return MPICall("MPI_Type_free", OrderedDict([("datatype", args[0]), ]), "1.0")

    @staticmethod
    def mpi_type_free_keyval(*args):
        return MPICall("MPI_Type_free_keyval", OrderedDict([("type_keyval", args[0]), ]), "2.0")

    @staticmethod
    def mpi_type_get_attr(*args):
        return MPICall("MPI_Type_get_attr", OrderedDict([("datatype", args[0]), ("type_keyval", args[1]), ("attribute_val", args[2]), ("flag", args[3]), ]), "2.0")

    @staticmethod
    def mpi_type_get_contents(*args):
        return MPICall("MPI_Type_get_contents", OrderedDict([("datatype", args[0]), ("max_integers", args[1]), ("max_addresses", args[2]), ("max_large_counts", args[3]), ("max_datatypes", args[4]), ("array_of_integers", args[5]), ("array_of_addresses", args[6]), ("array_of_large_counts", args[7]), ("array_of_datatypes", args[8]), ]), "2.0")

    @staticmethod
    def mpi_type_get_envelope(*args):
        return MPICall("MPI_Type_get_envelope", OrderedDict([("datatype", args[0]), ("num_integers", args[1]), ("num_addresses", args[2]), ("num_large_counts", args[3]), ("num_datatypes", args[4]), ("combiner", args[5]), ]), "2.0")

    @staticmethod
    def mpi_type_get_extent(*args):
        return MPICall("MPI_Type_get_extent", OrderedDict([("datatype", args[0]), ("lb", args[1]), ("extent", args[2]), ]), "2.0")

    @staticmethod
    def mpi_type_get_extent_x(*args):
        return MPICall("MPI_Type_get_extent_x", OrderedDict([("datatype", args[0]), ("lb", args[1]), ("extent", args[2]), ]), "3.0")

    @staticmethod
    def mpi_type_get_name(*args):
        return MPICall("MPI_Type_get_name", OrderedDict([("datatype", args[0]), ("type_name", args[1]), ("resultlen", args[2]), ]), "2.0")

    @staticmethod
    def mpi_type_get_true_extent(*args):
        return MPICall("MPI_Type_get_true_extent", OrderedDict([("datatype", args[0]), ("true_lb", args[1]), ("true_extent", args[2]), ]), "2.0")

    @staticmethod
    def mpi_type_get_true_extent_x(*args):
        return MPICall("MPI_Type_get_true_extent_x", OrderedDict([("datatype", args[0]), ("true_lb", args[1]), ("true_extent", args[2]), ]), "3.0")

    @staticmethod
    def mpi_type_indexed(*args):
        return MPICall("MPI_Type_indexed", OrderedDict([("count", args[0]), ("array_of_blocklengths", args[1]), ("array_of_displacements", args[2]), ("oldtype", args[3]), ("newtype", args[4]), ]), "1.0")

    @staticmethod
    def mpi_type_match_size(*args):
        return MPICall("MPI_Type_match_size", OrderedDict([("typeclass", args[0]), ("size", args[1]), ("datatype", args[2]), ]), "2.0")

    @staticmethod
    def mpi_type_null_copy_fn(*args):
        return MPICall("MPI_TYPE_NULL_COPY_FN", OrderedDict([("oldtype", args[0]), ("type_keyval", args[1]), ("extra_state", args[2]), ("attribute_val_in", args[3]), ("attribute_val_out", args[4]), ("flag", args[5]), ]), "4.0")

    @staticmethod
    def mpi_type_null_delete_fn(*args):
        return MPICall("MPI_TYPE_NULL_DELETE_FN", OrderedDict([("datatype", args[0]), ("type_keyval", args[1]), ("attribute_val", args[2]), ("extra_state", args[3]), ]), "4.0")

    @staticmethod
    def mpi_type_set_attr(*args):
        return MPICall("MPI_Type_set_attr", OrderedDict([("datatype", args[0]), ("type_keyval", args[1]), ("attribute_val", args[2]), ]), "2.0")

    @staticmethod
    def mpi_type_set_name(*args):
        return MPICall("MPI_Type_set_name", OrderedDict([("datatype", args[0]), ("type_name", args[1]), ]), "2.0")

    @staticmethod
    def mpi_type_size(*args):
        return MPICall("MPI_Type_size", OrderedDict([("datatype", args[0]), ("size", args[1]), ]), "1.0")

    @staticmethod
    def mpi_type_size_x(*args):
        return MPICall("MPI_Type_size_x", OrderedDict([("datatype", args[0]), ("size", args[1]), ]), "3.0")

    @staticmethod
    def mpi_type_vector(*args):
        return MPICall("MPI_Type_vector", OrderedDict([("count", args[0]), ("blocklength", args[1]), ("stride", args[2]), ("oldtype", args[3]), ("newtype", args[4]), ]), "1.0")

    @staticmethod
    def mpi_unpack(*args):
        return MPICall("MPI_Unpack", OrderedDict([("inbuf", args[0]), ("insize", args[1]), ("position", args[2]), ("outbuf", args[3]), ("outcount", args[4]), ("datatype", args[5]), ("comm", args[6]), ]), "1.0")

    @staticmethod
    def mpi_unpack_external(*args):
        return MPICall("MPI_Unpack_external", OrderedDict([("datarep", args[0]), ("inbuf", args[1]), ("insize", args[2]), ("position", args[3]), ("outbuf", args[4]), ("outcount", args[5]), ("datatype", args[6]), ]), "2.0")

    @staticmethod
    def mpi_unpublish_name(*args):
        return MPICall("MPI_Unpublish_name", OrderedDict([("service_name", args[0]), ("info", args[1]), ("port_name", args[2]), ]), "2.0")

    @staticmethod
    def mpi_user_function(*args):
        return MPICall("MPI_User_function", OrderedDict([("invec", args[0]), ("inoutvec", args[1]), ("len", args[2]), ("datatype", args[3]), ]), "1.0")

    @staticmethod
    def mpi_wait(*args):
        return MPICall("MPI_Wait", OrderedDict([("request", args[0]), ("status", args[1]), ]), "1.0")

    @staticmethod
    def mpi_waitall(*args):
        return MPICall("MPI_Waitall", OrderedDict([("count", args[0]), ("array_of_requests", args[1]), ("array_of_statuses", args[2]), ]), "1.0")

    @staticmethod
    def mpi_waitany(*args):
        return MPICall("MPI_Waitany", OrderedDict([("count", args[0]), ("array_of_requests", args[1]), ("index", args[2]), ("status", args[3]), ]), "1.0")

    @staticmethod
    def mpi_waitsome(*args):
        return MPICall("MPI_Waitsome", OrderedDict([("incount", args[0]), ("array_of_requests", args[1]), ("outcount", args[2]), ("array_of_indices", args[3]), ("array_of_statuses", args[4]), ]), "1.0")

    @staticmethod
    def mpi_win_allocate(*args):
        return MPICall("MPI_Win_allocate", OrderedDict([("size", args[0]), ("disp_unit", args[1]), ("info", args[2]), ("comm", args[3]), ("baseptr", args[4]), ("win", args[5]), ]), "3.0")

    @staticmethod
    def mpi_win_allocate_shared(*args):
        return MPICall("MPI_Win_allocate_shared", OrderedDict([("size", args[0]), ("disp_unit", args[1]), ("info", args[2]), ("comm", args[3]), ("baseptr", args[4]), ("win", args[5]), ]), "3.0")

    @staticmethod
    def mpi_win_attach(*args):
        return MPICall("MPI_Win_attach", OrderedDict([("win", args[0]), ("base", args[1]), ("size", args[2]), ]), "3.0")

    @staticmethod
    def mpi_win_c2f(*args):
        return MPICall("MPI_Win_c2f", OrderedDict([("win", args[0]), ]), "2.0")

    @staticmethod
    def mpi_win_call_errhandler(*args):
        return MPICall("MPI_Win_call_errhandler", OrderedDict([("win", args[0]), ("errorcode", args[1]), ]), "2.0")

    @staticmethod
    def mpi_win_complete(*args):
        return MPICall("MPI_Win_complete", OrderedDict([("win", args[0]), ]), "2.0")

    @staticmethod
    def mpi_win_copy_attr_function(*args):
        return MPICall("MPI_Win_copy_attr_function", OrderedDict([("oldwin", args[0]), ("win_keyval", args[1]), ("extra_state", args[2]), ("attribute_val_in", args[3]), ("attribute_val_out", args[4]), ("flag", args[5]), ]), "4.0")

    @staticmethod
    def mpi_win_create(*args):
        return MPICall("MPI_Win_create", OrderedDict([("base", args[0]), ("size", args[1]), ("disp_unit", args[2]), ("info", args[3]), ("comm", args[4]), ("win", args[5]), ]), "2.0")

    @staticmethod
    def mpi_win_create_dynamic(*args):
        return MPICall("MPI_Win_create_dynamic", OrderedDict([("info", args[0]), ("comm", args[1]), ("win", args[2]), ]), "3.0")

    @staticmethod
    def mpi_win_create_errhandler(*args):
        return MPICall("MPI_Win_create_errhandler", OrderedDict([("win_errhandler_fn", args[0]), ("errhandler", args[1]), ]), "2.0")

    @staticmethod
    def mpi_win_create_keyval(*args):
        return MPICall("MPI_Win_create_keyval", OrderedDict([("win_copy_attr_fn", args[0]), ("win_delete_attr_fn", args[1]), ("win_keyval", args[2]), ("extra_state", args[3]), ]), "2.0")

    @staticmethod
    def mpi_win_delete_attr(*args):
        return MPICall("MPI_Win_delete_attr", OrderedDict([("win", args[0]), ("win_keyval", args[1]), ]), "2.0")

    @staticmethod
    def mpi_win_delete_attr_function(*args):
        return MPICall("MPI_Win_delete_attr_function", OrderedDict([("win", args[0]), ("win_keyval", args[1]), ("attribute_val", args[2]), ("extra_state", args[3]), ]), "4.0")

    @staticmethod
    def mpi_win_detach(*args):
        return MPICall("MPI_Win_detach", OrderedDict([("win", args[0]), ("base", args[1]), ]), "3.0")

    @staticmethod
    def mpi_win_dup_fn(*args):
        return MPICall("MPI_WIN_DUP_FN", OrderedDict([("oldwin", args[0]), ("win_keyval", args[1]), ("extra_state", args[2]), ("attribute_val_in", args[3]), ("attribute_val_out", args[4]), ("flag", args[5]), ]), "4.0")

    @staticmethod
    def mpi_win_errhandler_function(*args):
        return MPICall("MPI_Win_errhandler_function", OrderedDict([("win", args[0]), ("error_code", args[1]), ("varargs", args[2]), ]), "4.0")

    @staticmethod
    def mpi_win_f2c(*args):
        return MPICall("MPI_Win_f2c", OrderedDict([("win", args[0]), ]), "2.0")

    @staticmethod
    def mpi_win_fence(*args):
        return MPICall("MPI_Win_fence", OrderedDict([("assert", args[0]), ("win", args[1]), ]), "2.0")

    @staticmethod
    def mpi_win_flush(*args):
        return MPICall("MPI_Win_flush", OrderedDict([("rank", args[0]), ("win", args[1]), ]), "3.0")

    @staticmethod
    def mpi_win_flush_all(*args):
        return MPICall("MPI_Win_flush_all", OrderedDict([("win", args[0]), ]), "3.0")

    @staticmethod
    def mpi_win_flush_local(*args):
        return MPICall("MPI_Win_flush_local", OrderedDict([("rank", args[0]), ("win", args[1]), ]), "3.0")

    @staticmethod
    def mpi_win_flush_local_all(*args):
        return MPICall("MPI_Win_flush_local_all", OrderedDict([("win", args[0]), ]), "3.0")

    @staticmethod
    def mpi_win_free(*args):
        return MPICall("MPI_Win_free", OrderedDict([("win", args[0]), ]), "2.0")

    @staticmethod
    def mpi_win_free_keyval(*args):
        return MPICall("MPI_Win_free_keyval", OrderedDict([("win_keyval", args[0]), ]), "2.0")

    @staticmethod
    def mpi_win_get_attr(*args):
        return MPICall("MPI_Win_get_attr", OrderedDict([("win", args[0]), ("win_keyval", args[1]), ("attribute_val", args[2]), ("flag", args[3]), ]), "2.0")

    @staticmethod
    def mpi_win_get_errhandler(*args):
        return MPICall("MPI_Win_get_errhandler", OrderedDict([("win", args[0]), ("errhandler", args[1]), ]), "2.0")

    @staticmethod
    def mpi_win_get_group(*args):
        return MPICall("MPI_Win_get_group", OrderedDict([("win", args[0]), ("group", args[1]), ]), "2.0")

    @staticmethod
    def mpi_win_get_info(*args):
        return MPICall("MPI_Win_get_info", OrderedDict([("win", args[0]), ("info_used", args[1]), ]), "3.0")

    @staticmethod
    def mpi_win_get_name(*args):
        return MPICall("MPI_Win_get_name", OrderedDict([("win", args[0]), ("win_name", args[1]), ("resultlen", args[2]), ]), "2.0")

    @staticmethod
    def mpi_win_lock(*args):
        return MPICall("MPI_Win_lock", OrderedDict([("lock_type", args[0]), ("rank", args[1]), ("assert", args[2]), ("win", args[3]), ]), "2.0")

    @staticmethod
    def mpi_win_lock_all(*args):
        return MPICall("MPI_Win_lock_all", OrderedDict([("assert", args[0]), ("win", args[1]), ]), "3.0")

    @staticmethod
    def mpi_win_null_copy_fn(*args):
        return MPICall("MPI_WIN_NULL_COPY_FN", OrderedDict([("oldwin", args[0]), ("win_keyval", args[1]), ("extra_state", args[2]), ("attribute_val_in", args[3]), ("attribute_val_out", args[4]), ("flag", args[5]), ]), "4.0")

    @staticmethod
    def mpi_win_null_delete_fn(*args):
        return MPICall("MPI_WIN_NULL_DELETE_FN", OrderedDict([("win", args[0]), ("win_keyval", args[1]), ("attribute_val", args[2]), ("extra_state", args[3]), ]), "4.0")

    @staticmethod
    def mpi_win_post(*args):
        return MPICall("MPI_Win_post", OrderedDict([("group", args[0]), ("assert", args[1]), ("win", args[2]), ]), "2.0")

    @staticmethod
    def mpi_win_set_attr(*args):
        return MPICall("MPI_Win_set_attr", OrderedDict([("win", args[0]), ("win_keyval", args[1]), ("attribute_val", args[2]), ]), "2.0")

    @staticmethod
    def mpi_win_set_errhandler(*args):
        return MPICall("MPI_Win_set_errhandler", OrderedDict([("win", args[0]), ("errhandler", args[1]), ]), "2.0")

    @staticmethod
    def mpi_win_set_info(*args):
        return MPICall("MPI_Win_set_info", OrderedDict([("win", args[0]), ("info", args[1]), ]), "3.0")

    @staticmethod
    def mpi_win_set_name(*args):
        return MPICall("MPI_Win_set_name", OrderedDict([("win", args[0]), ("win_name", args[1]), ]), "2.0")

    @staticmethod
    def mpi_win_shared_query(*args):
        return MPICall("MPI_Win_shared_query", OrderedDict([("win", args[0]), ("rank", args[1]), ("size", args[2]), ("disp_unit", args[3]), ("baseptr", args[4]), ]), "3.0")

    @staticmethod
    def mpi_win_start(*args):
        return MPICall("MPI_Win_start", OrderedDict([("group", args[0]), ("assert", args[1]), ("win", args[2]), ]), "2.0")

    @staticmethod
    def mpi_win_sync(*args):
        return MPICall("MPI_Win_sync", OrderedDict([("win", args[0]), ]), "3.0")

    @staticmethod
    def mpi_win_test(*args):
        return MPICall("MPI_Win_test", OrderedDict([("win", args[0]), ("flag", args[1]), ]), "2.0")

    @staticmethod
    def mpi_win_unlock(*args):
        return MPICall("MPI_Win_unlock", OrderedDict([("rank", args[0]), ("win", args[1]), ]), "2.0")

    @staticmethod
    def mpi_win_unlock_all(*args):
        return MPICall("MPI_Win_unlock_all", OrderedDict([("win", args[0]), ]), "3.0")

    @staticmethod
    def mpi_win_wait(*args):
        return MPICall("MPI_Win_wait", OrderedDict([("win", args[0]), ]), "2.0")

    @staticmethod
    def mpi_wtick(*args):
        return MPICall("MPI_Wtick", OrderedDict([]), "1.0")

    @staticmethod
    def mpi_wtime(*args):
        return MPICall("MPI_Wtime", OrderedDict([]), "1.0")


from Infrastructure.CorrectParameter import CorrectParameterFactory

class CorrectMPICallFactory:

    @staticmethod
    def get(func: str):
        f_to_call = getattr(CorrectMPICallFactory, func)
        return f_to_call()


    @staticmethod
    def mpi_abort():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_abort(correct_params.get("comm"),correct_params.get("errorcode"))

    @staticmethod
    def mpi_accumulate():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_accumulate(correct_params.get("origin_addr"),correct_params.get("origin_count"),correct_params.get("origin_datatype"),correct_params.get("target_rank"),correct_params.get("target_disp"),correct_params.get("target_count"),correct_params.get("target_datatype"),correct_params.get("op"),correct_params.get("win"))

    @staticmethod
    def mpi_add_error_class():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_add_error_class(correct_params.get("errorclass"))

    @staticmethod
    def mpi_add_error_code():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_add_error_code(correct_params.get("errorclass"),correct_params.get("errorcode"))

    @staticmethod
    def mpi_add_error_string():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_add_error_string(correct_params.get("errorcode"),correct_params.get("string"))

    @staticmethod
    def mpi_aint_add():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_aint_add(correct_params.get("base"),correct_params.get("disp"))

    @staticmethod
    def mpi_aint_diff():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_aint_diff(correct_params.get("addr1"),correct_params.get("addr2"))

    @staticmethod
    def mpi_allgather():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_allgather(correct_params.get("sendbuf"),correct_params.get("sendcount"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcount"),correct_params.get("recvtype"),correct_params.get("comm"))

    @staticmethod
    def mpi_allgather_init():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_allgather_init(correct_params.get("sendbuf"),correct_params.get("sendcount"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcount"),correct_params.get("recvtype"),correct_params.get("comm"),correct_params.get("info"),correct_params.get("request"))

    @staticmethod
    def mpi_allgatherv():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_allgatherv(correct_params.get("sendbuf"),correct_params.get("sendcount"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcounts"),correct_params.get("displs"),correct_params.get("recvtype"),correct_params.get("comm"))

    @staticmethod
    def mpi_allgatherv_init():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_allgatherv_init(correct_params.get("sendbuf"),correct_params.get("sendcount"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcounts"),correct_params.get("displs"),correct_params.get("recvtype"),correct_params.get("comm"),correct_params.get("info"),correct_params.get("request"))

    @staticmethod
    def mpi_alloc_mem():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_alloc_mem(correct_params.get("size"),correct_params.get("info"),correct_params.get("baseptr"))

    @staticmethod
    def mpi_allreduce():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_allreduce(correct_params.get("sendbuf"),correct_params.get("recvbuf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("op"),correct_params.get("comm"))

    @staticmethod
    def mpi_allreduce_init():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_allreduce_init(correct_params.get("sendbuf"),correct_params.get("recvbuf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("op"),correct_params.get("comm"),correct_params.get("info"),correct_params.get("request"))

    @staticmethod
    def mpi_alltoall():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_alltoall(correct_params.get("sendbuf"),correct_params.get("sendcount"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcount"),correct_params.get("recvtype"),correct_params.get("comm"))

    @staticmethod
    def mpi_alltoall_init():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_alltoall_init(correct_params.get("sendbuf"),correct_params.get("sendcount"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcount"),correct_params.get("recvtype"),correct_params.get("comm"),correct_params.get("info"),correct_params.get("request"))

    @staticmethod
    def mpi_alltoallv():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_alltoallv(correct_params.get("sendbuf"),correct_params.get("sendcounts"),correct_params.get("sdispls"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcounts"),correct_params.get("rdispls"),correct_params.get("recvtype"),correct_params.get("comm"))

    @staticmethod
    def mpi_alltoallv_init():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_alltoallv_init(correct_params.get("sendbuf"),correct_params.get("sendcounts"),correct_params.get("sdispls"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcounts"),correct_params.get("rdispls"),correct_params.get("recvtype"),correct_params.get("comm"),correct_params.get("info"),correct_params.get("request"))

    @staticmethod
    def mpi_alltoallw():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_alltoallw(correct_params.get("sendbuf"),correct_params.get("sendcounts"),correct_params.get("sdispls"),correct_params.get("sendtypes"),correct_params.get("recvbuf"),correct_params.get("recvcounts"),correct_params.get("rdispls"),correct_params.get("recvtypes"),correct_params.get("comm"))

    @staticmethod
    def mpi_alltoallw_init():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_alltoallw_init(correct_params.get("sendbuf"),correct_params.get("sendcounts"),correct_params.get("sdispls"),correct_params.get("sendtypes"),correct_params.get("recvbuf"),correct_params.get("recvcounts"),correct_params.get("rdispls"),correct_params.get("recvtypes"),correct_params.get("comm"),correct_params.get("info"),correct_params.get("request"))

    @staticmethod
    def mpi_attr_delete():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_attr_delete(correct_params.get("comm"),correct_params.get("keyval"))

    @staticmethod
    def mpi_attr_get():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_attr_get(correct_params.get("comm"),correct_params.get("keyval"),correct_params.get("attribute_val"),correct_params.get("flag"))

    @staticmethod
    def mpi_attr_put():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_attr_put(correct_params.get("comm"),correct_params.get("keyval"),correct_params.get("attribute_val"))

    @staticmethod
    def mpi_barrier():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_barrier(correct_params.get("comm"))

    @staticmethod
    def mpi_barrier_init():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_barrier_init(correct_params.get("comm"),correct_params.get("info"),correct_params.get("request"))

    @staticmethod
    def mpi_bcast():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_bcast(correct_params.get("buffer"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("root"),correct_params.get("comm"))

    @staticmethod
    def mpi_bcast_init():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_bcast_init(correct_params.get("buffer"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("root"),correct_params.get("comm"),correct_params.get("info"),correct_params.get("request"))

    @staticmethod
    def mpi_bsend():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_bsend(correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("dest"),correct_params.get("tag"),correct_params.get("comm"))

    @staticmethod
    def mpi_bsend_init():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_bsend_init(correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("dest"),correct_params.get("tag"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_buffer_attach():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_buffer_attach(correct_params.get("buffer"),correct_params.get("size"))

    @staticmethod
    def mpi_buffer_detach():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_buffer_detach(correct_params.get("buffer_addr"),correct_params.get("size"))

    @staticmethod
    def mpi_cancel():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_cancel(correct_params.get("request"))

    @staticmethod
    def mpi_cart_coords():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_cart_coords(correct_params.get("comm"),correct_params.get("rank"),correct_params.get("maxdims"),correct_params.get("coords"))

    @staticmethod
    def mpi_cart_create():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_cart_create(correct_params.get("comm_old"),correct_params.get("ndims"),correct_params.get("dims"),correct_params.get("periods"),correct_params.get("reorder"),correct_params.get("comm_cart"))

    @staticmethod
    def mpi_cart_get():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_cart_get(correct_params.get("comm_cart"),correct_params.get("maxdims"),correct_params.get("dims"),correct_params.get("periods"),correct_params.get("coords"))

    @staticmethod
    def mpi_cart_map():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_cart_map(correct_params.get("comm"),correct_params.get("ndims"),correct_params.get("dims"),correct_params.get("periods"),correct_params.get("newrank"))

    @staticmethod
    def mpi_cart_rank():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_cart_rank(correct_params.get("comm"),correct_params.get("coords"),correct_params.get("rank"))

    @staticmethod
    def mpi_cart_shift():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_cart_shift(correct_params.get("comm"),correct_params.get("direction"),correct_params.get("disp"),correct_params.get("rank_source"),correct_params.get("rank_dest"))

    @staticmethod
    def mpi_cart_sub():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_cart_sub(correct_params.get("comm"),correct_params.get("remain_dims"),correct_params.get("newcomm"))

    @staticmethod
    def mpi_cartdim_get():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_cartdim_get(correct_params.get("comm"),correct_params.get("ndims"))

    @staticmethod
    def mpi_close_port():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_close_port(correct_params.get("port_name"))

    @staticmethod
    def mpi_comm_accept():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_accept(correct_params.get("port_name"),correct_params.get("info"),correct_params.get("root"),correct_params.get("comm"),correct_params.get("newcomm"))

    @staticmethod
    def mpi_comm_c2f():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_c2f(correct_params.get("comm"))

    @staticmethod
    def mpi_comm_call_errhandler():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_call_errhandler(correct_params.get("comm"),correct_params.get("errorcode"))

    @staticmethod
    def mpi_comm_compare():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_compare(correct_params.get("comm1"),correct_params.get("comm2"),correct_params.get("result"))

    @staticmethod
    def mpi_comm_connect():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_connect(correct_params.get("port_name"),correct_params.get("info"),correct_params.get("root"),correct_params.get("comm"),correct_params.get("newcomm"))

    @staticmethod
    def mpi_comm_copy_attr_function():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_copy_attr_function(correct_params.get("oldcomm"),correct_params.get("comm_keyval"),correct_params.get("extra_state"),correct_params.get("attribute_val_in"),correct_params.get("attribute_val_out"),correct_params.get("flag"))

    @staticmethod
    def mpi_comm_create():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_create(correct_params.get("comm"),correct_params.get("group"),correct_params.get("newcomm"))

    @staticmethod
    def mpi_comm_create_errhandler():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_create_errhandler(correct_params.get("comm_errhandler_fn"),correct_params.get("errhandler"))

    @staticmethod
    def mpi_comm_create_from_group():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_create_from_group(correct_params.get("group"),correct_params.get("stringtag"),correct_params.get("info"),correct_params.get("errhandler"),correct_params.get("newcomm"))

    @staticmethod
    def mpi_comm_create_group():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_create_group(correct_params.get("comm"),correct_params.get("group"),correct_params.get("tag"),correct_params.get("newcomm"))

    @staticmethod
    def mpi_comm_create_keyval():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_create_keyval(correct_params.get("comm_copy_attr_fn"),correct_params.get("comm_delete_attr_fn"),correct_params.get("comm_keyval"),correct_params.get("extra_state"))

    @staticmethod
    def mpi_comm_delete_attr():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_delete_attr(correct_params.get("comm"),correct_params.get("comm_keyval"))

    @staticmethod
    def mpi_comm_delete_attr_function():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_delete_attr_function(correct_params.get("comm"),correct_params.get("comm_keyval"),correct_params.get("attribute_val"),correct_params.get("extra_state"))

    @staticmethod
    def mpi_comm_disconnect():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_disconnect(correct_params.get("comm"))

    @staticmethod
    def mpi_comm_dup():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_dup(correct_params.get("comm"),correct_params.get("newcomm"))

    @staticmethod
    def mpi_comm_dup_fn():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_dup_fn(correct_params.get("oldcomm"),correct_params.get("comm_keyval"),correct_params.get("extra_state"),correct_params.get("attribute_val_in"),correct_params.get("attribute_val_out"),correct_params.get("flag"))

    @staticmethod
    def mpi_comm_dup_with_info():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_dup_with_info(correct_params.get("comm"),correct_params.get("info"),correct_params.get("newcomm"))

    @staticmethod
    def mpi_comm_errhandler_function():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_errhandler_function(correct_params.get("comm"),correct_params.get("error_code"),correct_params.get("varargs"))

    @staticmethod
    def mpi_comm_f2c():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_f2c(correct_params.get("comm"))

    @staticmethod
    def mpi_comm_free():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_free(correct_params.get("comm"))

    @staticmethod
    def mpi_comm_free_keyval():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_free_keyval(correct_params.get("comm_keyval"))

    @staticmethod
    def mpi_comm_get_attr():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_get_attr(correct_params.get("comm"),correct_params.get("comm_keyval"),correct_params.get("attribute_val"),correct_params.get("flag"))

    @staticmethod
    def mpi_comm_get_errhandler():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_get_errhandler(correct_params.get("comm"),correct_params.get("errhandler"))

    @staticmethod
    def mpi_comm_get_info():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_get_info(correct_params.get("comm"),correct_params.get("info_used"))

    @staticmethod
    def mpi_comm_get_name():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_get_name(correct_params.get("comm"),correct_params.get("comm_name"),correct_params.get("resultlen"))

    @staticmethod
    def mpi_comm_get_parent():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_get_parent(correct_params.get("parent"))

    @staticmethod
    def mpi_comm_group():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_group(correct_params.get("comm"),correct_params.get("group"))

    @staticmethod
    def mpi_comm_idup():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_idup(correct_params.get("comm"),correct_params.get("newcomm"),correct_params.get("request"))

    @staticmethod
    def mpi_comm_idup_with_info():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_idup_with_info(correct_params.get("comm"),correct_params.get("info"),correct_params.get("newcomm"),correct_params.get("request"))

    @staticmethod
    def mpi_comm_join():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_join(correct_params.get("fd"),correct_params.get("intercomm"))

    @staticmethod
    def mpi_comm_null_copy_fn():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_null_copy_fn(correct_params.get("oldcomm"),correct_params.get("comm_keyval"),correct_params.get("extra_state"),correct_params.get("attribute_val_in"),correct_params.get("attribute_val_out"),correct_params.get("flag"))

    @staticmethod
    def mpi_comm_null_delete_fn():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_null_delete_fn(correct_params.get("comm"),correct_params.get("comm_keyval"),correct_params.get("attribute_val"),correct_params.get("extra_state"))

    @staticmethod
    def mpi_comm_rank():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_rank(correct_params.get("comm"),correct_params.get("rank"))

    @staticmethod
    def mpi_comm_remote_group():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_remote_group(correct_params.get("comm"),correct_params.get("group"))

    @staticmethod
    def mpi_comm_remote_size():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_remote_size(correct_params.get("comm"),correct_params.get("size"))

    @staticmethod
    def mpi_comm_set_attr():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_set_attr(correct_params.get("comm"),correct_params.get("comm_keyval"),correct_params.get("attribute_val"))

    @staticmethod
    def mpi_comm_set_errhandler():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_set_errhandler(correct_params.get("comm"),correct_params.get("errhandler"))

    @staticmethod
    def mpi_comm_set_info():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_set_info(correct_params.get("comm"),correct_params.get("info"))

    @staticmethod
    def mpi_comm_set_name():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_set_name(correct_params.get("comm"),correct_params.get("comm_name"))

    @staticmethod
    def mpi_comm_size():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_size(correct_params.get("comm"),correct_params.get("size"))

    @staticmethod
    def mpi_comm_spawn():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_spawn(correct_params.get("command"),correct_params.get("argv"),correct_params.get("maxprocs"),correct_params.get("info"),correct_params.get("root"),correct_params.get("comm"),correct_params.get("intercomm"),correct_params.get("array_of_errcodes"))

    @staticmethod
    def mpi_comm_spawn_multiple():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_spawn_multiple(correct_params.get("count"),correct_params.get("array_of_commands"),correct_params.get("array_of_argv"),correct_params.get("array_of_maxprocs"),correct_params.get("array_of_info"),correct_params.get("root"),correct_params.get("comm"),correct_params.get("intercomm"),correct_params.get("array_of_errcodes"))

    @staticmethod
    def mpi_comm_split():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_split(correct_params.get("comm"),correct_params.get("color"),correct_params.get("key"),correct_params.get("newcomm"))

    @staticmethod
    def mpi_comm_split_type():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_split_type(correct_params.get("comm"),correct_params.get("split_type"),correct_params.get("key"),correct_params.get("info"),correct_params.get("newcomm"))

    @staticmethod
    def mpi_comm_test_inter():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_comm_test_inter(correct_params.get("comm"),correct_params.get("flag"))

    @staticmethod
    def mpi_compare_and_swap():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_compare_and_swap(correct_params.get("origin_addr"),correct_params.get("compare_addr"),correct_params.get("result_addr"),correct_params.get("datatype"),correct_params.get("target_rank"),correct_params.get("target_disp"),correct_params.get("win"))

    @staticmethod
    def mpi_conversion_fn_null():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_conversion_fn_null(correct_params.get("userbuf"),correct_params.get("datatype"),correct_params.get("count"),correct_params.get("filebuf"),correct_params.get("position"),correct_params.get("extra_state"))

    @staticmethod
    def mpi_copy_function():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_copy_function(correct_params.get("oldcomm"),correct_params.get("keyval"),correct_params.get("extra_state"),correct_params.get("attribute_val_in"),correct_params.get("attribute_val_out"),correct_params.get("flag"))

    @staticmethod
    def mpi_datarep_conversion_function():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_datarep_conversion_function(correct_params.get("userbuf"),correct_params.get("datatype"),correct_params.get("count"),correct_params.get("filebuf"),correct_params.get("position"),correct_params.get("extra_state"))

    @staticmethod
    def mpi_datarep_extent_function():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_datarep_extent_function(correct_params.get("datatype"),correct_params.get("extent"),correct_params.get("extra_state"))

    @staticmethod
    def mpi_delete_function():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_delete_function(correct_params.get("comm"),correct_params.get("keyval"),correct_params.get("attribute_val"),correct_params.get("extra_state"))

    @staticmethod
    def mpi_dims_create():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_dims_create(correct_params.get("nnodes"),correct_params.get("ndims"),correct_params.get("dims"))

    @staticmethod
    def mpi_dist_graph_create():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_dist_graph_create(correct_params.get("comm_old"),correct_params.get("n"),correct_params.get("sources"),correct_params.get("degrees"),correct_params.get("destinations"),correct_params.get("weights"),correct_params.get("info"),correct_params.get("reorder"),correct_params.get("comm_dist_graph"))

    @staticmethod
    def mpi_dist_graph_create_adjacent():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_dist_graph_create_adjacent(correct_params.get("comm_old"),correct_params.get("indegree"),correct_params.get("sources"),correct_params.get("sourceweights"),correct_params.get("outdegree"),correct_params.get("destinations"),correct_params.get("destweights"),correct_params.get("info"),correct_params.get("reorder"),correct_params.get("comm_dist_graph"))

    @staticmethod
    def mpi_dist_graph_neighbors():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_dist_graph_neighbors(correct_params.get("comm"),correct_params.get("maxindegree"),correct_params.get("sources"),correct_params.get("sourceweights"),correct_params.get("maxoutdegree"),correct_params.get("destinations"),correct_params.get("destweights"))

    @staticmethod
    def mpi_dist_graph_neighbors_count():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_dist_graph_neighbors_count(correct_params.get("comm"),correct_params.get("indegree"),correct_params.get("outdegree"),correct_params.get("weighted"))

    @staticmethod
    def mpi_dup_fn():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_dup_fn(correct_params.get("oldcomm"),correct_params.get("keyval"),correct_params.get("extra_state"),correct_params.get("attribute_val_in"),correct_params.get("attribute_val_out"),correct_params.get("flag"))

    @staticmethod
    def mpi_errhandler_c2f():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_errhandler_c2f(correct_params.get("errhandler"))

    @staticmethod
    def mpi_errhandler_f2c():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_errhandler_f2c(correct_params.get("errhandler"))

    @staticmethod
    def mpi_errhandler_free():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_errhandler_free(correct_params.get("errhandler"))

    @staticmethod
    def mpi_error_class():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_error_class(correct_params.get("errorcode"),correct_params.get("errorclass"))

    @staticmethod
    def mpi_error_string():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_error_string(correct_params.get("errorcode"),correct_params.get("string"),correct_params.get("resultlen"))

    @staticmethod
    def mpi_exscan():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_exscan(correct_params.get("sendbuf"),correct_params.get("recvbuf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("op"),correct_params.get("comm"))

    @staticmethod
    def mpi_exscan_init():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_exscan_init(correct_params.get("sendbuf"),correct_params.get("recvbuf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("op"),correct_params.get("comm"),correct_params.get("info"),correct_params.get("request"))

    @staticmethod
    def mpi_f_sync_reg():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_f_sync_reg(correct_params.get("buf"))

    @staticmethod
    def mpi_fetch_and_op():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_fetch_and_op(correct_params.get("origin_addr"),correct_params.get("result_addr"),correct_params.get("datatype"),correct_params.get("target_rank"),correct_params.get("target_disp"),correct_params.get("op"),correct_params.get("win"))

    @staticmethod
    def mpi_file_c2f():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_c2f(correct_params.get("file"))

    @staticmethod
    def mpi_file_call_errhandler():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_call_errhandler(correct_params.get("fh"),correct_params.get("errorcode"))

    @staticmethod
    def mpi_file_close():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_close(correct_params.get("fh"))

    @staticmethod
    def mpi_file_create_errhandler():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_create_errhandler(correct_params.get("file_errhandler_fn"),correct_params.get("errhandler"))

    @staticmethod
    def mpi_file_delete():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_delete(correct_params.get("filename"),correct_params.get("info"))

    @staticmethod
    def mpi_file_errhandler_function():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_errhandler_function(correct_params.get("file"),correct_params.get("error_code"),correct_params.get("varargs"))

    @staticmethod
    def mpi_file_f2c():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_f2c(correct_params.get("file"))

    @staticmethod
    def mpi_file_get_amode():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_get_amode(correct_params.get("fh"),correct_params.get("amode"))

    @staticmethod
    def mpi_file_get_atomicity():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_get_atomicity(correct_params.get("fh"),correct_params.get("flag"))

    @staticmethod
    def mpi_file_get_byte_offset():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_get_byte_offset(correct_params.get("fh"),correct_params.get("offset"),correct_params.get("disp"))

    @staticmethod
    def mpi_file_get_errhandler():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_get_errhandler(correct_params.get("file"),correct_params.get("errhandler"))

    @staticmethod
    def mpi_file_get_group():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_get_group(correct_params.get("fh"),correct_params.get("group"))

    @staticmethod
    def mpi_file_get_info():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_get_info(correct_params.get("fh"),correct_params.get("info_used"))

    @staticmethod
    def mpi_file_get_position():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_get_position(correct_params.get("fh"),correct_params.get("offset"))

    @staticmethod
    def mpi_file_get_position_shared():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_get_position_shared(correct_params.get("fh"),correct_params.get("offset"))

    @staticmethod
    def mpi_file_get_size():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_get_size(correct_params.get("fh"),correct_params.get("size"))

    @staticmethod
    def mpi_file_get_type_extent():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_get_type_extent(correct_params.get("fh"),correct_params.get("datatype"),correct_params.get("extent"))

    @staticmethod
    def mpi_file_get_view():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_get_view(correct_params.get("fh"),correct_params.get("disp"),correct_params.get("etype"),correct_params.get("filetype"),correct_params.get("datarep"))

    @staticmethod
    def mpi_file_iread():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_iread(correct_params.get("fh"),correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("request"))

    @staticmethod
    def mpi_file_iread_all():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_iread_all(correct_params.get("fh"),correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("request"))

    @staticmethod
    def mpi_file_iread_at():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_iread_at(correct_params.get("fh"),correct_params.get("offset"),correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("request"))

    @staticmethod
    def mpi_file_iread_at_all():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_iread_at_all(correct_params.get("fh"),correct_params.get("offset"),correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("request"))

    @staticmethod
    def mpi_file_iread_shared():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_iread_shared(correct_params.get("fh"),correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("request"))

    @staticmethod
    def mpi_file_iwrite():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_iwrite(correct_params.get("fh"),correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("request"))

    @staticmethod
    def mpi_file_iwrite_all():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_iwrite_all(correct_params.get("fh"),correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("request"))

    @staticmethod
    def mpi_file_iwrite_at():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_iwrite_at(correct_params.get("fh"),correct_params.get("offset"),correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("request"))

    @staticmethod
    def mpi_file_iwrite_at_all():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_iwrite_at_all(correct_params.get("fh"),correct_params.get("offset"),correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("request"))

    @staticmethod
    def mpi_file_iwrite_shared():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_iwrite_shared(correct_params.get("fh"),correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("request"))

    @staticmethod
    def mpi_file_open():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_open(correct_params.get("comm"),correct_params.get("filename"),correct_params.get("amode"),correct_params.get("info"),correct_params.get("fh"))

    @staticmethod
    def mpi_file_preallocate():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_preallocate(correct_params.get("fh"),correct_params.get("size"))

    @staticmethod
    def mpi_file_read():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_read(correct_params.get("fh"),correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("status"))

    @staticmethod
    def mpi_file_read_all():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_read_all(correct_params.get("fh"),correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("status"))

    @staticmethod
    def mpi_file_read_all_begin():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_read_all_begin(correct_params.get("fh"),correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"))

    @staticmethod
    def mpi_file_read_all_end():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_read_all_end(correct_params.get("fh"),correct_params.get("buf"),correct_params.get("status"))

    @staticmethod
    def mpi_file_read_at():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_read_at(correct_params.get("fh"),correct_params.get("offset"),correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("status"))

    @staticmethod
    def mpi_file_read_at_all():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_read_at_all(correct_params.get("fh"),correct_params.get("offset"),correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("status"))

    @staticmethod
    def mpi_file_read_at_all_begin():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_read_at_all_begin(correct_params.get("fh"),correct_params.get("offset"),correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"))

    @staticmethod
    def mpi_file_read_at_all_end():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_read_at_all_end(correct_params.get("fh"),correct_params.get("buf"),correct_params.get("status"))

    @staticmethod
    def mpi_file_read_ordered():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_read_ordered(correct_params.get("fh"),correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("status"))

    @staticmethod
    def mpi_file_read_ordered_begin():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_read_ordered_begin(correct_params.get("fh"),correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"))

    @staticmethod
    def mpi_file_read_ordered_end():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_read_ordered_end(correct_params.get("fh"),correct_params.get("buf"),correct_params.get("status"))

    @staticmethod
    def mpi_file_read_shared():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_read_shared(correct_params.get("fh"),correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("status"))

    @staticmethod
    def mpi_file_seek():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_seek(correct_params.get("fh"),correct_params.get("offset"),correct_params.get("whence"))

    @staticmethod
    def mpi_file_seek_shared():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_seek_shared(correct_params.get("fh"),correct_params.get("offset"),correct_params.get("whence"))

    @staticmethod
    def mpi_file_set_atomicity():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_set_atomicity(correct_params.get("fh"),correct_params.get("flag"))

    @staticmethod
    def mpi_file_set_errhandler():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_set_errhandler(correct_params.get("file"),correct_params.get("errhandler"))

    @staticmethod
    def mpi_file_set_info():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_set_info(correct_params.get("fh"),correct_params.get("info"))

    @staticmethod
    def mpi_file_set_size():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_set_size(correct_params.get("fh"),correct_params.get("size"))

    @staticmethod
    def mpi_file_set_view():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_set_view(correct_params.get("fh"),correct_params.get("disp"),correct_params.get("etype"),correct_params.get("filetype"),correct_params.get("datarep"),correct_params.get("info"))

    @staticmethod
    def mpi_file_sync():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_sync(correct_params.get("fh"))

    @staticmethod
    def mpi_file_write():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_write(correct_params.get("fh"),correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("status"))

    @staticmethod
    def mpi_file_write_all():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_write_all(correct_params.get("fh"),correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("status"))

    @staticmethod
    def mpi_file_write_all_begin():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_write_all_begin(correct_params.get("fh"),correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"))

    @staticmethod
    def mpi_file_write_all_end():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_write_all_end(correct_params.get("fh"),correct_params.get("buf"),correct_params.get("status"))

    @staticmethod
    def mpi_file_write_at():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_write_at(correct_params.get("fh"),correct_params.get("offset"),correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("status"))

    @staticmethod
    def mpi_file_write_at_all():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_write_at_all(correct_params.get("fh"),correct_params.get("offset"),correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("status"))

    @staticmethod
    def mpi_file_write_at_all_begin():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_write_at_all_begin(correct_params.get("fh"),correct_params.get("offset"),correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"))

    @staticmethod
    def mpi_file_write_at_all_end():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_write_at_all_end(correct_params.get("fh"),correct_params.get("buf"),correct_params.get("status"))

    @staticmethod
    def mpi_file_write_ordered():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_write_ordered(correct_params.get("fh"),correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("status"))

    @staticmethod
    def mpi_file_write_ordered_begin():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_write_ordered_begin(correct_params.get("fh"),correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"))

    @staticmethod
    def mpi_file_write_ordered_end():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_write_ordered_end(correct_params.get("fh"),correct_params.get("buf"),correct_params.get("status"))

    @staticmethod
    def mpi_file_write_shared():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_file_write_shared(correct_params.get("fh"),correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("status"))

    @staticmethod
    def mpi_finalize():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_finalize()

    @staticmethod
    def mpi_finalized():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_finalized(correct_params.get("flag"))

    @staticmethod
    def mpi_free_mem():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_free_mem(correct_params.get("base"))

    @staticmethod
    def mpi_gather():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_gather(correct_params.get("sendbuf"),correct_params.get("sendcount"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcount"),correct_params.get("recvtype"),correct_params.get("root"),correct_params.get("comm"))

    @staticmethod
    def mpi_gather_init():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_gather_init(correct_params.get("sendbuf"),correct_params.get("sendcount"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcount"),correct_params.get("recvtype"),correct_params.get("root"),correct_params.get("comm"),correct_params.get("info"),correct_params.get("request"))

    @staticmethod
    def mpi_gatherv():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_gatherv(correct_params.get("sendbuf"),correct_params.get("sendcount"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcounts"),correct_params.get("displs"),correct_params.get("recvtype"),correct_params.get("root"),correct_params.get("comm"))

    @staticmethod
    def mpi_gatherv_init():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_gatherv_init(correct_params.get("sendbuf"),correct_params.get("sendcount"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcounts"),correct_params.get("displs"),correct_params.get("recvtype"),correct_params.get("root"),correct_params.get("comm"),correct_params.get("info"),correct_params.get("request"))

    @staticmethod
    def mpi_get():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_get(correct_params.get("origin_addr"),correct_params.get("origin_count"),correct_params.get("origin_datatype"),correct_params.get("target_rank"),correct_params.get("target_disp"),correct_params.get("target_count"),correct_params.get("target_datatype"),correct_params.get("win"))

    @staticmethod
    def mpi_get_accumulate():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_get_accumulate(correct_params.get("origin_addr"),correct_params.get("origin_count"),correct_params.get("origin_datatype"),correct_params.get("result_addr"),correct_params.get("result_count"),correct_params.get("result_datatype"),correct_params.get("target_rank"),correct_params.get("target_disp"),correct_params.get("target_count"),correct_params.get("target_datatype"),correct_params.get("op"),correct_params.get("win"))

    @staticmethod
    def mpi_get_address():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_get_address(correct_params.get("location"),correct_params.get("address"))

    @staticmethod
    def mpi_get_count():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_get_count(correct_params.get("status"),correct_params.get("datatype"),correct_params.get("count"))

    @staticmethod
    def mpi_get_elements():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_get_elements(correct_params.get("status"),correct_params.get("datatype"),correct_params.get("count"))

    @staticmethod
    def mpi_get_elements_x():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_get_elements_x(correct_params.get("status"),correct_params.get("datatype"),correct_params.get("count"))

    @staticmethod
    def mpi_get_library_version():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_get_library_version(correct_params.get("version"),correct_params.get("resultlen"))

    @staticmethod
    def mpi_get_processor_name():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_get_processor_name(correct_params.get("name"),correct_params.get("resultlen"))

    @staticmethod
    def mpi_get_version():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_get_version(correct_params.get("version"),correct_params.get("subversion"))

    @staticmethod
    def mpi_graph_create():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_graph_create(correct_params.get("comm_old"),correct_params.get("nnodes"),correct_params.get("index"),correct_params.get("edges"),correct_params.get("reorder"),correct_params.get("comm_graph"))

    @staticmethod
    def mpi_graph_get():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_graph_get(correct_params.get("comm"),correct_params.get("maxindex"),correct_params.get("maxedges"),correct_params.get("index"),correct_params.get("edges"))

    @staticmethod
    def mpi_graph_map():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_graph_map(correct_params.get("comm"),correct_params.get("nnodes"),correct_params.get("index"),correct_params.get("edges"),correct_params.get("newrank"))

    @staticmethod
    def mpi_graph_neighbors():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_graph_neighbors(correct_params.get("comm"),correct_params.get("rank"),correct_params.get("maxneighbors"),correct_params.get("neighbors"))

    @staticmethod
    def mpi_graph_neighbors_count():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_graph_neighbors_count(correct_params.get("comm"),correct_params.get("rank"),correct_params.get("nneighbors"))

    @staticmethod
    def mpi_graphdims_get():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_graphdims_get(correct_params.get("comm"),correct_params.get("nnodes"),correct_params.get("nedges"))

    @staticmethod
    def mpi_grequest_cancel_function():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_grequest_cancel_function(correct_params.get("extra_state"),correct_params.get("complete"))

    @staticmethod
    def mpi_grequest_complete():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_grequest_complete(correct_params.get("request"))

    @staticmethod
    def mpi_grequest_free_function():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_grequest_free_function(correct_params.get("extra_state"))

    @staticmethod
    def mpi_grequest_query_function():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_grequest_query_function(correct_params.get("extra_state"),correct_params.get("status"))

    @staticmethod
    def mpi_grequest_start():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_grequest_start(correct_params.get("query_fn"),correct_params.get("free_fn"),correct_params.get("cancel_fn"),correct_params.get("extra_state"),correct_params.get("request"))

    @staticmethod
    def mpi_group_c2f():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_group_c2f(correct_params.get("group"))

    @staticmethod
    def mpi_group_compare():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_group_compare(correct_params.get("group1"),correct_params.get("group2"),correct_params.get("result"))

    @staticmethod
    def mpi_group_difference():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_group_difference(correct_params.get("group1"),correct_params.get("group2"),correct_params.get("newgroup"))

    @staticmethod
    def mpi_group_excl():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_group_excl(correct_params.get("group"),correct_params.get("n"),correct_params.get("ranks"),correct_params.get("newgroup"))

    @staticmethod
    def mpi_group_f2c():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_group_f2c(correct_params.get("group"))

    @staticmethod
    def mpi_group_free():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_group_free(correct_params.get("group"))

    @staticmethod
    def mpi_group_from_session_pset():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_group_from_session_pset(correct_params.get("session"),correct_params.get("pset_name"),correct_params.get("newgroup"))

    @staticmethod
    def mpi_group_incl():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_group_incl(correct_params.get("group"),correct_params.get("n"),correct_params.get("ranks"),correct_params.get("newgroup"))

    @staticmethod
    def mpi_group_intersection():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_group_intersection(correct_params.get("group1"),correct_params.get("group2"),correct_params.get("newgroup"))

    @staticmethod
    def mpi_group_range_excl():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_group_range_excl(correct_params.get("group"),correct_params.get("n"),correct_params.get("ranges"),correct_params.get("newgroup"))

    @staticmethod
    def mpi_group_range_incl():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_group_range_incl(correct_params.get("group"),correct_params.get("n"),correct_params.get("ranges"),correct_params.get("newgroup"))

    @staticmethod
    def mpi_group_rank():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_group_rank(correct_params.get("group"),correct_params.get("rank"))

    @staticmethod
    def mpi_group_size():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_group_size(correct_params.get("group"),correct_params.get("size"))

    @staticmethod
    def mpi_group_translate_ranks():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_group_translate_ranks(correct_params.get("group1"),correct_params.get("n"),correct_params.get("ranks1"),correct_params.get("group2"),correct_params.get("ranks2"))

    @staticmethod
    def mpi_group_union():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_group_union(correct_params.get("group1"),correct_params.get("group2"),correct_params.get("newgroup"))

    @staticmethod
    def mpi_iallgather():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_iallgather(correct_params.get("sendbuf"),correct_params.get("sendcount"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcount"),correct_params.get("recvtype"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_iallgatherv():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_iallgatherv(correct_params.get("sendbuf"),correct_params.get("sendcount"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcounts"),correct_params.get("displs"),correct_params.get("recvtype"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_iallreduce():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_iallreduce(correct_params.get("sendbuf"),correct_params.get("recvbuf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("op"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_ialltoall():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_ialltoall(correct_params.get("sendbuf"),correct_params.get("sendcount"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcount"),correct_params.get("recvtype"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_ialltoallv():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_ialltoallv(correct_params.get("sendbuf"),correct_params.get("sendcounts"),correct_params.get("sdispls"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcounts"),correct_params.get("rdispls"),correct_params.get("recvtype"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_ialltoallw():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_ialltoallw(correct_params.get("sendbuf"),correct_params.get("sendcounts"),correct_params.get("sdispls"),correct_params.get("sendtypes"),correct_params.get("recvbuf"),correct_params.get("recvcounts"),correct_params.get("rdispls"),correct_params.get("recvtypes"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_ibarrier():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_ibarrier(correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_ibcast():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_ibcast(correct_params.get("buffer"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("root"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_ibsend():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_ibsend(correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("dest"),correct_params.get("tag"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_iexscan():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_iexscan(correct_params.get("sendbuf"),correct_params.get("recvbuf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("op"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_igather():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_igather(correct_params.get("sendbuf"),correct_params.get("sendcount"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcount"),correct_params.get("recvtype"),correct_params.get("root"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_igatherv():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_igatherv(correct_params.get("sendbuf"),correct_params.get("sendcount"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcounts"),correct_params.get("displs"),correct_params.get("recvtype"),correct_params.get("root"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_improbe():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_improbe(correct_params.get("source"),correct_params.get("tag"),correct_params.get("comm"),correct_params.get("flag"),correct_params.get("message"),correct_params.get("status"))

    @staticmethod
    def mpi_imrecv():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_imrecv(correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("message"),correct_params.get("request"))

    @staticmethod
    def mpi_ineighbor_allgather():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_ineighbor_allgather(correct_params.get("sendbuf"),correct_params.get("sendcount"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcount"),correct_params.get("recvtype"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_ineighbor_allgatherv():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_ineighbor_allgatherv(correct_params.get("sendbuf"),correct_params.get("sendcount"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcounts"),correct_params.get("displs"),correct_params.get("recvtype"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_ineighbor_alltoall():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_ineighbor_alltoall(correct_params.get("sendbuf"),correct_params.get("sendcount"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcount"),correct_params.get("recvtype"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_ineighbor_alltoallv():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_ineighbor_alltoallv(correct_params.get("sendbuf"),correct_params.get("sendcounts"),correct_params.get("sdispls"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcounts"),correct_params.get("rdispls"),correct_params.get("recvtype"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_ineighbor_alltoallw():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_ineighbor_alltoallw(correct_params.get("sendbuf"),correct_params.get("sendcounts"),correct_params.get("sdispls"),correct_params.get("sendtypes"),correct_params.get("recvbuf"),correct_params.get("recvcounts"),correct_params.get("rdispls"),correct_params.get("recvtypes"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_info_c2f():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_info_c2f(correct_params.get("info"))

    @staticmethod
    def mpi_info_create():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_info_create(correct_params.get("info"))

    @staticmethod
    def mpi_info_create_env():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_info_create_env(correct_params.get("argc"),correct_params.get("argv"),correct_params.get("info"))

    @staticmethod
    def mpi_info_delete():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_info_delete(correct_params.get("info"),correct_params.get("key"))

    @staticmethod
    def mpi_info_dup():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_info_dup(correct_params.get("info"),correct_params.get("newinfo"))

    @staticmethod
    def mpi_info_f2c():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_info_f2c(correct_params.get("info"))

    @staticmethod
    def mpi_info_free():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_info_free(correct_params.get("info"))

    @staticmethod
    def mpi_info_get():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_info_get(correct_params.get("info"),correct_params.get("key"),correct_params.get("valuelen"),correct_params.get("value"),correct_params.get("flag"))

    @staticmethod
    def mpi_info_get_nkeys():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_info_get_nkeys(correct_params.get("info"),correct_params.get("nkeys"))

    @staticmethod
    def mpi_info_get_nthkey():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_info_get_nthkey(correct_params.get("info"),correct_params.get("n"),correct_params.get("key"))

    @staticmethod
    def mpi_info_get_string():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_info_get_string(correct_params.get("info"),correct_params.get("key"),correct_params.get("buflen"),correct_params.get("value"),correct_params.get("flag"))

    @staticmethod
    def mpi_info_get_valuelen():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_info_get_valuelen(correct_params.get("info"),correct_params.get("key"),correct_params.get("valuelen"),correct_params.get("flag"))

    @staticmethod
    def mpi_info_set():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_info_set(correct_params.get("info"),correct_params.get("key"),correct_params.get("value"))

    @staticmethod
    def mpi_init():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_init(correct_params.get("argc"),correct_params.get("argv"))

    @staticmethod
    def mpi_init_thread():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_init_thread(correct_params.get("argc"),correct_params.get("argv"),correct_params.get("required"),correct_params.get("provided"))

    @staticmethod
    def mpi_initialized():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_initialized(correct_params.get("flag"))

    @staticmethod
    def mpi_intercomm_create():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_intercomm_create(correct_params.get("local_comm"),correct_params.get("local_leader"),correct_params.get("peer_comm"),correct_params.get("remote_leader"),correct_params.get("tag"),correct_params.get("newintercomm"))

    @staticmethod
    def mpi_intercomm_create_from_groups():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_intercomm_create_from_groups(correct_params.get("local_group"),correct_params.get("local_leader"),correct_params.get("remote_group"),correct_params.get("remote_leader"),correct_params.get("stringtag"),correct_params.get("info"),correct_params.get("errhandler"),correct_params.get("newintercomm"))

    @staticmethod
    def mpi_intercomm_merge():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_intercomm_merge(correct_params.get("intercomm"),correct_params.get("high"),correct_params.get("newintracomm"))

    @staticmethod
    def mpi_iprobe():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_iprobe(correct_params.get("source"),correct_params.get("tag"),correct_params.get("comm"),correct_params.get("flag"),correct_params.get("status"))

    @staticmethod
    def mpi_irecv():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_irecv(correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("source"),correct_params.get("tag"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_ireduce():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_ireduce(correct_params.get("sendbuf"),correct_params.get("recvbuf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("op"),correct_params.get("root"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_ireduce_scatter():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_ireduce_scatter(correct_params.get("sendbuf"),correct_params.get("recvbuf"),correct_params.get("recvcounts"),correct_params.get("datatype"),correct_params.get("op"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_ireduce_scatter_block():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_ireduce_scatter_block(correct_params.get("sendbuf"),correct_params.get("recvbuf"),correct_params.get("recvcount"),correct_params.get("datatype"),correct_params.get("op"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_irsend():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_irsend(correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("dest"),correct_params.get("tag"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_is_thread_main():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_is_thread_main(correct_params.get("flag"))

    @staticmethod
    def mpi_iscan():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_iscan(correct_params.get("sendbuf"),correct_params.get("recvbuf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("op"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_iscatter():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_iscatter(correct_params.get("sendbuf"),correct_params.get("sendcount"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcount"),correct_params.get("recvtype"),correct_params.get("root"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_iscatterv():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_iscatterv(correct_params.get("sendbuf"),correct_params.get("sendcounts"),correct_params.get("displs"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcount"),correct_params.get("recvtype"),correct_params.get("root"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_isend():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_isend(correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("dest"),correct_params.get("tag"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_isendrecv():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_isendrecv(correct_params.get("sendbuf"),correct_params.get("sendcount"),correct_params.get("sendtype"),correct_params.get("dest"),correct_params.get("sendtag"),correct_params.get("recvbuf"),correct_params.get("recvcount"),correct_params.get("recvtype"),correct_params.get("source"),correct_params.get("recvtag"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_isendrecv_replace():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_isendrecv_replace(correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("dest"),correct_params.get("sendtag"),correct_params.get("source"),correct_params.get("recvtag"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_issend():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_issend(correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("dest"),correct_params.get("tag"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_keyval_create():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_keyval_create(correct_params.get("copy_fn"),correct_params.get("delete_fn"),correct_params.get("keyval"),correct_params.get("extra_state"))

    @staticmethod
    def mpi_keyval_free():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_keyval_free(correct_params.get("keyval"))

    @staticmethod
    def mpi_lookup_name():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_lookup_name(correct_params.get("service_name"),correct_params.get("info"),correct_params.get("port_name"))

    @staticmethod
    def mpi_message_c2f():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_message_c2f(correct_params.get("message"))

    @staticmethod
    def mpi_message_f2c():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_message_f2c(correct_params.get("message"))

    @staticmethod
    def mpi_mprobe():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_mprobe(correct_params.get("source"),correct_params.get("tag"),correct_params.get("comm"),correct_params.get("message"),correct_params.get("status"))

    @staticmethod
    def mpi_mrecv():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_mrecv(correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("message"),correct_params.get("status"))

    @staticmethod
    def mpi_neighbor_allgather():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_neighbor_allgather(correct_params.get("sendbuf"),correct_params.get("sendcount"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcount"),correct_params.get("recvtype"),correct_params.get("comm"))

    @staticmethod
    def mpi_neighbor_allgather_init():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_neighbor_allgather_init(correct_params.get("sendbuf"),correct_params.get("sendcount"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcount"),correct_params.get("recvtype"),correct_params.get("comm"),correct_params.get("info"),correct_params.get("request"))

    @staticmethod
    def mpi_neighbor_allgatherv():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_neighbor_allgatherv(correct_params.get("sendbuf"),correct_params.get("sendcount"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcounts"),correct_params.get("displs"),correct_params.get("recvtype"),correct_params.get("comm"))

    @staticmethod
    def mpi_neighbor_allgatherv_init():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_neighbor_allgatherv_init(correct_params.get("sendbuf"),correct_params.get("sendcount"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcounts"),correct_params.get("displs"),correct_params.get("recvtype"),correct_params.get("comm"),correct_params.get("info"),correct_params.get("request"))

    @staticmethod
    def mpi_neighbor_alltoall():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_neighbor_alltoall(correct_params.get("sendbuf"),correct_params.get("sendcount"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcount"),correct_params.get("recvtype"),correct_params.get("comm"))

    @staticmethod
    def mpi_neighbor_alltoall_init():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_neighbor_alltoall_init(correct_params.get("sendbuf"),correct_params.get("sendcount"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcount"),correct_params.get("recvtype"),correct_params.get("comm"),correct_params.get("info"),correct_params.get("request"))

    @staticmethod
    def mpi_neighbor_alltoallv():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_neighbor_alltoallv(correct_params.get("sendbuf"),correct_params.get("sendcounts"),correct_params.get("sdispls"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcounts"),correct_params.get("rdispls"),correct_params.get("recvtype"),correct_params.get("comm"))

    @staticmethod
    def mpi_neighbor_alltoallv_init():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_neighbor_alltoallv_init(correct_params.get("sendbuf"),correct_params.get("sendcounts"),correct_params.get("sdispls"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcounts"),correct_params.get("rdispls"),correct_params.get("recvtype"),correct_params.get("comm"),correct_params.get("info"),correct_params.get("request"))

    @staticmethod
    def mpi_neighbor_alltoallw():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_neighbor_alltoallw(correct_params.get("sendbuf"),correct_params.get("sendcounts"),correct_params.get("sdispls"),correct_params.get("sendtypes"),correct_params.get("recvbuf"),correct_params.get("recvcounts"),correct_params.get("rdispls"),correct_params.get("recvtypes"),correct_params.get("comm"))

    @staticmethod
    def mpi_neighbor_alltoallw_init():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_neighbor_alltoallw_init(correct_params.get("sendbuf"),correct_params.get("sendcounts"),correct_params.get("sdispls"),correct_params.get("sendtypes"),correct_params.get("recvbuf"),correct_params.get("recvcounts"),correct_params.get("rdispls"),correct_params.get("recvtypes"),correct_params.get("comm"),correct_params.get("info"),correct_params.get("request"))

    @staticmethod
    def mpi_null_copy_fn():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_null_copy_fn(correct_params.get("oldcomm"),correct_params.get("keyval"),correct_params.get("extra_state"),correct_params.get("attribute_val_in"),correct_params.get("attribute_val_out"),correct_params.get("flag"))

    @staticmethod
    def mpi_null_delete_fn():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_null_delete_fn(correct_params.get("comm"),correct_params.get("keyval"),correct_params.get("attribute_val"),correct_params.get("extra_state"))

    @staticmethod
    def mpi_op_c2f():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_op_c2f(correct_params.get("op"))

    @staticmethod
    def mpi_op_commutative():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_op_commutative(correct_params.get("op"),correct_params.get("commute"))

    @staticmethod
    def mpi_op_create():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_op_create(correct_params.get("user_fn"),correct_params.get("commute"),correct_params.get("op"))

    @staticmethod
    def mpi_op_f2c():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_op_f2c(correct_params.get("op"))

    @staticmethod
    def mpi_op_free():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_op_free(correct_params.get("op"))

    @staticmethod
    def mpi_open_port():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_open_port(correct_params.get("info"),correct_params.get("port_name"))

    @staticmethod
    def mpi_pack():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_pack(correct_params.get("inbuf"),correct_params.get("incount"),correct_params.get("datatype"),correct_params.get("outbuf"),correct_params.get("outsize"),correct_params.get("position"),correct_params.get("comm"))

    @staticmethod
    def mpi_pack_external():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_pack_external(correct_params.get("datarep"),correct_params.get("inbuf"),correct_params.get("incount"),correct_params.get("datatype"),correct_params.get("outbuf"),correct_params.get("outsize"),correct_params.get("position"))

    @staticmethod
    def mpi_pack_external_size():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_pack_external_size(correct_params.get("datarep"),correct_params.get("incount"),correct_params.get("datatype"),correct_params.get("size"))

    @staticmethod
    def mpi_pack_size():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_pack_size(correct_params.get("incount"),correct_params.get("datatype"),correct_params.get("comm"),correct_params.get("size"))

    @staticmethod
    def mpi_parrived():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_parrived(correct_params.get("request"),correct_params.get("partition"),correct_params.get("flag"))

    @staticmethod
    def mpi_pcontrol():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_pcontrol(correct_params.get("level"),correct_params.get("varargs"))

    @staticmethod
    def mpi_pready():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_pready(correct_params.get("partition"),correct_params.get("request"))

    @staticmethod
    def mpi_pready_list():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_pready_list(correct_params.get("length"),correct_params.get("array_of_partitions"),correct_params.get("request"))

    @staticmethod
    def mpi_pready_range():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_pready_range(correct_params.get("partition_low"),correct_params.get("partition_high"),correct_params.get("request"))

    @staticmethod
    def mpi_precv_init():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_precv_init(correct_params.get("buf"),correct_params.get("partitions"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("source"),correct_params.get("tag"),correct_params.get("comm"),correct_params.get("info"),correct_params.get("request"))

    @staticmethod
    def mpi_probe():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_probe(correct_params.get("source"),correct_params.get("tag"),correct_params.get("comm"),correct_params.get("status"))

    @staticmethod
    def mpi_psend_init():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_psend_init(correct_params.get("buf"),correct_params.get("partitions"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("dest"),correct_params.get("tag"),correct_params.get("comm"),correct_params.get("info"),correct_params.get("request"))

    @staticmethod
    def mpi_publish_name():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_publish_name(correct_params.get("service_name"),correct_params.get("info"),correct_params.get("port_name"))

    @staticmethod
    def mpi_put():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_put(correct_params.get("origin_addr"),correct_params.get("origin_count"),correct_params.get("origin_datatype"),correct_params.get("target_rank"),correct_params.get("target_disp"),correct_params.get("target_count"),correct_params.get("target_datatype"),correct_params.get("win"))

    @staticmethod
    def mpi_query_thread():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_query_thread(correct_params.get("provided"))

    @staticmethod
    def mpi_raccumulate():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_raccumulate(correct_params.get("origin_addr"),correct_params.get("origin_count"),correct_params.get("origin_datatype"),correct_params.get("target_rank"),correct_params.get("target_disp"),correct_params.get("target_count"),correct_params.get("target_datatype"),correct_params.get("op"),correct_params.get("win"),correct_params.get("request"))

    @staticmethod
    def mpi_recv():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_recv(correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("source"),correct_params.get("tag"),correct_params.get("comm"),correct_params.get("status"))

    @staticmethod
    def mpi_recv_init():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_recv_init(correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("source"),correct_params.get("tag"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_reduce():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_reduce(correct_params.get("sendbuf"),correct_params.get("recvbuf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("op"),correct_params.get("root"),correct_params.get("comm"))

    @staticmethod
    def mpi_reduce_init():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_reduce_init(correct_params.get("sendbuf"),correct_params.get("recvbuf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("op"),correct_params.get("root"),correct_params.get("comm"),correct_params.get("info"),correct_params.get("request"))

    @staticmethod
    def mpi_reduce_local():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_reduce_local(correct_params.get("inbuf"),correct_params.get("inoutbuf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("op"))

    @staticmethod
    def mpi_reduce_scatter():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_reduce_scatter(correct_params.get("sendbuf"),correct_params.get("recvbuf"),correct_params.get("recvcounts"),correct_params.get("datatype"),correct_params.get("op"),correct_params.get("comm"))

    @staticmethod
    def mpi_reduce_scatter_block():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_reduce_scatter_block(correct_params.get("sendbuf"),correct_params.get("recvbuf"),correct_params.get("recvcount"),correct_params.get("datatype"),correct_params.get("op"),correct_params.get("comm"))

    @staticmethod
    def mpi_reduce_scatter_block_init():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_reduce_scatter_block_init(correct_params.get("sendbuf"),correct_params.get("recvbuf"),correct_params.get("recvcount"),correct_params.get("datatype"),correct_params.get("op"),correct_params.get("comm"),correct_params.get("info"),correct_params.get("request"))

    @staticmethod
    def mpi_reduce_scatter_init():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_reduce_scatter_init(correct_params.get("sendbuf"),correct_params.get("recvbuf"),correct_params.get("recvcounts"),correct_params.get("datatype"),correct_params.get("op"),correct_params.get("comm"),correct_params.get("info"),correct_params.get("request"))

    @staticmethod
    def mpi_register_datarep():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_register_datarep(correct_params.get("datarep"),correct_params.get("read_conversion_fn"),correct_params.get("write_conversion_fn"),correct_params.get("dtype_file_extent_fn"),correct_params.get("extra_state"))

    @staticmethod
    def mpi_request_c2f():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_request_c2f(correct_params.get("request"))

    @staticmethod
    def mpi_request_f2c():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_request_f2c(correct_params.get("request"))

    @staticmethod
    def mpi_request_free():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_request_free(correct_params.get("request"))

    @staticmethod
    def mpi_request_get_status():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_request_get_status(correct_params.get("request"),correct_params.get("flag"),correct_params.get("status"))

    @staticmethod
    def mpi_rget():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_rget(correct_params.get("origin_addr"),correct_params.get("origin_count"),correct_params.get("origin_datatype"),correct_params.get("target_rank"),correct_params.get("target_disp"),correct_params.get("target_count"),correct_params.get("target_datatype"),correct_params.get("win"),correct_params.get("request"))

    @staticmethod
    def mpi_rget_accumulate():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_rget_accumulate(correct_params.get("origin_addr"),correct_params.get("origin_count"),correct_params.get("origin_datatype"),correct_params.get("result_addr"),correct_params.get("result_count"),correct_params.get("result_datatype"),correct_params.get("target_rank"),correct_params.get("target_disp"),correct_params.get("target_count"),correct_params.get("target_datatype"),correct_params.get("op"),correct_params.get("win"),correct_params.get("request"))

    @staticmethod
    def mpi_rput():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_rput(correct_params.get("origin_addr"),correct_params.get("origin_count"),correct_params.get("origin_datatype"),correct_params.get("target_rank"),correct_params.get("target_disp"),correct_params.get("target_count"),correct_params.get("target_datatype"),correct_params.get("win"),correct_params.get("request"))

    @staticmethod
    def mpi_rsend():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_rsend(correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("dest"),correct_params.get("tag"),correct_params.get("comm"))

    @staticmethod
    def mpi_rsend_init():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_rsend_init(correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("dest"),correct_params.get("tag"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_scan():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_scan(correct_params.get("sendbuf"),correct_params.get("recvbuf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("op"),correct_params.get("comm"))

    @staticmethod
    def mpi_scan_init():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_scan_init(correct_params.get("sendbuf"),correct_params.get("recvbuf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("op"),correct_params.get("comm"),correct_params.get("info"),correct_params.get("request"))

    @staticmethod
    def mpi_scatter():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_scatter(correct_params.get("sendbuf"),correct_params.get("sendcount"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcount"),correct_params.get("recvtype"),correct_params.get("root"),correct_params.get("comm"))

    @staticmethod
    def mpi_scatter_init():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_scatter_init(correct_params.get("sendbuf"),correct_params.get("sendcount"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcount"),correct_params.get("recvtype"),correct_params.get("root"),correct_params.get("comm"),correct_params.get("info"),correct_params.get("request"))

    @staticmethod
    def mpi_scatterv():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_scatterv(correct_params.get("sendbuf"),correct_params.get("sendcounts"),correct_params.get("displs"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcount"),correct_params.get("recvtype"),correct_params.get("root"),correct_params.get("comm"))

    @staticmethod
    def mpi_scatterv_init():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_scatterv_init(correct_params.get("sendbuf"),correct_params.get("sendcounts"),correct_params.get("displs"),correct_params.get("sendtype"),correct_params.get("recvbuf"),correct_params.get("recvcount"),correct_params.get("recvtype"),correct_params.get("root"),correct_params.get("comm"),correct_params.get("info"),correct_params.get("request"))

    @staticmethod
    def mpi_send():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_send(correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("dest"),correct_params.get("tag"),correct_params.get("comm"))

    @staticmethod
    def mpi_send_init():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_send_init(correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("dest"),correct_params.get("tag"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_sendrecv():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_sendrecv(correct_params.get("sendbuf"),correct_params.get("sendcount"),correct_params.get("sendtype"),correct_params.get("dest"),correct_params.get("sendtag"),correct_params.get("recvbuf"),correct_params.get("recvcount"),correct_params.get("recvtype"),correct_params.get("source"),correct_params.get("recvtag"),correct_params.get("comm"),correct_params.get("status"))

    @staticmethod
    def mpi_sendrecv_replace():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_sendrecv_replace(correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("dest"),correct_params.get("sendtag"),correct_params.get("source"),correct_params.get("recvtag"),correct_params.get("comm"),correct_params.get("status"))

    @staticmethod
    def mpi_session_c2f():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_session_c2f(correct_params.get("session"))

    @staticmethod
    def mpi_session_call_errhandler():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_session_call_errhandler(correct_params.get("session"),correct_params.get("errorcode"))

    @staticmethod
    def mpi_session_create_errhandler():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_session_create_errhandler(correct_params.get("session_errhandler_fn"),correct_params.get("errhandler"))

    @staticmethod
    def mpi_session_errhandler_function():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_session_errhandler_function(correct_params.get("session"),correct_params.get("error_code"),correct_params.get("varargs"))

    @staticmethod
    def mpi_session_f2c():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_session_f2c(correct_params.get("session"))

    @staticmethod
    def mpi_session_finalize():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_session_finalize(correct_params.get("session"))

    @staticmethod
    def mpi_session_get_errhandler():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_session_get_errhandler(correct_params.get("session"),correct_params.get("errhandler"))

    @staticmethod
    def mpi_session_get_info():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_session_get_info(correct_params.get("session"),correct_params.get("info_used"))

    @staticmethod
    def mpi_session_get_nth_pset():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_session_get_nth_pset(correct_params.get("session"),correct_params.get("info"),correct_params.get("n"),correct_params.get("pset_len"),correct_params.get("pset_name"))

    @staticmethod
    def mpi_session_get_num_psets():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_session_get_num_psets(correct_params.get("session"),correct_params.get("info"),correct_params.get("npset_names"))

    @staticmethod
    def mpi_session_get_pset_info():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_session_get_pset_info(correct_params.get("session"),correct_params.get("pset_name"),correct_params.get("info"))

    @staticmethod
    def mpi_session_init():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_session_init(correct_params.get("info"),correct_params.get("errhandler"),correct_params.get("session"))

    @staticmethod
    def mpi_session_set_errhandler():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_session_set_errhandler(correct_params.get("session"),correct_params.get("errhandler"))

    @staticmethod
    def mpi_sizeof():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_sizeof(correct_params.get("x"),correct_params.get("size"))

    @staticmethod
    def mpi_ssend():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_ssend(correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("dest"),correct_params.get("tag"),correct_params.get("comm"))

    @staticmethod
    def mpi_ssend_init():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_ssend_init(correct_params.get("buf"),correct_params.get("count"),correct_params.get("datatype"),correct_params.get("dest"),correct_params.get("tag"),correct_params.get("comm"),correct_params.get("request"))

    @staticmethod
    def mpi_start():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_start(correct_params.get("request"))

    @staticmethod
    def mpi_startall():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_startall(correct_params.get("count"),correct_params.get("array_of_requests"))

    @staticmethod
    def mpi_status_c2f():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_status_c2f(correct_params.get("c_status"),correct_params.get("f_status"))

    @staticmethod
    def mpi_status_c2f08():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_status_c2f08(correct_params.get("c_status"),correct_params.get("f08_status"))

    @staticmethod
    def mpi_status_f082c():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_status_f082c(correct_params.get("f08_status"),correct_params.get("c_status"))

    @staticmethod
    def mpi_status_f082f():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_status_f082f(correct_params.get("f08_status"),correct_params.get("f_status"))

    @staticmethod
    def mpi_status_f2c():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_status_f2c(correct_params.get("f_status"),correct_params.get("c_status"))

    @staticmethod
    def mpi_status_f2f08():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_status_f2f08(correct_params.get("f_status"),correct_params.get("f08_status"))

    @staticmethod
    def mpi_status_set_cancelled():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_status_set_cancelled(correct_params.get("status"),correct_params.get("flag"))

    @staticmethod
    def mpi_status_set_elements():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_status_set_elements(correct_params.get("status"),correct_params.get("datatype"),correct_params.get("count"))

    @staticmethod
    def mpi_status_set_elements_x():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_status_set_elements_x(correct_params.get("status"),correct_params.get("datatype"),correct_params.get("count"))

    @staticmethod
    def mpi_t_category_changed():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_category_changed(correct_params.get("update_number"))

    @staticmethod
    def mpi_t_category_get_categories():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_category_get_categories(correct_params.get("cat_index"),correct_params.get("len"),correct_params.get("indices"))

    @staticmethod
    def mpi_t_category_get_cvars():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_category_get_cvars(correct_params.get("cat_index"),correct_params.get("len"),correct_params.get("indices"))

    @staticmethod
    def mpi_t_category_get_events():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_category_get_events(correct_params.get("cat_index"),correct_params.get("len"),correct_params.get("indices"))

    @staticmethod
    def mpi_t_category_get_index():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_category_get_index(correct_params.get("name"),correct_params.get("cat_index"))

    @staticmethod
    def mpi_t_category_get_info():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_category_get_info(correct_params.get("cat_index"),correct_params.get("name"),correct_params.get("name_len"),correct_params.get("desc"),correct_params.get("desc_len"),correct_params.get("num_cvars"),correct_params.get("num_pvars"),correct_params.get("num_categories"))

    @staticmethod
    def mpi_t_category_get_num():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_category_get_num(correct_params.get("num_cat"))

    @staticmethod
    def mpi_t_category_get_num_events():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_category_get_num_events(correct_params.get("cat_index"),correct_params.get("num_events"))

    @staticmethod
    def mpi_t_category_get_pvars():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_category_get_pvars(correct_params.get("cat_index"),correct_params.get("len"),correct_params.get("indices"))

    @staticmethod
    def mpi_t_cvar_get_index():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_cvar_get_index(correct_params.get("name"),correct_params.get("cvar_index"))

    @staticmethod
    def mpi_t_cvar_get_info():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_cvar_get_info(correct_params.get("cvar_index"),correct_params.get("name"),correct_params.get("name_len"),correct_params.get("verbosity"),correct_params.get("datatype"),correct_params.get("enumtype"),correct_params.get("desc"),correct_params.get("desc_len"),correct_params.get("bind"),correct_params.get("scope"))

    @staticmethod
    def mpi_t_cvar_get_num():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_cvar_get_num(correct_params.get("num_cvar"))

    @staticmethod
    def mpi_t_cvar_handle_alloc():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_cvar_handle_alloc(correct_params.get("cvar_index"),correct_params.get("obj_handle"),correct_params.get("handle"),correct_params.get("count"))

    @staticmethod
    def mpi_t_cvar_handle_free():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_cvar_handle_free(correct_params.get("handle"))

    @staticmethod
    def mpi_t_cvar_read():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_cvar_read(correct_params.get("handle"),correct_params.get("buf"))

    @staticmethod
    def mpi_t_cvar_write():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_cvar_write(correct_params.get("handle"),correct_params.get("buf"))

    @staticmethod
    def mpi_t_enum_get_info():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_enum_get_info(correct_params.get("enumtype"),correct_params.get("num"),correct_params.get("name"),correct_params.get("name_len"))

    @staticmethod
    def mpi_t_enum_get_item():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_enum_get_item(correct_params.get("enumtype"),correct_params.get("index"),correct_params.get("value"),correct_params.get("name"),correct_params.get("name_len"))

    @staticmethod
    def mpi_t_event_callback_get_info():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_event_callback_get_info(correct_params.get("event_registration"),correct_params.get("cb_safety"),correct_params.get("info_used"))

    @staticmethod
    def mpi_t_event_callback_set_info():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_event_callback_set_info(correct_params.get("event_registration"),correct_params.get("cb_safety"),correct_params.get("info"))

    @staticmethod
    def mpi_t_event_cb_function():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_event_cb_function(correct_params.get("event_instance"),correct_params.get("event_registration"),correct_params.get("cb_safety"),correct_params.get("user_data"))

    @staticmethod
    def mpi_t_event_copy():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_event_copy(correct_params.get("event_instance"),correct_params.get("buffer"))

    @staticmethod
    def mpi_t_event_dropped_cb_function():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_event_dropped_cb_function(correct_params.get("count"),correct_params.get("event_registration"),correct_params.get("source_index"),correct_params.get("cb_safety"),correct_params.get("user_data"))

    @staticmethod
    def mpi_t_event_free_cb_function():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_event_free_cb_function(correct_params.get("event_registration"),correct_params.get("cb_safety"),correct_params.get("user_data"))

    @staticmethod
    def mpi_t_event_get_index():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_event_get_index(correct_params.get("name"),correct_params.get("event_index"))

    @staticmethod
    def mpi_t_event_get_info():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_event_get_info(correct_params.get("event_index"),correct_params.get("name"),correct_params.get("name_len"),correct_params.get("verbosity"),correct_params.get("array_of_datatypes"),correct_params.get("array_of_displacements"),correct_params.get("num_elements"),correct_params.get("enumtype"),correct_params.get("info"),correct_params.get("desc"),correct_params.get("desc_len"),correct_params.get("bind"))

    @staticmethod
    def mpi_t_event_get_num():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_event_get_num(correct_params.get("num_events"))

    @staticmethod
    def mpi_t_event_get_source():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_event_get_source(correct_params.get("event_instance"),correct_params.get("source_index"))

    @staticmethod
    def mpi_t_event_get_timestamp():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_event_get_timestamp(correct_params.get("event_instance"),correct_params.get("event_timestamp"))

    @staticmethod
    def mpi_t_event_handle_alloc():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_event_handle_alloc(correct_params.get("event_index"),correct_params.get("obj_handle"),correct_params.get("info"),correct_params.get("event_registration"))

    @staticmethod
    def mpi_t_event_handle_free():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_event_handle_free(correct_params.get("event_registration"),correct_params.get("user_data"),correct_params.get("free_cb_function"))

    @staticmethod
    def mpi_t_event_handle_get_info():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_event_handle_get_info(correct_params.get("event_registration"),correct_params.get("info_used"))

    @staticmethod
    def mpi_t_event_handle_set_info():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_event_handle_set_info(correct_params.get("event_registration"),correct_params.get("info"))

    @staticmethod
    def mpi_t_event_read():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_event_read(correct_params.get("event_instance"),correct_params.get("element_index"),correct_params.get("buffer"))

    @staticmethod
    def mpi_t_event_register_callback():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_event_register_callback(correct_params.get("event_registration"),correct_params.get("cb_safety"),correct_params.get("info"),correct_params.get("user_data"),correct_params.get("event_cb_function"))

    @staticmethod
    def mpi_t_event_set_dropped_handler():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_event_set_dropped_handler(correct_params.get("event_registration"),correct_params.get("dropped_cb_function"))

    @staticmethod
    def mpi_t_finalize():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_finalize()

    @staticmethod
    def mpi_t_init_thread():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_init_thread(correct_params.get("required"),correct_params.get("provided"))

    @staticmethod
    def mpi_t_pvar_get_index():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_pvar_get_index(correct_params.get("name"),correct_params.get("var_class"),correct_params.get("pvar_index"))

    @staticmethod
    def mpi_t_pvar_get_info():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_pvar_get_info(correct_params.get("pvar_index"),correct_params.get("name"),correct_params.get("name_len"),correct_params.get("verbosity"),correct_params.get("var_class"),correct_params.get("datatype"),correct_params.get("enumtype"),correct_params.get("desc"),correct_params.get("desc_len"),correct_params.get("bind"),correct_params.get("readonly"),correct_params.get("continuous"),correct_params.get("atomic"))

    @staticmethod
    def mpi_t_pvar_get_num():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_pvar_get_num(correct_params.get("num_pvar"))

    @staticmethod
    def mpi_t_pvar_handle_alloc():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_pvar_handle_alloc(correct_params.get("pe_session"),correct_params.get("pvar_index"),correct_params.get("obj_handle"),correct_params.get("handle"),correct_params.get("count"))

    @staticmethod
    def mpi_t_pvar_handle_free():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_pvar_handle_free(correct_params.get("pe_session"),correct_params.get("handle"))

    @staticmethod
    def mpi_t_pvar_read():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_pvar_read(correct_params.get("pe_session"),correct_params.get("handle"),correct_params.get("buf"))

    @staticmethod
    def mpi_t_pvar_readreset():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_pvar_readreset(correct_params.get("pe_session"),correct_params.get("handle"),correct_params.get("buf"))

    @staticmethod
    def mpi_t_pvar_reset():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_pvar_reset(correct_params.get("pe_session"),correct_params.get("handle"))

    @staticmethod
    def mpi_t_pvar_session_create():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_pvar_session_create(correct_params.get("pe_session"))

    @staticmethod
    def mpi_t_pvar_session_free():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_pvar_session_free(correct_params.get("pe_session"))

    @staticmethod
    def mpi_t_pvar_start():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_pvar_start(correct_params.get("pe_session"),correct_params.get("handle"))

    @staticmethod
    def mpi_t_pvar_stop():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_pvar_stop(correct_params.get("pe_session"),correct_params.get("handle"))

    @staticmethod
    def mpi_t_pvar_write():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_pvar_write(correct_params.get("pe_session"),correct_params.get("handle"),correct_params.get("buf"))

    @staticmethod
    def mpi_t_source_get_info():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_source_get_info(correct_params.get("source_index"),correct_params.get("name"),correct_params.get("name_len"),correct_params.get("desc"),correct_params.get("desc_len"),correct_params.get("ordering"),correct_params.get("ticks_per_second"),correct_params.get("max_ticks"),correct_params.get("info"))

    @staticmethod
    def mpi_t_source_get_num():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_source_get_num(correct_params.get("num_sources"))

    @staticmethod
    def mpi_t_source_get_timestamp():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_t_source_get_timestamp(correct_params.get("source_index"),correct_params.get("timestamp"))

    @staticmethod
    def mpi_test():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_test(correct_params.get("request"),correct_params.get("flag"),correct_params.get("status"))

    @staticmethod
    def mpi_test_cancelled():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_test_cancelled(correct_params.get("status"),correct_params.get("flag"))

    @staticmethod
    def mpi_testall():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_testall(correct_params.get("count"),correct_params.get("array_of_requests"),correct_params.get("flag"),correct_params.get("array_of_statuses"))

    @staticmethod
    def mpi_testany():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_testany(correct_params.get("count"),correct_params.get("array_of_requests"),correct_params.get("index"),correct_params.get("flag"),correct_params.get("status"))

    @staticmethod
    def mpi_testsome():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_testsome(correct_params.get("incount"),correct_params.get("array_of_requests"),correct_params.get("outcount"),correct_params.get("array_of_indices"),correct_params.get("array_of_statuses"))

    @staticmethod
    def mpi_topo_test():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_topo_test(correct_params.get("comm"),correct_params.get("status"))

    @staticmethod
    def mpi_type_c2f():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_c2f(correct_params.get("datatype"))

    @staticmethod
    def mpi_type_commit():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_commit(correct_params.get("datatype"))

    @staticmethod
    def mpi_type_contiguous():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_contiguous(correct_params.get("count"),correct_params.get("oldtype"),correct_params.get("newtype"))

    @staticmethod
    def mpi_type_copy_attr_function():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_copy_attr_function(correct_params.get("oldtype"),correct_params.get("type_keyval"),correct_params.get("extra_state"),correct_params.get("attribute_val_in"),correct_params.get("attribute_val_out"),correct_params.get("flag"))

    @staticmethod
    def mpi_type_create_darray():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_create_darray(correct_params.get("size"),correct_params.get("rank"),correct_params.get("ndims"),correct_params.get("array_of_gsizes"),correct_params.get("array_of_distribs"),correct_params.get("array_of_dargs"),correct_params.get("array_of_psizes"),correct_params.get("order"),correct_params.get("oldtype"),correct_params.get("newtype"))

    @staticmethod
    def mpi_type_create_f90_complex():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_create_f90_complex(correct_params.get("p"),correct_params.get("r"),correct_params.get("newtype"))

    @staticmethod
    def mpi_type_create_f90_integer():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_create_f90_integer(correct_params.get("r"),correct_params.get("newtype"))

    @staticmethod
    def mpi_type_create_f90_real():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_create_f90_real(correct_params.get("p"),correct_params.get("r"),correct_params.get("newtype"))

    @staticmethod
    def mpi_type_create_hindexed():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_create_hindexed(correct_params.get("count"),correct_params.get("array_of_blocklengths"),correct_params.get("array_of_displacements"),correct_params.get("oldtype"),correct_params.get("newtype"))

    @staticmethod
    def mpi_type_create_hindexed_block():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_create_hindexed_block(correct_params.get("count"),correct_params.get("blocklength"),correct_params.get("array_of_displacements"),correct_params.get("oldtype"),correct_params.get("newtype"))

    @staticmethod
    def mpi_type_create_hvector():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_create_hvector(correct_params.get("count"),correct_params.get("blocklength"),correct_params.get("stride"),correct_params.get("oldtype"),correct_params.get("newtype"))

    @staticmethod
    def mpi_type_create_indexed_block():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_create_indexed_block(correct_params.get("count"),correct_params.get("blocklength"),correct_params.get("array_of_displacements"),correct_params.get("oldtype"),correct_params.get("newtype"))

    @staticmethod
    def mpi_type_create_keyval():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_create_keyval(correct_params.get("type_copy_attr_fn"),correct_params.get("type_delete_attr_fn"),correct_params.get("type_keyval"),correct_params.get("extra_state"))

    @staticmethod
    def mpi_type_create_resized():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_create_resized(correct_params.get("oldtype"),correct_params.get("lb"),correct_params.get("extent"),correct_params.get("newtype"))

    @staticmethod
    def mpi_type_create_struct():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_create_struct(correct_params.get("count"),correct_params.get("array_of_blocklengths"),correct_params.get("array_of_displacements"),correct_params.get("array_of_types"),correct_params.get("newtype"))

    @staticmethod
    def mpi_type_create_subarray():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_create_subarray(correct_params.get("ndims"),correct_params.get("array_of_sizes"),correct_params.get("array_of_subsizes"),correct_params.get("array_of_starts"),correct_params.get("order"),correct_params.get("oldtype"),correct_params.get("newtype"))

    @staticmethod
    def mpi_type_delete_attr():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_delete_attr(correct_params.get("datatype"),correct_params.get("type_keyval"))

    @staticmethod
    def mpi_type_delete_attr_function():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_delete_attr_function(correct_params.get("datatype"),correct_params.get("type_keyval"),correct_params.get("attribute_val"),correct_params.get("extra_state"))

    @staticmethod
    def mpi_type_dup():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_dup(correct_params.get("oldtype"),correct_params.get("newtype"))

    @staticmethod
    def mpi_type_dup_fn():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_dup_fn(correct_params.get("oldtype"),correct_params.get("type_keyval"),correct_params.get("extra_state"),correct_params.get("attribute_val_in"),correct_params.get("attribute_val_out"),correct_params.get("flag"))

    @staticmethod
    def mpi_type_f2c():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_f2c(correct_params.get("datatype"))

    @staticmethod
    def mpi_type_free():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_free(correct_params.get("datatype"))

    @staticmethod
    def mpi_type_free_keyval():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_free_keyval(correct_params.get("type_keyval"))

    @staticmethod
    def mpi_type_get_attr():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_get_attr(correct_params.get("datatype"),correct_params.get("type_keyval"),correct_params.get("attribute_val"),correct_params.get("flag"))

    @staticmethod
    def mpi_type_get_contents():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_get_contents(correct_params.get("datatype"),correct_params.get("max_integers"),correct_params.get("max_addresses"),correct_params.get("max_large_counts"),correct_params.get("max_datatypes"),correct_params.get("array_of_integers"),correct_params.get("array_of_addresses"),correct_params.get("array_of_large_counts"),correct_params.get("array_of_datatypes"))

    @staticmethod
    def mpi_type_get_envelope():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_get_envelope(correct_params.get("datatype"),correct_params.get("num_integers"),correct_params.get("num_addresses"),correct_params.get("num_large_counts"),correct_params.get("num_datatypes"),correct_params.get("combiner"))

    @staticmethod
    def mpi_type_get_extent():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_get_extent(correct_params.get("datatype"),correct_params.get("lb"),correct_params.get("extent"))

    @staticmethod
    def mpi_type_get_extent_x():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_get_extent_x(correct_params.get("datatype"),correct_params.get("lb"),correct_params.get("extent"))

    @staticmethod
    def mpi_type_get_name():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_get_name(correct_params.get("datatype"),correct_params.get("type_name"),correct_params.get("resultlen"))

    @staticmethod
    def mpi_type_get_true_extent():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_get_true_extent(correct_params.get("datatype"),correct_params.get("true_lb"),correct_params.get("true_extent"))

    @staticmethod
    def mpi_type_get_true_extent_x():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_get_true_extent_x(correct_params.get("datatype"),correct_params.get("true_lb"),correct_params.get("true_extent"))

    @staticmethod
    def mpi_type_indexed():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_indexed(correct_params.get("count"),correct_params.get("array_of_blocklengths"),correct_params.get("array_of_displacements"),correct_params.get("oldtype"),correct_params.get("newtype"))

    @staticmethod
    def mpi_type_match_size():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_match_size(correct_params.get("typeclass"),correct_params.get("size"),correct_params.get("datatype"))

    @staticmethod
    def mpi_type_null_copy_fn():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_null_copy_fn(correct_params.get("oldtype"),correct_params.get("type_keyval"),correct_params.get("extra_state"),correct_params.get("attribute_val_in"),correct_params.get("attribute_val_out"),correct_params.get("flag"))

    @staticmethod
    def mpi_type_null_delete_fn():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_null_delete_fn(correct_params.get("datatype"),correct_params.get("type_keyval"),correct_params.get("attribute_val"),correct_params.get("extra_state"))

    @staticmethod
    def mpi_type_set_attr():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_set_attr(correct_params.get("datatype"),correct_params.get("type_keyval"),correct_params.get("attribute_val"))

    @staticmethod
    def mpi_type_set_name():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_set_name(correct_params.get("datatype"),correct_params.get("type_name"))

    @staticmethod
    def mpi_type_size():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_size(correct_params.get("datatype"),correct_params.get("size"))

    @staticmethod
    def mpi_type_size_x():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_size_x(correct_params.get("datatype"),correct_params.get("size"))

    @staticmethod
    def mpi_type_vector():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_type_vector(correct_params.get("count"),correct_params.get("blocklength"),correct_params.get("stride"),correct_params.get("oldtype"),correct_params.get("newtype"))

    @staticmethod
    def mpi_unpack():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_unpack(correct_params.get("inbuf"),correct_params.get("insize"),correct_params.get("position"),correct_params.get("outbuf"),correct_params.get("outcount"),correct_params.get("datatype"),correct_params.get("comm"))

    @staticmethod
    def mpi_unpack_external():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_unpack_external(correct_params.get("datarep"),correct_params.get("inbuf"),correct_params.get("insize"),correct_params.get("position"),correct_params.get("outbuf"),correct_params.get("outcount"),correct_params.get("datatype"))

    @staticmethod
    def mpi_unpublish_name():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_unpublish_name(correct_params.get("service_name"),correct_params.get("info"),correct_params.get("port_name"))

    @staticmethod
    def mpi_user_function():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_user_function(correct_params.get("invec"),correct_params.get("inoutvec"),correct_params.get("len"),correct_params.get("datatype"))

    @staticmethod
    def mpi_wait():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_wait(correct_params.get("request"),correct_params.get("status"))

    @staticmethod
    def mpi_waitall():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_waitall(correct_params.get("count"),correct_params.get("array_of_requests"),correct_params.get("array_of_statuses"))

    @staticmethod
    def mpi_waitany():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_waitany(correct_params.get("count"),correct_params.get("array_of_requests"),correct_params.get("index"),correct_params.get("status"))

    @staticmethod
    def mpi_waitsome():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_waitsome(correct_params.get("incount"),correct_params.get("array_of_requests"),correct_params.get("outcount"),correct_params.get("array_of_indices"),correct_params.get("array_of_statuses"))

    @staticmethod
    def mpi_win_allocate():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_allocate(correct_params.get("size"),correct_params.get("disp_unit"),correct_params.get("info"),correct_params.get("comm"),correct_params.get("baseptr"),correct_params.get("win"))

    @staticmethod
    def mpi_win_allocate_shared():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_allocate_shared(correct_params.get("size"),correct_params.get("disp_unit"),correct_params.get("info"),correct_params.get("comm"),correct_params.get("baseptr"),correct_params.get("win"))

    @staticmethod
    def mpi_win_attach():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_attach(correct_params.get("win"),correct_params.get("base"),correct_params.get("size"))

    @staticmethod
    def mpi_win_c2f():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_c2f(correct_params.get("win"))

    @staticmethod
    def mpi_win_call_errhandler():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_call_errhandler(correct_params.get("win"),correct_params.get("errorcode"))

    @staticmethod
    def mpi_win_complete():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_complete(correct_params.get("win"))

    @staticmethod
    def mpi_win_copy_attr_function():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_copy_attr_function(correct_params.get("oldwin"),correct_params.get("win_keyval"),correct_params.get("extra_state"),correct_params.get("attribute_val_in"),correct_params.get("attribute_val_out"),correct_params.get("flag"))

    @staticmethod
    def mpi_win_create():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_create(correct_params.get("base"),correct_params.get("size"),correct_params.get("disp_unit"),correct_params.get("info"),correct_params.get("comm"),correct_params.get("win"))

    @staticmethod
    def mpi_win_create_dynamic():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_create_dynamic(correct_params.get("info"),correct_params.get("comm"),correct_params.get("win"))

    @staticmethod
    def mpi_win_create_errhandler():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_create_errhandler(correct_params.get("win_errhandler_fn"),correct_params.get("errhandler"))

    @staticmethod
    def mpi_win_create_keyval():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_create_keyval(correct_params.get("win_copy_attr_fn"),correct_params.get("win_delete_attr_fn"),correct_params.get("win_keyval"),correct_params.get("extra_state"))

    @staticmethod
    def mpi_win_delete_attr():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_delete_attr(correct_params.get("win"),correct_params.get("win_keyval"))

    @staticmethod
    def mpi_win_delete_attr_function():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_delete_attr_function(correct_params.get("win"),correct_params.get("win_keyval"),correct_params.get("attribute_val"),correct_params.get("extra_state"))

    @staticmethod
    def mpi_win_detach():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_detach(correct_params.get("win"),correct_params.get("base"))

    @staticmethod
    def mpi_win_dup_fn():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_dup_fn(correct_params.get("oldwin"),correct_params.get("win_keyval"),correct_params.get("extra_state"),correct_params.get("attribute_val_in"),correct_params.get("attribute_val_out"),correct_params.get("flag"))

    @staticmethod
    def mpi_win_errhandler_function():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_errhandler_function(correct_params.get("win"),correct_params.get("error_code"),correct_params.get("varargs"))

    @staticmethod
    def mpi_win_f2c():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_f2c(correct_params.get("win"))

    @staticmethod
    def mpi_win_fence():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_fence(correct_params.get("assert"),correct_params.get("win"))

    @staticmethod
    def mpi_win_flush():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_flush(correct_params.get("rank"),correct_params.get("win"))

    @staticmethod
    def mpi_win_flush_all():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_flush_all(correct_params.get("win"))

    @staticmethod
    def mpi_win_flush_local():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_flush_local(correct_params.get("rank"),correct_params.get("win"))

    @staticmethod
    def mpi_win_flush_local_all():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_flush_local_all(correct_params.get("win"))

    @staticmethod
    def mpi_win_free():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_free(correct_params.get("win"))

    @staticmethod
    def mpi_win_free_keyval():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_free_keyval(correct_params.get("win_keyval"))

    @staticmethod
    def mpi_win_get_attr():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_get_attr(correct_params.get("win"),correct_params.get("win_keyval"),correct_params.get("attribute_val"),correct_params.get("flag"))

    @staticmethod
    def mpi_win_get_errhandler():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_get_errhandler(correct_params.get("win"),correct_params.get("errhandler"))

    @staticmethod
    def mpi_win_get_group():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_get_group(correct_params.get("win"),correct_params.get("group"))

    @staticmethod
    def mpi_win_get_info():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_get_info(correct_params.get("win"),correct_params.get("info_used"))

    @staticmethod
    def mpi_win_get_name():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_get_name(correct_params.get("win"),correct_params.get("win_name"),correct_params.get("resultlen"))

    @staticmethod
    def mpi_win_lock():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_lock(correct_params.get("lock_type"),correct_params.get("rank"),correct_params.get("assert"),correct_params.get("win"))

    @staticmethod
    def mpi_win_lock_all():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_lock_all(correct_params.get("assert"),correct_params.get("win"))

    @staticmethod
    def mpi_win_null_copy_fn():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_null_copy_fn(correct_params.get("oldwin"),correct_params.get("win_keyval"),correct_params.get("extra_state"),correct_params.get("attribute_val_in"),correct_params.get("attribute_val_out"),correct_params.get("flag"))

    @staticmethod
    def mpi_win_null_delete_fn():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_null_delete_fn(correct_params.get("win"),correct_params.get("win_keyval"),correct_params.get("attribute_val"),correct_params.get("extra_state"))

    @staticmethod
    def mpi_win_post():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_post(correct_params.get("group"),correct_params.get("assert"),correct_params.get("win"))

    @staticmethod
    def mpi_win_set_attr():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_set_attr(correct_params.get("win"),correct_params.get("win_keyval"),correct_params.get("attribute_val"))

    @staticmethod
    def mpi_win_set_errhandler():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_set_errhandler(correct_params.get("win"),correct_params.get("errhandler"))

    @staticmethod
    def mpi_win_set_info():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_set_info(correct_params.get("win"),correct_params.get("info"))

    @staticmethod
    def mpi_win_set_name():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_set_name(correct_params.get("win"),correct_params.get("win_name"))

    @staticmethod
    def mpi_win_shared_query():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_shared_query(correct_params.get("win"),correct_params.get("rank"),correct_params.get("size"),correct_params.get("disp_unit"),correct_params.get("baseptr"))

    @staticmethod
    def mpi_win_start():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_start(correct_params.get("group"),correct_params.get("assert"),correct_params.get("win"))

    @staticmethod
    def mpi_win_sync():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_sync(correct_params.get("win"))

    @staticmethod
    def mpi_win_test():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_test(correct_params.get("win"),correct_params.get("flag"))

    @staticmethod
    def mpi_win_unlock():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_unlock(correct_params.get("rank"),correct_params.get("win"))

    @staticmethod
    def mpi_win_unlock_all():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_unlock_all(correct_params.get("win"))

    @staticmethod
    def mpi_win_wait():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_win_wait(correct_params.get("win"))

    @staticmethod
    def mpi_wtick():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_wtick()

    @staticmethod
    def mpi_wtime():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().mpi_wtime()
