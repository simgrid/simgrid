#! /usr/bin/python3
from Infrastructure.MPIAPIInfo.MPIAPICategories import mpi_scatter_collective_funcs
from Infrastructure.ScoingModule.ScoringTable import is_combination_important
from Infrastructure.Variables import *

from Infrastructure.ErrorGenerator import ErrorGenerator
from Infrastructure.MPICallFactory import MPICallFactory, CorrectMPICallFactory
from Infrastructure.TemplateFactory import get_collective_template, predefined_types, user_defined_types, \
    predefined_mpi_dtype_consants, get_type_buffers, get_bytes_size_for_type, get_communicator, get_intercomm, \
    get_buffer_for_type, get_buffer_for_usertype

# TODO refactor into different file
# test if the tool chan deal with messages send over different communicators
predefined_comms = ["MPI_COMM_WORLD"]
comm_creators = ["mpi_comm_dup", "mpi_comm_dup_with_info", "mpi_comm_idup",
                 "mpi_comm_idup_with_info", "mpi_comm_create", "mpi_comm_create_group", "mpi_comm_split",
                 "mpi_comm_split_type", "mpi_comm_create_from_group"
                 ]
intercomms = ["mpi_intercomm_create", "mpi_intercomm_merge", "mpi_intercomm_create_from_groups"]


def get_local_missmatch(type_1, type_2, func_to_use):
    tm = get_collective_template(func_to_use)
    tm.set_description("LocalParameterMissmatch-Type-" + func_to_use,
                       "Wrong datatype matching: %s vs %s" % (type_1, type_2))

    s = 1
    if func_to_use in mpi_scatter_collective_funcs:
        s = "nprocs"
    type_var_1, buf_name_1, type_var_2, buf_name_2 = get_type_buffers(tm, type_1, type_2, s, s)

    for call in tm.get_instruction("MPICALL", return_list=True):

        call.set_has_error()
        if call.has_arg("recvtype"):
            call.set_arg("recvtype", type_var_1)
            call.set_arg("sendtype", type_var_1)
        else:
            call.set_arg("datatype", type_var_1)
        if call.has_arg("recvbuf"):
            call.set_arg("recvbuf", buf_name_2)
            call.set_arg("sendbuf", buf_name_2)
        else:
            call.set_arg("buffer", buf_name_2)
    return tm


def get_global_missmatch(type_1, type_2, count_1, count_2, func_to_use, comm):
    tm = get_collective_template(func_to_use)
    comm_var_name = "MPI_COMM_WORLD"
    if comm in comm_creators:
        comm_var_name = get_communicator(comm, tm)

    if comm in intercomms:
        comm_var_name = get_intercomm(comm, tm)

    s1 = count_1
    s2 = count_2
    if func_to_use in mpi_scatter_collective_funcs:
        s1 = "nprocs * (" + str(count_1) + ")"
        s2 = "nprocs * (" + str(count_2) + ")"
    type_var_1, buf_name_1, type_var_2, buf_name_2 = get_type_buffers(tm, type_1, type_2, s1, s2)

    tm.set_description("GlobalParameterMissmatch-Type-" + func_to_use,
                       "Wrong datatype matching: %s vs %s" % (type_1, type_2))

    for call in tm.get_instruction("MPICALL", return_list=True):
        call.set_rank_executing(0)
        call.set_has_error()
        call.set_arg("comm", comm_var_name)
        if call.has_arg("recvbuf"):
            call.set_arg("recvbuf", buf_name_1)
            call.set_arg("sendbuf", buf_name_1)
        else:
            call.set_arg("buffer", buf_name_1)
        if call.has_arg("recvtype"):
            call.set_arg("recvtype", type_var_1)
            call.set_arg("sendtype", type_var_1)
        else:
            call.set_arg("datatype", type_var_1)
        if call.has_arg("recvcount"):
            call.set_arg("recvcount", count_1)
            call.set_arg("sendcount", count_1)
        else:
            call.set_arg("count", count_1)

        c = CorrectMPICallFactory.get(func_to_use)
        c.set_rank_executing('not0')
        call.set_has_error()
        c.set_arg("comm", comm_var_name)
        if c.has_arg("recvbuf"):
            c.set_arg("recvbuf", buf_name_2)
            c.set_arg("sendbuf", buf_name_2)
        else:
            c.set_arg("buffer", buf_name_2)
        if c.has_arg("recvtype"):
            c.set_arg("recvtype", type_var_2)
            c.set_arg("sendtype", type_var_2)
        else:
            c.set_arg("datatype", type_var_2)
        if c.has_arg("recvcount"):
            c.set_arg("recvcount", count_2)
            c.set_arg("sendcount", count_2)
        else:
            c.set_arg("count", count_2)

        tm.insert_instruction(c, after_instruction=call)

    return tm


def get_correct_case(type_1, count_1, func_to_use, comm):
    tm = get_collective_template(func_to_use)
    comm_var_name = "MPI_COMM_WORLD"
    if comm in comm_creators:
        comm_var_name = get_communicator(comm, tm)

    if comm in intercomms:
        comm_var_name = get_intercomm(comm, tm)

    if type_1 in predefined_types:
        buf_alloc = get_buffer_for_type(type_1, count_1)
        if func_to_use in mpi_scatter_collective_funcs:
            buf_alloc.set_num_elements("nprocs * (" + str(buf_alloc.get_num_elements()) + ")")

        tm.insert_instruction(buf_alloc, before_instruction=tm._instructions[0])
        buf_name = buf_alloc.get_name()
        type_var = type_1
    else:
        buf_name, type_var = get_buffer_for_usertype(type_1, tm, tm._instructions[0], count_1)

    tm.set_description("Correct-" + func_to_use, "")

    for call in tm.get_instruction("MPICALL", return_list=True):
        call.set_rank_executing(0)
        call.set_arg("comm", comm_var_name)
        if call.has_arg("recvbuf"):
            call.set_arg("recvbuf", buf_name)
            call.set_arg("sendbuf", "MPI_IN_PLACE")
        else:
            call.set_arg("buffer", buf_name)
        if call.has_arg("recvtype"):
            call.set_arg("recvtype", type_var)
            call.set_arg("sendtype", type_var)
        else:
            call.set_arg("datatype", type_var)
        if call.has_arg("recvcount"):
            call.set_arg("recvcount", count_1)
            call.set_arg("sendcount", count_1)
        else:
            call.set_arg("count", count_1)
        if call.has_arg("op"):
            if type_1 in ["MPI_FLOAT_INT", "MPI_DOUBLE_INT", "MPI_LONG_INT", "MPI_2INT", "MPI_SHORT_INT",
                          "MPI_LONG_DOUBLE_INT"]:
                call.set_arg("op", "MPI_MAXLOC")

        c = CorrectMPICallFactory.get(func_to_use)
        c.set_rank_executing('not0')
        c.set_arg("comm", comm_var_name)
        if c.has_arg("recvbuf"):
            c.set_arg("recvbuf", buf_name)
            c.set_arg("sendbuf", "MPI_IN_PLACE")
        else:
            c.set_arg("buffer", buf_name)
        if c.has_arg("recvtype"):
            c.set_arg("recvtype", type_var)
            c.set_arg("sendtype", type_var)
        else:
            c.set_arg("datatype", type_var)
        if c.has_arg("recvcount"):
            c.set_arg("recvcount", count_1)
            c.set_arg("sendcount", count_1)
        else:
            c.set_arg("count", count_1)
        if c.has_arg("op"):
            if type_1 in ["MPI_FLOAT_INT", "MPI_DOUBLE_INT", "MPI_LONG_INT", "MPI_2INT", "MPI_SHORT_INT",
                          "MPI_LONG_DOUBLE_INT"]:
                c.set_arg("op", "MPI_MAXLOC")

        tm.insert_instruction(c, after_instruction=call)

    return tm


def is_combination_compatible(a, b):
    t1, f1, c1 = a
    t2, f2, c2 = b
    if t1 in predefined_types and t2 in predefined_types and predefined_mpi_dtype_consants[t1] == \
            predefined_mpi_dtype_consants[t2] and not (t1 == "MPI_BYTE" or t2 == "MPI_BYTE"):
        # one type is just the alias of another, this is allowed
        # but BYTE may not be mixed with other types see standard section 3.3.1
        return False

    return t1 != t2 and c1 == c2 and f1 == f2


class InvalidComErrorColl(ErrorGenerator):
    functions_to_use = ["mpi_bcast", "mpi_ibcast", "mpi_reduce", "mpi_ireduce", "mpi_exscan", "mpi_scan", "mpi_iscan",
                        "mpi_gather", "mpi_igather", "mpi_allgather", "mpi_iallgather", "mpi_allreduce",
                        "mpi_iallreduce", "mpi_alltoall", "mpi_ialltoall", "mpi_scatter", "mpi_iscatter"]
    func_one_type_arg = ["mpi_bcast", "mpi_reduce", "mpi_exscan", "mpi_scan", "mpi_ibcast", "mpi_ireduce", "mpi_iscan",
                         "mpi_allreduce", "mpi_iallreduce"]
    functions_not_supported_yet = ["mpi_reduce_scatter_block", "mpi_allgatherv", "mpi_alltoallv", "mpi_alltoallw",
                                   "mpi_gatherv", "mpi_reduce_scatter", "mpi_scatterv"]

    def __init__(self):
        pass

    def get_feature(self):
        return ["COLL"]

    def generate(self, generate_level, real_world_score_table):

        types = predefined_types + user_defined_types

        important_cases = []

        for f in self.functions_to_use:
            for comm in predefined_comms + comm_creators + intercomms:
                for t1 in types:
                    if generate_level in REAL_WORLD_FILTERING_LEVELS and not is_combination_important(
                            real_world_score_table,
                            f, datatype=t1.lower(),
                            communicator=comm):
                        continue
                    important_cases.append((t1, f, comm))
                if generate_level == BASIC_TEST_LEVEL:
                    break

        if generate_level == SUFFICIENT_REAL_WORLD_TEST_LEVEL or generate_level == SUFFICIENT_TEST_LEVEL:
            # ensure each combination is used once but not all combinations of those
            combinations_to_use = []
            for a in important_cases:
                for b in important_cases:
                    if is_combination_compatible(a, b):
                        combinations_to_use.append((a, b))
                        break  # break inner loop: combine every a with one matching b
        else:
            combinations_to_use = [(a, b) for a in important_cases for b in important_cases if
                                   is_combination_compatible(a, b)]

        # "re-format"
        combinations_to_use = [(t1, t2, f, c) for (t1, f, c), (t2, _, _) in combinations_to_use]

        if generate_level == BASIC_TEST_LEVEL:
            combinations_to_use = combinations_to_use[0:1]

        correct_types_checked = set()
        for type_1, type_2, func_to_use, comm in combinations_to_use:
            if comm == "MPI_COMM_WORLD":
                yield get_local_missmatch(type_1, type_2, func_to_use)

            yield get_global_missmatch(type_1, type_2, 1, 1, func_to_use, comm)
            # missmatch with matching sizes
            yield get_global_missmatch(type_1, type_2, get_bytes_size_for_type(type_2),
                                       get_bytes_size_for_type(type_1),
                                       func_to_use, comm)

            if (type_1, comm) not in correct_types_checked:
                if not (type_1 in user_defined_types and func_to_use in ["mpi_reduce", "mpi_allreduce", "mpi_ireduce",
                                                                         "mpi_iallreduce", "mpi_reduce_init",
                                                                         "mpi_allreduce_init"]):
                    # user defined type needs user defined recuction
                    correct_types_checked.add((type_1, comm))
                    yield get_correct_case(type_1, 1, func_to_use, comm)
