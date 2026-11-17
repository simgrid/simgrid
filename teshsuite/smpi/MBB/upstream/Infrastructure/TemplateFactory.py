#! /usr/bin/python3
from __future__ import annotations

import typing
from typing import List, Tuple

from Infrastructure.AllocCall import AllocCall
from Infrastructure.CorrectParameter import CorrectParameterFactory
from Infrastructure.Instruction import Instruction
from Infrastructure.MPIAPIInfo.MPIAPICategories import mpi_scatter_collective_funcs
from Infrastructure.MPICall import MPICall
from Infrastructure.MPICallFactory import CorrectMPICallFactory, MPICallFactory
from Infrastructure.Template import TemplateManager

list_of_not_implemented_types = [
    # TODO add wrong usage of
    'MPI_PACKED',
    # min/maxloc reduction types
]

predefined_mpi_dtype_consants = {
    'MPI_INT': 'signed int',
    'MPI_DOUBLE': 'double',

    'MPI_CHAR': 'char',
    'MPI_SHORT': 'signed short int',
    'MPI_LONG': 'signed long int',
    'MPI_LONG_LONG_INT': 'signed long long int',
    'MPI_LONG_LONG': 'signed long long int',
    'MPI_SIGNED_CHAR': 'signed char',
    'MPI_UNSIGNED_CHAR': 'unsigned char',
    'MPI_UNSIGNED_SHORT': 'unsigned short int',
    'MPI_UNSIGNED': 'unsigned int',
    'MPI_UNSIGNED_LONG': 'unsigned long int',
    'MPI_UNSIGNED_LONG_LONG': 'unsigned long long int',
    'MPI_FLOAT': 'float',
    'MPI_LONG_DOUBLE': 'long double',
    'MPI_WCHAR': 'wchar_t',
    'MPI_C_BOOL': '_Bool',
    'MPI_INT8_T': 'int8_t',
    'MPI_INT16_T': 'int16_t',
    'MPI_INT32_T': 'int32_t',
    'MPI_INT64_T': 'int64_t',
    'MPI_UINT8_T': 'uint8_t',
    'MPI_UINT16_T': 'uint16_t',
    'MPI_UINT32_T': 'uint32_t',
    'MPI_UINT64_T': 'uint64_t',
    'MPI_C_COMPLEX': 'float _Complex',
    'MPI_C_FLOAT_COMPLEX': 'float _Complex',
    'MPI_C_DOUBLE_COMPLEX': 'double _Complex',
    'MPI_C_LONG_DOUBLE_COMPLEX': 'long double _Complex',
    'MPI_BYTE': 'char',
    'MPI_PACKED': 'char',
    # MPI 4
    'MPI_AINT': 'MPI_Aint',
    'MPI_COUNT': 'MPI_Count',
    'MPI_OFFSET': 'MPI_Offset',
    # predefined struct types
    'MPI_2INT': "struct {int a; int b;}",
    'MPI_FLOAT_INT': "struct {float a; int b;}",
    'MPI_DOUBLE_INT': "struct {double a; int b;}",
    'MPI_LONG_INT': "struct {long a; int b;}",
    'MPI_SHORT_INT': "struct {short a; int b;}",
    'MPI_LONG_DOUBLE_INT': "struct {long double a; int b;}"
}
predefined_types = list(predefined_mpi_dtype_consants.keys())

# TODO can you use MPI_Type_create_darray for p2p communication? it is only meant for file IO

user_defined_types = [
    "mpi_type_contiguous",
    "mpi_type_vector",
    "mpi_type_create_hvector",
    "mpi_type_indexed",
    "mpi_type_create_hindexed",
    "mpi_type_create_indexed_block",
    "mpi_type_create_hindexed_block",
    "mpi_type_create_struct",
    "mpi_type_create_subarray",
    "mpi_type_create_subarray"
]


def get_buffer_for_usertype(mpi_type_creator, tm, insert_before, size=100):
    if mpi_type_creator == "mpi_type_contiguous":
        result_type = tm.add_stack_variable("MPI_Datatype")
        call = MPICallFactory.mpi_type_contiguous(2, "MPI_INT", "&" + result_type)
        tm.insert_instruction(call, before_instruction=insert_before, before_first_of_list=True)
        alloc = AllocCall("int", str(size) + " * 2", "buf_" + mpi_type_creator.lower())
        tm.insert_instruction(alloc, before_instruction=insert_before, before_first_of_list=True)
    elif mpi_type_creator == "mpi_type_vector":
        result_type = tm.add_stack_variable("MPI_Datatype")
        call = MPICallFactory.mpi_type_vector(2, 2, 3, "MPI_INT", "&" + result_type)
        tm.insert_instruction(call, before_instruction=insert_before, before_first_of_list=True)
        alloc = AllocCall("int", str(size) + " * 2*3", "buf_" + mpi_type_creator.lower())
        tm.insert_instruction(alloc, before_instruction=insert_before, before_first_of_list=True)
    elif mpi_type_creator == "mpi_type_create_hvector":
        result_type = tm.add_stack_variable("MPI_Datatype")
        call = MPICallFactory.mpi_type_create_hvector(2, 2, str(4) + "* sizeof(int)", "MPI_INT", "&" + result_type)
        tm.insert_instruction(call, before_instruction=insert_before, before_first_of_list=True)
        alloc = AllocCall("int", str(size) + " * 2*4", "buf_" + mpi_type_creator.lower())
        tm.insert_instruction(alloc, before_instruction=insert_before, before_first_of_list=True)
    elif mpi_type_creator == "mpi_type_indexed":
        result_type = tm.add_stack_variable("MPI_Datatype")
        tm.insert_instruction(Instruction("int block_length_indexed[] = {3,2};"),
                              before_instruction=insert_before, before_first_of_list=True)
        tm.insert_instruction(Instruction("int displacements_indexed[] = {0,4};"),
                              before_instruction=insert_before, before_first_of_list=True)
        call = MPICallFactory.mpi_type_indexed(2, "block_length_indexed", "displacements_indexed", "MPI_INT",
                                               "&" + result_type)
        tm.insert_instruction(call, before_instruction=insert_before, before_first_of_list=True)
        alloc = AllocCall("int", str(size) + " * 6", "buf_" + mpi_type_creator.lower())
        tm.insert_instruction(alloc, before_instruction=insert_before, before_first_of_list=True)
    elif mpi_type_creator == "mpi_type_create_hindexed":
        result_type = tm.add_stack_variable("MPI_Datatype")
        tm.insert_instruction(Instruction("int block_length_hindexed[] = {3,2};"),
                              before_instruction=insert_before, before_first_of_list=True)
        tm.insert_instruction(Instruction("MPI_Aint displacements_hindexed[] = {0,5*sizeof(int)};"),
                              before_instruction=insert_before, before_first_of_list=True)
        call = MPICallFactory.mpi_type_create_hindexed(2, "block_length_hindexed", "displacements_hindexed", "MPI_INT",
                                                       "&" + result_type)
        tm.insert_instruction(call, before_instruction=insert_before, before_first_of_list=True)
        alloc = AllocCall("int", str(size) + " * 7", "buf_" + mpi_type_creator.lower())
        tm.insert_instruction(alloc, before_instruction=insert_before, before_first_of_list=True)
    elif mpi_type_creator == "mpi_type_create_indexed_block":
        result_type = tm.add_stack_variable("MPI_Datatype")
        tm.insert_instruction(Instruction("int displacements_indexed_block[] = {0,4};"),
                              before_instruction=insert_before, before_first_of_list=True)
        call = MPICallFactory.mpi_type_create_indexed_block(2, "3", "displacements_indexed_block", "MPI_INT",
                                                            "&" + result_type)
        tm.insert_instruction(call, before_instruction=insert_before, before_first_of_list=True)
        alloc = AllocCall("int", str(size) + " * 7", "buf_" + mpi_type_creator.lower())
        tm.insert_instruction(alloc, before_instruction=insert_before, before_first_of_list=True)
    elif mpi_type_creator == "mpi_type_create_hindexed_block":
        result_type = tm.add_stack_variable("MPI_Datatype")
        tm.insert_instruction(Instruction("MPI_Aint displacements_hindexed_block[] = {0,9*sizeof(int)};"),
                              before_instruction=insert_before, before_first_of_list=True)
        call = MPICallFactory.mpi_type_create_hindexed_block(2, "3", "displacements_hindexed_block", "MPI_INT",
                                                             "&" + result_type)
        tm.insert_instruction(call, before_instruction=insert_before, before_first_of_list=True)
        alloc = AllocCall("int", str(size) + " * 12", "buf_" + mpi_type_creator.lower())
        tm.insert_instruction(alloc, before_instruction=insert_before, before_first_of_list=True)
    elif mpi_type_creator == "mpi_type_create_struct":
        result_type = tm.add_stack_variable("MPI_Datatype")
        tm.insert_instruction(Instruction("int block_length_struct[] = {2,4};"),
                              before_instruction=insert_before, before_first_of_list=True)
        tm.insert_instruction(Instruction("MPI_Aint displacements_struct[] = {0,3*sizeof(int)};"),
                              before_instruction=insert_before, before_first_of_list=True)
        tm.insert_instruction(Instruction("MPI_Datatype dtypes_struct[] = {MPI_INT,MPI_INT};"),
                              before_instruction=insert_before, before_first_of_list=True)
        call = MPICallFactory.mpi_type_create_struct(2, "block_length_struct", "displacements_struct", "dtypes_struct",
                                                     "&" + result_type)
        tm.insert_instruction(call, before_instruction=insert_before, before_first_of_list=True)
        alloc = AllocCall("int", str(size) + " * 7", "buf_" + mpi_type_creator.lower())
        tm.insert_instruction(alloc, before_instruction=insert_before, before_first_of_list=True)
    elif mpi_type_creator == "mpi_type_create_subarray":
        result_type = tm.add_stack_variable("MPI_Datatype")
        tm.insert_instruction(Instruction("int sizes_subarray[] = {3,3};"),
                              before_instruction=insert_before, before_first_of_list=True)
        tm.insert_instruction(Instruction("int sub_sizes_subarray[] = {3,1};"),
                              before_instruction=insert_before, before_first_of_list=True)
        tm.insert_instruction(Instruction("int starts_subarray[] = {0,1};"),
                              before_instruction=insert_before, before_first_of_list=True)

        call = MPICallFactory.mpi_type_create_subarray(2, "sizes_subarray", "sub_sizes_subarray", "starts_subarray",
                                                       "MPI_ORDER_C", "MPI_INT",
                                                       "&" + result_type)
        tm.insert_instruction(call, before_instruction=insert_before, before_first_of_list=True)
        alloc = AllocCall("int", str(size) + " * 3*3", "buf_" + mpi_type_creator.lower())
        tm.insert_instruction(alloc, before_instruction=insert_before, before_first_of_list=True)
    else:
        # print(mpi_type_creator)
        assert False and "Not implemented"

    tm.insert_instruction(
        MPICallFactory.mpi_type_commit("&" + result_type), before_instruction=insert_before, before_first_of_list=True)

    tm.register_instruction(MPICallFactory.mpi_type_free("&" + result_type))

    return alloc.get_name(), result_type


def get_buffer_for_type(mpi_type, size=100):
    ctype = predefined_mpi_dtype_consants[mpi_type]
    return AllocCall(ctype, size, "buf_" + mpi_type.lower())


def get_bytes_size_for_type(mpi_type):
    if mpi_type in predefined_mpi_dtype_consants:
        return "sizeof(" + predefined_mpi_dtype_consants[mpi_type] + ")"

    if mpi_type == "mpi_type_contiguous":
        return "(2* sizeof(int))"
    if mpi_type == "mpi_type_vector":
        return "(2*3* sizeof(int))"
    if mpi_type == "mpi_type_create_hvector":
        return "(2*4* sizeof(int))"
    if mpi_type == "mpi_type_indexed":
        return "(6* sizeof(int))"
    if mpi_type == "mpi_type_create_hindexed":
        return "(7* sizeof(int))"
    if mpi_type == "mpi_type_create_indexed_block":
        return "(7* sizeof(int))"
    if mpi_type == "mpi_type_create_hindexed_block":
        return "(12* sizeof(int))"
    if mpi_type == "mpi_type_create_struct":
        return "(7* sizeof(int))"
    if mpi_type == "mpi_type_create_subarray":
        return "(3*3* sizeof(int))"

    print(mpi_type)
    assert False and "Not implemented"


def get_type_buffers(tm, type_1, type_2, size_1, size_2):
    if type_1 in predefined_types:
        type_1_variable = type_1
        buf_alloc_call = get_buffer_for_type(type_1, size_1)
        tm.insert_instruction(buf_alloc_call, before_instruction="MPICALL",
                              before_first_of_list=True)
        buf_name_1 = buf_alloc_call.get_name()
    else:
        assert type_1 in user_defined_types
        buf_name_1, type_1_variable = get_buffer_for_usertype(type_1, tm, "MPICALL", size_1)
    tm.register_instruction("free(" + buf_name_1 + ");")
    if type_2 in predefined_types:
        type_2_variable = type_2
        buf_alloc_call = get_buffer_for_type(type_2, size_2)
        tm.insert_instruction(buf_alloc_call, before_instruction="MPICALL",
                              before_first_of_list=True)
        buf_name_2 = buf_alloc_call.get_name()
    else:
        assert type_2 in user_defined_types
        buf_name_2, type_2_variable = get_buffer_for_usertype(type_2, tm, "MPICALL", size_2)
    tm.register_instruction("free(" + buf_name_2 + ");")
    return type_1_variable, buf_name_1, type_2_variable, buf_name_2




def get_default_template(mpi_func):
    """
    Contructs a default template for the given mpi function
    Returns:
        TemplateManager Initialized with a default template
        The function is contained in a block named MPICALL with seperate calls for rank 1 and 2)
    """

    pass


def get_send_recv_template(send_func: str = "mpi_isend", recv_func: str | typing.Tuple[str, str] = "mpi_irecv"):
    """
    Contructs a default template for the given mpi send recv function pair it contains a send from rank 1 to rank 0
    Returns:
        TemplateManager Initialized with a default template
        The function is contained in a block named MPICALL with seperate calls for rank 1 and 2)
    """

    # currently supported:

    # mprobe and mrecv combinations allowed
    probe_pairs = [["mpi_mprobe", "mpi_mrecv"], ["mpi_mprobe", "mpi_imrecv"], ["mpi_improbe", "mpi_mrecv"],
                   ["mpi_improbe", "mpi_imrecv"]]

    sendrecv_funcs = ["mpi_sendrecv", "mpi_sendrecv_replace"]

    persistent_send_funcs = ["mpi_send_init", "mpi_ssend_init", "mpi_bsend_init", "mpi_rsend_init", "mpi_psend_init"]
    persistent_recv_funcs = ["mpi_recv_init", "mpi_precv_init"]

    isend_funcs = ["mpi_isend", "mpi_issend", "mpi_irsend", "mpi_ibsend"]
    irecv_funcs = ["mpi_irecv"]

    assert (send_func in ["mpi_send", "mpi_ssend", "mpi_rsend", "mpi_bsend"]
            + sendrecv_funcs + isend_funcs + persistent_send_funcs)
    assert recv_func in ["mpi_recv"] + sendrecv_funcs + irecv_funcs + persistent_recv_funcs + probe_pairs

    if send_func in sendrecv_funcs or recv_func == sendrecv_funcs:
        assert recv_func == send_func
        # default template generation only supports if both use same mechanism

    if send_func in ["mpi_rsend", "mpi_irsend", "mpi_rsend_init"]:
        assert recv_func in ["mpi_irecv", "mpi_recv_init", "mpi_precv_init"]  # else: deadlock

    if send_func == "mpi_psend_init":
        # psend must match witth precv
        assert recv_func == "mpi_precv_init"

    tm = TemplateManager()
    cf = CorrectParameterFactory()

    tm.register_instruction(cf.get_buffer_alloc(), identifier="ALLOC")
    if send_func in sendrecv_funcs:
        # spilt send and recv buf
        alloc = cf.get_buffer_alloc()
        alloc.set_identifier("ALLOC")
        alloc.set_name("recv_buf")
        tm.register_instruction(alloc)
    if recv_func in probe_pairs:
        tm.add_stack_variable("MPI_Message")

    if send_func in ["mpi_bsend", "mpi_ibsend", "mpi_bsend_init"]:
        buf_size = "sizeof(int)*10 + MPI_BSEND_OVERHEAD"
        tm.register_instruction(AllocCall("char", buf_size, "mpi_buf"), identifier="ALLOC")
        tm.register_instruction(MPICallFactory().mpi_buffer_attach("mpi_buf", buf_size), identifier="ALLOC")

    if (send_func in isend_funcs + persistent_send_funcs or
            recv_func in persistent_recv_funcs + irecv_funcs + probe_pairs):
        tm.add_stack_variable("MPI_Request")
    if recv_func in probe_pairs:
        flag_name = tm.add_stack_variable("int")
    # end preperation of all local variables

    # before the send/recv block
    if send_func in ["mpi_rsend", "mpi_irsend", "mpi_rsend_init"]:
        # sender needs to wait until recv is started
        tm.register_instruction(CorrectMPICallFactory.mpi_barrier(), identifier="SYNC", rank_to_execute=1)

    # get the send and recv block
    recv_to_use = recv_func
    if recv_func in probe_pairs:
        recv_to_use = recv_func[0]

    s = CorrectMPICallFactory.get(send_func)
    r = CorrectMPICallFactory.get(recv_to_use)

    if send_func in sendrecv_funcs:
        # sending the second msg
        s.set_arg("source", "0")
        r.set_arg("dest", "1")
        if s.has_arg("recvbuf"):
            s.set_arg("recvbuf", "recv_buf")
        if r.has_arg("recvbuf"):
            r.set_arg("recvbuf", "recv_buf")

    s.set_identifier("MPICALL")
    s.set_rank_executing(1)
    r.set_identifier("MPICALL")
    r.set_rank_executing(0)
    tm.register_instruction(s)
    tm.register_instruction(r)

    if recv_func in probe_pairs:
        if recv_func in [["mpi_improbe", "mpi_mrecv"],
                         ["mpi_improbe", "mpi_imrecv"]]:
            tm.insert_instruction(Instruction("while (!" + flag_name + "){", rank=0, identifier="PROBELOOP"),
                                  before_instruction=r)
            # insertion before the improbe call
            tm.register_instruction("}", rank_to_execute=0, identifier="PROBELOOP")  # end while
        # the matched recv
        tm.register_instruction(CorrectMPICallFactory.get(recv_func[1]), rank_to_execute=0, identifier="MATCHEDRECEIVE")

    if send_func in persistent_send_funcs:
        tm.register_instruction(CorrectMPICallFactory.mpi_start(), rank_to_execute=1, identifier="START")
        if send_func == "mpi_psend_init":
            # the pready takes a Request NOt a request*
            tm.register_instruction(MPICallFactory.mpi_pready("0", cf.get("request")[1:]), rank_to_execute=1)
    if recv_func in persistent_recv_funcs:
        tm.register_instruction(CorrectMPICallFactory.mpi_start(), rank_to_execute=0, identifier="START")
        # parrived is not necessary

    # after send/recv

    if send_func in ["mpi_rsend", "mpi_irsend", "mpi_rsend_init"]:
        # barrier indicating recv has started
        tm.register_instruction(CorrectMPICallFactory.mpi_barrier(), rank_to_execute=0, identifier="SYNC")

    # wait for op to complete

    if send_func in isend_funcs + persistent_send_funcs:
        tm.register_instruction(CorrectMPICallFactory.mpi_wait(), rank_to_execute=1, identifier="WAIT")
    if recv_func in irecv_funcs + persistent_recv_funcs + [["mpi_mprobe", "mpi_imrecv"],
                                                           ["mpi_improbe", "mpi_imrecv"]]:
        tm.register_instruction(CorrectMPICallFactory.mpi_wait(), rank_to_execute=0, identifier="WAIT")

    # end MPI operation
    # cleanup
    if send_func in ["mpi_bsend", "mpi_ibsend", "mpi_bsend_init"]:
        tm.register_instruction("int freed_size;", identifier="FREE")
        tm.register_instruction(MPICallFactory.mpi_buffer_detach("mpi_buf", "&freed_size"), identifier="FREE")
        tm.register_instruction("free(mpi_buf);", identifier="FREE")

    tm.register_instruction(cf.get_buffer_free(), identifier="FREE")

    if send_func in sendrecv_funcs:
        # spilt send and recv buf
        tm.register_instruction("free(recv_buf);", identifier="FREE")

    if send_func in persistent_send_funcs:
        tm.register_instruction(CorrectMPICallFactory.mpi_request_free(), rank_to_execute=1,identifier="FREE")
    if  recv_func in persistent_recv_funcs:
        tm.register_instruction(CorrectMPICallFactory.mpi_request_free(), rank_to_execute=0, identifier="FREE")

    return tm



def get_rma_template(win_alloc_func: str = "mpi_win_allocate", rmaop_func: str = "mpi_get", sync_mode: str = "fence"):
    """
    Contructs a default template for RMA communication.
    Returns:
        TemplateManager Initialized with a default template
    """
    tm = TemplateManager()
    cf = CorrectParameterFactory()


    # local buffer allocation, can be used by calls from different ranks
    tm.register_instruction(AllocCall(
        cf.dtype[0], cf.buf_size, cf.buf_var_name, use_malloc=False, identifier="RMA_LOCALBUF_ALLOC", rank="all"))

    (win_declare, alloc_list, free_list) = get_allocated_window(win_alloc_func, cf.get("win"), cf.winbuf_var_name, "int", "10")
    tm.register_instruction(win_declare, identifier="RMA_WIN_DECLARE")
    tm.register_instruction(alloc_list, identifier="RMA_WIN_ALLOC")

    if sync_mode == "fence":
        tm.register_instruction(CorrectMPICallFactory.mpi_win_fence(), identifier="STARTRMAEPOCH")
    elif sync_mode == "winlockall":
        tm.register_instruction(CorrectMPICallFactory.mpi_win_lock_all(), identifier="STARTRMAEPOCH")
    elif sync_mode == "winlock":
        tm.register_instruction(CorrectMPICallFactory.mpi_win_lock(), identifier="STARTRMAEPOCH", rank_to_execute=0)

    (alloc_list, inst_rma, inst_req) = get_rma_call(tm, rmaop_func, 0, "RMAOP")
    tm.register_instruction(alloc_list, identifier="RMABUFALLOC")
    tm.register_instruction(inst_rma, identifier="RMAOP")
    if inst_req is not None:
        tm.register_instruction(inst_req, identifier="RMAOPFINISH")

    if sync_mode == "fence":
        tm.register_instruction(CorrectMPICallFactory.mpi_win_fence(), identifier="ENDRMAEPOCH")
    elif sync_mode == "winlockall":
        tm.register_instruction(CorrectMPICallFactory.mpi_win_unlock_all(), identifier="ENDRMAEPOCH")
    elif sync_mode == "winlock":
        tm.register_instruction(CorrectMPICallFactory.mpi_win_unlock(), identifier="ENDRMAEPOCH", rank_to_execute=0)
    # end MPI operation
    # cleanup
    tm.register_instruction(free_list, identifier="RMA_WIN_FREE")


    return tm





def get_invalid_param_p2p_case(param, value, check_receive, send_func, recv_func):
    tm = get_send_recv_template(send_func, recv_func)

    rank = 1
    if check_receive:
        rank = 0

    for call in tm.get_instruction(identifier="MPICALL", return_list=True):
        if call.get_rank_executing() == rank:
            assert call.has_arg(param)
            call.set_arg(param, value)
            call.set_has_error()

    return tm
def get_invalid_param_rma_case(param, rmaop_func, value):
    tm = get_rma_template(rmaop_func=rmaop_func)

    for call in tm.get_instruction(identifier="RMAOP", return_list=True):
        if call.get_rank_executing() == 0:
            assert call.has_arg(param)
            call.set_arg(param, value)

    return tm

def insert_probe(tm, probe_to_use, recv_call):
    probe_call = CorrectMPICallFactory.get(probe_to_use)
    probe_call.set_rank_executing(recv_call.get_rank_executing())

    if probe_to_use == "mpi_iprobe":
        flag_name = tm.add_stack_variable("int")  # the flag
        tm.insert_instruction(Instruction(flag_name + "=0;", rank=recv_call.get_rank_executing()),
                              before_instruction=recv_call)
        tm.insert_instruction(Instruction("while (!" + flag_name + "){", rank=recv_call.get_rank_executing()),
                              before_instruction=recv_call)
        probe_call.set_arg("flag", "&"+flag_name)
        tm.insert_instruction(probe_call, before_instruction=recv_call)
        tm.insert_instruction(Instruction("}", rank=recv_call.get_rank_executing()),
                              before_instruction=recv_call)  # end while
    else:
        tm.insert_instruction(probe_call, before_instruction=recv_call)

    return probe_call


def replace_wait(wait_call, tm, wait_func_to_use):
    assert wait_func_to_use in ["mpi_wait", "mpi_test", "mpi_waitall", "mpi_testall", "mpi_waitany", "mpi_testany",
                                "mpi_waitsome", "mpi_testsome"]
    if wait_func_to_use == "mpi_wait":
        return  # nothing to do
    if wait_func_to_use == "mpi_test":
        flag_name = tm.add_stack_variable("int")
        test_call = MPICallFactory.mpi_test(wait_call.get_arg("request"), "&" + flag_name, wait_call.get_arg("status"))
        test_call.set_rank_executing(wait_call.get_rank_executing())
        test_call.set_identifier(wait_call.get_identifier())
        tm.insert_instruction(Instruction("while (!" + flag_name + "){", rank=wait_call.get_rank_executing()),
                              before_instruction=wait_call)
        tm.insert_instruction(test_call, before_instruction=wait_call)  # insertion before the improbe call
        tm.insert_instruction(Instruction("}", rank=wait_call.get_rank_executing()), before_instruction="FREE",
                              )  # end while
        tm.remove_instruction(wait_call)
        return
    status_to_use = wait_call.get_arg("status")
    if status_to_use == "MPI_STATUS_IGNORE":
        status_to_use = "MPI_STATUSES_IGNORE"
    if wait_func_to_use == "mpi_waitall":
        test_call = MPICallFactory.mpi_waitall("1", wait_call.get_arg("request"), status_to_use)
        test_call.set_rank_executing(wait_call.get_rank_executing())
        test_call.set_identifier(wait_call.get_identifier())
        tm.insert_instruction(test_call, before_instruction=wait_call)  # insertion before the improbe call
        tm.remove_instruction(wait_call)
        return
    if wait_func_to_use == "mpi_testall":
        flag_name = tm.add_stack_variable("int")
        test_call = MPICallFactory.mpi_testall("1", wait_call.get_arg("request"), "&" + flag_name,
                                               status_to_use)
        test_call.set_rank_executing(wait_call.get_rank_executing())
        test_call.set_identifier(wait_call.get_identifier())
        tm.insert_instruction(Instruction("while (!" + flag_name + "){", rank=wait_call.get_rank_executing()),
                              before_instruction=wait_call)
        tm.insert_instruction(test_call, before_instruction=wait_call)  # insertion before the improbe call
        tm.insert_instruction(Instruction("}", rank=wait_call.get_rank_executing()),
                              before_instruction=wait_call)  # end while
        tm.remove_instruction(wait_call)
        return
    if wait_func_to_use == "mpi_waitany":
        idx_name = tm.add_stack_variable("int")
        test_call = MPICallFactory.mpi_waitany("1", wait_call.get_arg("request"), "&" + idx_name,
                                               wait_call.get_arg("status"))
        test_call.set_rank_executing(wait_call.get_rank_executing())
        test_call.set_identifier(wait_call.get_identifier())
        tm.insert_instruction(test_call, before_instruction=wait_call)  # insertion before the improbe call
        tm.remove_instruction(wait_call)
        return
    if wait_func_to_use == "mpi_testany":
        flag_name = tm.add_stack_variable("int")
        idx_name = tm.add_stack_variable("int")
        test_call = MPICallFactory.mpi_testany("1", wait_call.get_arg("request"), "&" + idx_name, "&" + flag_name,
                                               wait_call.get_arg("status"))
        test_call.set_rank_executing(wait_call.get_rank_executing())
        test_call.set_identifier(wait_call.get_identifier())
        tm.insert_instruction(Instruction("while (!" + flag_name + "){", rank=wait_call.get_rank_executing()),
                              before_instruction=wait_call)
        tm.insert_instruction(test_call, before_instruction=wait_call)  # insertion before the improbe call
        tm.insert_instruction(Instruction("}", rank=wait_call.get_rank_executing()),
                              before_instruction=wait_call)  # end while
        tm.remove_instruction(wait_call)
        return
    if wait_func_to_use == "mpi_waitsome":
        idx_name = tm.add_stack_variable("int")
        idx_array = tm.add_stack_variable("int")
        test_call = MPICallFactory.mpi_waitsome("1", wait_call.get_arg("request"), "&" + idx_name,
                                                "&" + idx_array, status_to_use)
        test_call.set_rank_executing(wait_call.get_rank_executing())
        test_call.set_identifier(wait_call.get_identifier())
        tm.insert_instruction(test_call, before_instruction=wait_call)  # insertion before the improbe call
        tm.remove_instruction(wait_call)
        return
    if wait_func_to_use == "mpi_testsome":
        flag_name = tm.add_stack_variable("int")
        idx_array = tm.add_stack_variable("int")
        test_call = MPICallFactory.mpi_testsome("1", wait_call.get_arg("request"), "&" + flag_name, "&" + idx_array,
                                                status_to_use)
        test_call.set_rank_executing(wait_call.get_rank_executing())
        test_call.set_identifier(wait_call.get_identifier())
        tm.insert_instruction(Instruction("while (!" + flag_name + "){", rank=wait_call.get_rank_executing()),
                              before_instruction=wait_call)
        tm.insert_instruction(test_call, before_instruction=wait_call)  # insertion before the improbe call
        tm.insert_instruction(Instruction("}", rank=wait_call.get_rank_executing()),
                              before_instruction=wait_call)  # end while
        tm.remove_instruction(wait_call)
        return
    assert False and "Not implemented"


def get_collective_template(collective_func):
    """
    Contructs a default template for the given mpi collective
    Returns:
        TemplateManager Initialized with a default template
        The function is contained in a block named MPICALL
        with seperate calls for rank 1 and 2 if seperate ==True
    """

    tm = TemplateManager()
    cf = CorrectParameterFactory()

    cmpicf = CorrectMPICallFactory()
    call_creator_function = getattr(cmpicf, collective_func)
    c = call_creator_function()

    if c.has_arg("buffer") or c.has_arg("sendbuf"): 
        alloc = cf.get_buffer_alloc()
        if collective_func in mpi_scatter_collective_funcs:
            alloc.set_num_elements("nprocs * ("+alloc.get_num_elements()+")")
        alloc.set_identifier("ALLOC")
        alloc.set_name("buf")
        tm.register_instruction(alloc)
    if c.has_arg("recvbuf"):
        recvalloc = cf.get_recvbuffer_alloc()
        recvalloc.set_identifier("ALLOC")
        recvalloc.set_name("recv_buf")
        tm.register_instruction(recvalloc)

    if c.has_arg("comm_cart"):
        tm.add_stack_variable("MPI_Comm")
        # TODO: create proper instructions
        tm.register_instruction(Instruction("int periods[2]={1,1};"), identifier="ALLOC")
        tm.register_instruction(Instruction("int dims[2]={0,0};"), identifier="ALLOC") # this is an initialization, we use dim_create function
        tm.register_instruction(Instruction("int coords[2]={0,0};"), identifier="ALLOC")
        # Get the dims of the cartesian topology MPI_Dims_create(nprocs,2,dims);
        dims_create = MPICallFactory.mpi_dims_create("nprocs", "2", "dims")
        tm.register_instruction(dims_create)
    
    # add request for nonblocking collectives
    if collective_func.startswith("mpi_i"): 
        tm.add_stack_variable("MPI_Request")


    coll = CorrectMPICallFactory.get(collective_func)
    coll.set_identifier("MPICALL")
    tm.register_instruction(coll)
    if coll.has_arg("recvbuf"):
            # TODO we need to find out if recvcount is larger!
            # not all functions have this
            coll.set_arg("recvbuf", "recv_buf")

    # add wait function for nonblocking collectives
    if collective_func.startswith("mpi_i"):
        tm.register_instruction(CorrectMPICallFactory.mpi_wait(), rank_to_execute='all', identifier="WAIT")

    if c.has_arg("buffer") or c.has_arg("sendbuf"):
        tm.register_instruction(cf.get_buffer_free(), identifier="FREE")
    if c.has_arg("recvbuf"):
        tm.register_instruction(cf.get_recvbuffer_free(), identifier="FREE")

    return tm

def get_two_collective_template(collective_func1, collective_func2):
    """
    Contructs a default template for two given mpi collectives
    Returns:
        TemplateManager Initialized with a default template
        The function is contained in a block named MPICALL
    """

    tm = TemplateManager()
    cf = CorrectParameterFactory()

    # todo: spilt send and recv buf
    alloc = cf.get_buffer_alloc()
    if collective_func1 in mpi_scatter_collective_funcs or collective_func2 in mpi_scatter_collective_funcs:
        alloc.set_num_elements("nprocs * (" + alloc.get_num_elements() + ")")
    alloc.set_identifier("ALLOC")
    alloc.set_name("buf")
    tm.register_instruction(alloc)
    recvalloc = cf.get_recvbuffer_alloc()
    recvalloc.set_identifier("ALLOC")
    recvalloc.set_name("recv_buf")
    tm.register_instruction(recvalloc)


    cmpicf = CorrectMPICallFactory()
    call_creator_function = getattr(cmpicf, collective_func1)
    c = call_creator_function()

    # add request for nonblocking collectives
    if collective_func1.startswith("mpi_i") or collective_func2.startswith("mpi_i"): 
        tm.add_stack_variable("MPI_Request")


    coll1 = CorrectMPICallFactory.get(collective_func1)
    coll1.set_identifier("MPICALL")
    tm.register_instruction(coll1)
    coll1.set_rank_executing(0)
    if coll1.has_arg("recvbuf"):
            coll1.set_arg("recvbuf", "recv_buf")

    coll2 = CorrectMPICallFactory.get(collective_func2)
    coll2.set_identifier("MPICALL")
    tm.register_instruction(coll2)
    coll2.set_rank_executing('not0')
    if coll2.has_arg("recvbuf"):
            coll2.set_arg("recvbuf", "recv_buf")

    # add wait function for nonblocking collectives
    if collective_func1.startswith("mpi_i") or collective_func2.startswith("mpi_i"):
        tm.register_instruction(CorrectMPICallFactory.mpi_wait(), rank_to_execute='all', identifier="WAIT")

    tm.register_instruction(cf.get_buffer_free(), identifier="FREE")
    tm.register_instruction(cf.get_recvbuffer_free(), identifier="FREE")

    return tm

def get_allocated_window(win_alloc_func, name, bufname, ctype, num_elements):
    """
    Constructs a window allocation using Win_allocate or Win_create and a corresponding free call.
    :param win_alloc_func: The window allocation to use (mpi_win_allocate or mpi_win_create).
    :param name: name of the window
    """
    identifier = "win_allocate"
    alloc_list = []
    free_list = []

    # declare window
    win_declare = Instruction(f"MPI_Win {name};", identifier=identifier)

    # extract C data type and window buffer name
    # dtype = CorrectParameterFactory().dtype[0]
    # winbuf_name = CorrectParameterFactory().winbuf_var_name
    # winbuf_size = CorrectParameterFactory().buf_size_bytes
    win_allocate_call = None

    if win_alloc_func == "mpi_win_allocate":
        # MPI allocate, only declaration of local buffer required
        alloc_list.append(Instruction(f"{ctype}* {bufname};", "all", identifier))
        win_allocate_call = CorrectMPICallFactory().mpi_win_allocate()
        win_allocate_call.set_arg("baseptr", "&" + bufname)
    elif win_alloc_func == "mpi_win_create":
        # allocate buffer for win_create
        alloc_list.append(AllocCall(ctype, num_elements, bufname))
        win_allocate_call = CorrectMPICallFactory().mpi_win_create()
        win_allocate_call.set_arg("base", bufname)
    else:
        assert False

    # set common parameters for both calls
    win_allocate_call.set_arg("win", "&" + name)

    buf_size_bytes = num_elements + "*sizeof(" + ctype + ")"
    win_allocate_call.set_arg("size", buf_size_bytes)

    win_allocate_call.set_arg("disp_unit", f"sizeof({ctype})")
    alloc_list.append(win_allocate_call)

    win_free_call = CorrectMPICallFactory().mpi_win_free()
    win_free_call.set_arg("win", "&" + name)
    free_list.append(win_free_call)

    return (win_declare, alloc_list, free_list)


def get_rma_call(tm: TemplateManager, rma_func, rank, identifier="RMACall") -> Tuple[List[Instruction],Instruction, Instruction | None]:
    # some RMA ops require buffer for result_addr and compare_addr
    additional_alloc_list: List[Instruction] = []
    # instruction required to finish RMA call (for request-based RMA, wait for request)
    inst_rma_req_wait = None

    cf = CorrectParameterFactory()
    cfmpi = CorrectMPICallFactory()

    rma_call = getattr(cfmpi, rma_func)()
    rma_call.set_identifier(identifier)
    rma_call.set_rank_executing(rank)
    rma_call.set_arg("target_rank", "1")

    # request-based RMA call, add request
    if rma_call.has_arg("request"):
        req = tm.add_stack_variable("MPI_Request")
        rma_call.set_arg("request", "&" + req)
        inst_rma_req_wait = Instruction(f"MPI_Wait(&{req}, MPI_STATUS_IGNORE);", rank=rank)

    # some RMA ops require result_addr
    if rma_call.has_arg("result_addr"):
        result_addr = tm.add_stack_variable("int")
        result_addr_alloc = AllocCall(cf.dtype[0], cf.buf_size, result_addr, rank=rank, identifier=identifier)
        rma_call.set_arg("result_addr", result_addr)
        additional_alloc_list.append(result_addr_alloc)
        

    # some RMA ops require compare_addr
    if rma_call.has_arg("compare_addr"):
        compare_addr = tm.add_stack_variable("int")
        compare_addr_alloc = AllocCall(cf.dtype[0], cf.buf_size, compare_addr, rank=rank)
        rma_call.set_arg("compare_addr", compare_addr)
        additional_alloc_list.append(compare_addr_alloc)

    return (additional_alloc_list, rma_call, inst_rma_req_wait)



def get_communicator(comm_create_func: str, tm: TemplateManager, before_idx: int = 0, identifier: str = "COMM"):
    """
    :param comm_create_func: teh function used to create the new communicator
    :param name: name of the communicator variable
    Inserts operations with the given identifier before the given index
    :return: name of result variable
    """
    assert comm_create_func in ["mpi_comm_dup", "mpi_comm_dup_with_info", "mpi_comm_idup",
                                "mpi_comm_idup_with_info", "mpi_comm_create", "mpi_comm_create_group", "mpi_comm_split",
                                "mpi_comm_split_type", "mpi_comm_create_from_group"]
    before_inst = tm.get_instruction(idx=before_idx)
    newcomm = tm.add_stack_variable("MPI_Comm")
    instr_list=[]
    if comm_create_func.startswith("mpi_comm_i"):
        req_name = tm.add_stack_variable("MPI_Request")
    if comm_create_func in ["mpi_comm_create", "mpi_comm_create_group","mpi_comm_create_from_group"]:
        group_variable = tm.add_stack_variable("MPI_Group")
        group = CorrectMPICallFactory.mpi_comm_group()
        group.set_identifier(identifier)
        group.set_arg("group", "&" + group_variable)
        tm.insert_instruction(group, before_instruction=before_inst)

    call = CorrectMPICallFactory.get(comm_create_func)
    call.set_arg("newcomm", "&" + newcomm)
    if comm_create_func.startswith("mpi_comm_i"):
        call.set_arg("request", "&" + req_name)
    if comm_create_func in ["mpi_comm_create", "mpi_comm_create_group","mpi_comm_create_from_group"]:
        call.set_arg("group", group_variable)  # not &group
    call.set_identifier(identifier)
    tm.insert_instruction(call, before_instruction=before_inst)
    if comm_create_func.startswith("mpi_comm_i"):
        wait = MPICallFactory.mpi_wait("&" + req_name, "MPI_STATUS_IGNORE")
        wait.set_identifier(identifier)
        tm.insert_instruction(wait, before_instruction=before_inst)

    comm_free = MPICallFactory.mpi_comm_free("&"+newcomm)
    comm_free.set_identifier(identifier)
    tm.register_instruction(comm_free)

    if comm_create_func in ["mpi_comm_create", "mpi_comm_create_group","mpi_comm_create_from_group"]:
        group_free = CorrectMPICallFactory.mpi_group_free()
        group_free.set_arg("group", "&" + group_variable)
        group_free.set_identifier(identifier)
        tm.insert_instruction(group_free, before_instruction=before_inst)
    return newcomm


def get_intercomm(comm_create_func: str, tm: TemplateManager, before_idx: int = 0, identifier: str = "COMM"):
    """
       Inserts operations with the given identifier before the given index
    :return: name of result variable
    """
    before_inst = tm.get_instruction(idx=before_idx)
    assert comm_create_func in ["mpi_intercomm_create", "mpi_intercomm_create_from_groups", "mpi_intercomm_merge"]

    if comm_create_func == "mpi_intercomm_create":
        base_comm = tm.add_stack_variable("MPI_Comm")
        intercomm = tm.add_stack_variable("MPI_Comm")
        call = MPICallFactory.mpi_comm_split("MPI_COMM_WORLD", "rank % 2", "rank", "&" + base_comm)
        call.set_identifier(identifier)
        tm.insert_instruction(call, before_instruction=before_inst)
        call = MPICallFactory.mpi_intercomm_create(base_comm, "0", "MPI_COMM_WORLD", "!(rank %2)",
                                                   CorrectParameterFactory().get("tag"), "&" + intercomm)
        call.set_identifier(identifier)
        tm.insert_instruction(call, before_instruction=before_inst)
        call = MPICallFactory.mpi_comm_free("&" + base_comm)
        call.set_identifier(identifier)
        tm.insert_instruction(call, before_instruction=before_inst)
        comm_free = MPICallFactory.mpi_comm_free("&" + intercomm)
        comm_free.set_identifier(identifier)
        tm.register_instruction(comm_free)
        return intercomm
    if comm_create_func == "mpi_intercomm_create_from_groups":
        intercomm = tm.add_stack_variable("MPI_Comm")
        world_group = tm.add_stack_variable("MPI_Group")
        even_group = tm.add_stack_variable("MPI_Group")
        odd_group = tm.add_stack_variable("MPI_Group")
        call = MPICallFactory.mpi_comm_group("MPI_COMM_WORLD", "&" + world_group)
        call.set_identifier(identifier)
        tm.insert_instruction(call, before_instruction=before_inst)
        inst = Instruction("int triplet[3];"
                           "triplet[0] =0;"
                           "triplet[1] =nprocs - 1;"
                           "triplet[2] =2;", identifier=identifier)
        tm.insert_instruction(inst, before_instruction=before_inst)
        call = MPICallFactory.mpi_group_range_incl(world_group, "1", "&triplet", "&" +even_group)
        call.set_identifier(identifier)
        tm.insert_instruction(call, before_instruction=before_inst)
        inst = Instruction("triplet[0] =1;", identifier=identifier)
        tm.insert_instruction(inst, before_instruction=before_inst)
        call = MPICallFactory.mpi_group_range_incl(world_group, "1", "&triplet", "&" +odd_group)
        call.set_identifier(identifier)
        tm.insert_instruction(call, before_instruction=before_inst)

        call = MPICallFactory.mpi_intercomm_create_from_groups("(rank % 2 ? " + even_group + ":" + odd_group + ")",
                                                               "0",
                                                               "(!(rank % 2) ? " + even_group + ":" + odd_group + ")",
                                                               "0",
                                                               CorrectParameterFactory().get("stringtag"),
                                                               CorrectParameterFactory().get("INFO"),
                                                               CorrectParameterFactory().get("errhandler"),
                                                               "&" + intercomm)
        call.set_identifier(identifier)
        tm.insert_instruction(call, before_instruction=before_inst)
        comm_free = MPICallFactory.mpi_comm_free("&" + intercomm)
        comm_free.set_identifier(identifier)
        tm.register_instruction(comm_free)
        return intercomm

    if comm_create_func == "mpi_intercomm_merge":
        intercomm_base_comm = tm.add_stack_variable("MPI_Comm")
        to_merge_intercomm = tm.add_stack_variable("MPI_Comm")
        result_comm = tm.add_stack_variable("MPI_Comm")
        call = MPICallFactory.mpi_comm_split("MPI_COMM_WORLD", "rank % 2", "rank", "&" + intercomm_base_comm)
        call.set_identifier(identifier)
        tm.insert_instruction(call, before_instruction=before_inst)
        call = MPICallFactory.mpi_intercomm_create(intercomm_base_comm, "0", "MPI_COMM_WORLD", "!(rank %2)",
                                                   CorrectParameterFactory().get("tag"), "&" + to_merge_intercomm)
        call.set_identifier(identifier)
        tm.insert_instruction(call, before_instruction=before_inst)
        call = MPICallFactory.mpi_intercomm_merge(to_merge_intercomm, "rank %2", "&" + result_comm)
        call.set_identifier(identifier)
        tm.insert_instruction(call, before_instruction=before_inst)
        call = MPICallFactory.mpi_comm_free("&" + to_merge_intercomm)
        call.set_identifier(identifier)
        tm.insert_instruction(call, before_instruction=before_inst)
        call = MPICallFactory.mpi_comm_free("&" + intercomm_base_comm)
        call.set_identifier(identifier)
        tm.insert_instruction(call, before_instruction=before_inst)
        comm_free = MPICallFactory.mpi_comm_free("&" + result_comm)
        comm_free.set_identifier(identifier)
        tm.register_instruction(comm_free)
        return result_comm

    return None


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
