#! /usr/bin/python3
from Infrastructure.MPICallFactory import CorrectMPICallFactory
from Infrastructure.Variables import *

from Infrastructure.ErrorGenerator import ErrorGenerator
from Infrastructure.TemplateFactory import get_collective_template


class InvalidComErrorColl(ErrorGenerator):
    invalid_com = ["MPI_COMM_NULL", "NULL"]
    functions_to_use = ["mpi_allgather", "mpi_allreduce", "mpi_alltoall", "mpi_barrier", "mpi_bcast", "mpi_reduce",
                        "mpi_scatter", "mpi_exscan", "mpi_gather", "mpi_reduce_scatter_block", "mpi_scan",
                        "mpi_ibarrier", "mpi_iallreduce", "mpi_ialltoall", "mpi_ibcast", "mpi_ireduce", "mpi_iscatter",
                        "mpi_igather", "mpi_iscan", "mpi_cart_create"]
    functions_not_supported_yet = ["mpi_allgatherv", "mpi_alltoallv", "mpi_alltoallw", "mpi_gatherv",
                                   "mpi_reduce_scatter", "mpi_scatterv"]
    ####functions_to_use = ["mpi_allgather","mpi_allgatherv","mpi_allreduce","mpi_alltoall","mpi_alltoallv","mpi_alltoallw","mpi_barrier","mpi_bcast", "mpi_exscan","mpi_gather", "mpi_gatherv","mpi_reduce", "mpi_reduce_scatter", "mpi_reduce_scatter_block", "mpi_scan", "mpi_scatter", "mpi_scatterv", "mpi_ibarrier", "mpi_iallreduce", "mpi_ialltoall", "mpi_ibcast", "mpi_ireduce", "mpi_iscatter", "mpi_igather", "mpi_iscan"]
    topology_functions = ["mpi_cart_create"]

    def __init__(self):
        pass

    def get_feature(self):
        return ["COLL"]

    def generate(self, generate_level, real_world_score_table):
        for com_to_use in self.invalid_com:
            for func_to_use in self.functions_to_use:
                tm = get_collective_template(func_to_use)

                tm.set_description("InvalidParam-Comm-" + func_to_use, "Invalid communicator: %s" % com_to_use)
                for call in tm.get_instruction("MPICALL", return_list=True):
                    arg_to_replace = "comm" if call.has_arg("comm") else "comm_old"
                    call.set_arg(arg_to_replace, com_to_use)
                    call.set_has_error()

                yield tm
                if generate_level <= BASIC_TEST_LEVEL:
                    break

        for func_to_use in self.topology_functions:
            tm = get_collective_template(func_to_use)

            for com_to_use in ["MPI_COMM_NULL", "NULL", "MPI_COMM_WORLD"]:
                tm.set_description("InvalidParam-Comm-" + func_to_use + "-mpi_cart_get",
                                   "A function tries to get cartesian information of " + com_to_use)

                cart_get = CorrectMPICallFactory.mpi_cart_get()
                cart_get.set_arg("comm_cart", com_to_use)
                tm.register_instruction(cart_get)
                cart_get.set_has_error()
                comm_free = CorrectMPICallFactory().mpi_comm_free()
                comm_free.set_arg("comm", "&mpi_comm_0")
                tm.register_instruction(comm_free)
                yield tm

            if generate_level <= BASIC_TEST_LEVEL:
                return
