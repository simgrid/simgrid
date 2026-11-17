#! /usr/bin/python3
from copy import copy
from random import shuffle

from Infrastructure.AllocCall import AllocCall
from Infrastructure.ErrorGenerator import ErrorGenerator
from Infrastructure.Instruction import Instruction
from Infrastructure.MPICallFactory import MPICallFactory, CorrectMPICallFactory
from Infrastructure.CorrectParameter import CorrectParameterFactory
from Infrastructure.ScoingModule.ScoringTable import is_combination_important
from Infrastructure.Template import TemplateManager
from Infrastructure.TemplateFactory import get_send_recv_template, get_invalid_param_p2p_case, get_communicator, \
    get_intercomm, predefined_types, user_defined_types, predefined_mpi_dtype_consants, get_type_buffers, \
    get_bytes_size_for_type, get_buffer_for_type, get_buffer_for_usertype

from itertools import chain

from Infrastructure.Variables import *

# TODO refactoring into different file
# test if the tool chan deal with messages send over different communicators
predefined_comms = ["MPI_COMM_WORLD"]
comm_creators = ["mpi_comm_dup", "mpi_comm_dup_with_info", "mpi_comm_idup",
                 "mpi_comm_idup_with_info", "mpi_comm_create", "mpi_comm_create_group", "mpi_comm_split",
                 "mpi_comm_split_type", "mpi_comm_create_from_group"
                 ]
intercomms = ["mpi_intercomm_create", "mpi_intercomm_merge", "mpi_intercomm_create_from_groups"]


def get_local_missmatch(type_1, type_2, send_func, recv_func):
    tm = get_send_recv_template(send_func, recv_func)
    tm.set_description("LocalParameterMissmatch-Dtype-" + send_func,
                       "datatype missmatch: Buffer: " + type_1 + " MPI_Call: " + type_2)
    type_var_1, buf_name_1, type_var_2, buf_name_2 = get_type_buffers(tm, type_1, type_2, 10, 10)
    # local missmatch
    for call in tm.get_instruction(identifier="MPICALL", return_list=True):
        call.set_has_error()
        call.set_arg("buf", buf_name_1)
        call.set_arg("datatype", type_var_2)

    return tm


def get_correct_case(type_1, size_1, send_func, recv_func, comm):
    tm = get_send_recv_template(send_func, recv_func)
    tm.set_description("Correct-" + send_func,
                       "")
    comm_var_name = "MPI_COMM_WORLD"
    if comm in comm_creators:
        comm_var_name = get_communicator(comm, tm)

    if comm in intercomms:
        comm_var_name = get_intercomm(comm, tm)

    call = tm.get_instruction(identifier="MPICALL", rank_excuting=1)

    if type_1 in predefined_types:
        buf_alloc = get_buffer_for_type(type_1, size_1)
        tm.insert_instruction(buf_alloc, before_instruction=tm._instructions[0])
        buf_name = buf_alloc.get_name()
        type_var = type_1
    else:
        buf_name, type_var = get_buffer_for_usertype(type_1, tm, tm._instructions[0], size_1)

    call.set_arg("buf", buf_name)
    call.set_arg("datatype", type_var)
    call.set_arg("count", size_1)
    call.set_arg("comm", comm_var_name)
    call = tm.get_instruction(identifier="MPICALL", rank_excuting=0)
    call.set_arg("buf", buf_name)
    call.set_arg("datatype", type_var)
    call.set_arg("count", size_1)
    call.set_arg("comm", comm_var_name)

    return tm


def get_global_missmatch(type_1, type_2, size_1, size_2, send_func, recv_func, comm):
    tm = get_send_recv_template(send_func, recv_func)
    tm.set_description("GlobalParameterMissmatch-Dtype-" + send_func,
                       "datatype missmatch: Rank0: " + type_1 + " Rank1: " + type_2)
    comm_var_name = "MPI_COMM_WORLD"
    if comm in comm_creators:
        comm_var_name = get_communicator(comm, tm)

    if comm in intercomms:
        comm_var_name = get_intercomm(comm, tm)

    type_var_1, buf_name_1, type_var_2, buf_name_2 = get_type_buffers(tm, type_1, type_2, size_1, size_2)
    call = tm.get_instruction(identifier="MPICALL", rank_excuting=1)
    call.set_has_error()
    call.set_arg("buf", buf_name_1)
    call.set_arg("datatype", type_var_1)
    call.set_arg("count", size_1)
    call.set_arg("comm", comm_var_name)
    call = tm.get_instruction(identifier="MPICALL", rank_excuting=0)
    call.set_has_error()
    call.set_arg("buf", buf_name_2)
    call.set_arg("datatype", type_var_2)
    call.set_arg("count", size_2)
    call.set_arg("comm", comm_var_name)

    return tm


def is_combination_compatible(s, r):
    t1, send_func, c1 = s
    t2, recv_func, c2 = r

    if send_func in ["mpi_rsend", "mpi_irsend", "mpi_rsend_init"] and recv_func not in ["mpi_irecv",
                                                                                        "mpi_recv_init",
                                                                                        "mpi_precv_init"]:
        # leads to deadlock
        return False
    if send_func == "mpi_psend_init" and recv_func != "mpi_precv_init":
        # partitioned may only match with partitioned
        return False

    if t1 in predefined_types and t2 in predefined_types and predefined_mpi_dtype_consants[
        t1] == predefined_mpi_dtype_consants[t2] and not (t1 == "MPI_BYTE" or t2 == "MPI_BYTE"):
        # one type is just the alias of another, this is allowed
        # but BYTE may not be mixed with other types see standard section 3.3.1
        return False

    if (t1 == "MPI_INT" and t2 not in predefined_types or
            t2 == "MPI_INT" and t1 not in predefined_types):
        return False
        # our user defined types may actually match int, so wwe check against another type

    return c1 == c2 and t1 != t2


class DtypeMissmatch(ErrorGenerator):
    invalid_bufs = [CorrectParameterFactory().buf_var_name, "NULL"]
    send_funcs = ["mpi_send",
                  "mpi_isend", "mpi_ssend", "mpi_issend", "mpi_rsend", "mpi_irsend", "mpi_bsend", "mpi_ibsend",
                  "mpi_send_init", "mpi_ssend_init", "mpi_bsend_init", "mpi_rsend_init", "mpi_psend_init",
                  ]

    recv_funcs = ["mpi_recv", "mpi_irecv", "mpi_recv_init", "mpi_precv_init"]

    sendrecv_funcs = ["mpi_sendrecv", "mpi_sendrecv_replace"]

    def __init__(self):
        pass

    def get_feature(self):
        return ["P2P"]

    def generate(self, generate_level, real_world_score_table):

        # (type,func,comm)
        important_sends = []
        important_recvs = []  #
        for type in predefined_types + user_defined_types:
            for send_func in self.send_funcs:
                for comm in predefined_comms + comm_creators + intercomms:
                    important_sends.append((type, send_func, comm))
                    if generate_level == BASIC_TEST_LEVEL:
                        break

        for type in predefined_types + user_defined_types:
            for recv_func in self.recv_funcs:
                for comm in predefined_comms + comm_creators + intercomms:
                    important_recvs.append((type, recv_func, comm))
                    if generate_level == BASIC_TEST_LEVEL:
                        break

        # filter to only important ones
        if generate_level in REAL_WORLD_FILTERING_LEVELS:
            important_sends = [(t, f, c) for (t, f, c) in important_sends if
                               is_combination_important(real_world_score_table, f,
                                                        datatype=t.lower(),
                                                        communicator=c)]
            important_recvs = [(t, f, c) for (t, f, c) in important_recvs if
                               is_combination_important(real_world_score_table, f,
                                                        datatype=t.lower(),
                                                        communicator=c)]

        if generate_level == SUFFICIENT_REAL_WORLD_TEST_LEVEL or generate_level == SUFFICIENT_TEST_LEVEL:
            # include every important case once
            # but not all possible missmatches
            combinations_to_use = []
            # If there are still s values left, pair them with any r
            for s in important_sends:
                for r in important_recvs:
                    if is_combination_compatible(s, r):
                        combinations_to_use.append((s, r))
                        break
            # pair all leftover r's
            for r in important_recvs:
                if all(r != pair[0] for pair in combinations_to_use):  # Check if s is already paired
                    for s in important_sends:
                        if is_combination_compatible(s, r):
                            combinations_to_use.append((s, r))
                            break
        else:
            # all possible combinations
            combinations_to_use = [(s, r) for s in important_sends for r in important_recvs if
                                   is_combination_compatible(s, r)]

        # "re-format"
        combinations_to_use = [(t1, t2, s, r, c) for (t1, s, c), (t2, r, _) in combinations_to_use]

        if generate_level == BASIC_TEST_LEVEL:
            combinations_to_use = combinations_to_use[0:1]

        correct_types_checked = set()
        for type_1, type_2, send_func, recv_func, comm in combinations_to_use:

            # local missmatch with byte is allowed
            if not (type_1 == "MPI_BYTE" or  type_2 == "MPI_BYTE"):
                # local missmatch only for one communicator
                if comm == "MPI_COMM_WORLD":
                    yield get_local_missmatch(type_1, type_2, send_func, recv_func)

            # global missmatch: communicator is important
            yield get_global_missmatch(type_1, type_2, 1, 1, send_func, recv_func, comm)

            # global missmatch with size = sizeof(a)* sizeof(b) so that total size match both types
            yield get_global_missmatch(type_1, type_2, get_bytes_size_for_type(type_2),
                                       get_bytes_size_for_type(type_1), send_func, recv_func, comm)

            if (type_1, comm) not in correct_types_checked:
                correct_types_checked.add((type_1, comm))
                yield get_correct_case(type_1, 1, send_func, recv_func, comm)

        # TODO mrecv?
        # TODO sendrecv?
