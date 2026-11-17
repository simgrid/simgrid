#! /usr/bin/python3
from Infrastructure.Variables import *

from Infrastructure.ErrorGenerator import ErrorGenerator
from Infrastructure.Instruction import Instruction
from Infrastructure.MPICallFactory import MPICallFactory, CorrectMPICallFactory
from Infrastructure.CorrectParameter import CorrectParameterFactory, get_matching_recv
from Infrastructure.Template import TemplateManager
from Infrastructure.TemplateFactory import get_send_recv_template, get_collective_template, \
    get_two_collective_template


class InvalidRankErrorColl(ErrorGenerator):
    functions_to_use = ["mpi_allgather", "mpi_allreduce", "mpi_alltoall", "mpi_barrier", "mpi_bcast", "mpi_reduce",
                        "mpi_scatter", "mpi_exscan", "mpi_gather", "mpi_reduce_scatter_block", "mpi_scan",
                        "mpi_ibarrier", "mpi_iallreduce", "mpi_ialltoall", "mpi_ibcast", "mpi_ireduce", "mpi_iscatter",
                        "mpi_igather", "mpi_iscan"]
    functions_not_supported_yet = ["mpi_allgatherv", "mpi_alltoallv", "mpi_alltoallw", "mpi_gatherv",
                                   "mpi_reduce_scatter", "mpi_scatterv"]

    # need_buf_funcs = ["mpi_bcast", "mpi_ibcast", "mpi_reduce", "mpi_ireduce", "mpi_exscan", "mpi_scan", "mpi_iscan", "mpi_gather", "mpi_igather", "mpi_allgather", "mpi_iallgather", "mpi_allreduce", "mpi_iallreduce", "mpi_alltoall", "mpi_ialltoall", "mpi_scatter", "mpi_iscatter", "mpi_reduce_scatter_block"]

    def __init__(self):
        pass

    def get_feature(self):
        return ["COLL"]

    def generate(self, generate_level, real_world_score_table):

        for func_to_use in self.functions_to_use:
            tm = get_collective_template(func_to_use)

            tm.set_description("CallOrdering-unmatched-" + func_to_use, func_to_use + " is not called by all processes")

            for call in tm.get_instruction("MPICALL", return_list=True):
                call.set_rank_executing(0)  # do the same for wait function
                call.set_has_error()

            tm.set_can_deadlock()

            yield tm
            if generate_level <= BASIC_TEST_LEVEL:
                break
                # basic test level only tests each error once, no need to loop all functions

        for i, func1 in enumerate(self.functions_to_use):
            for func2 in self.functions_to_use[i:]:
                tm = get_two_collective_template(func1, func2)

                tm.set_description("CallOrdering-unmatched-" + func1 + "-" + func2,
                                   "Collective mismatch: " + func1 + " is matched with " + func2)

                for call in tm.get_instruction("MPICALL", return_list=True):
                    call.set_has_error()
                if func1 != func2:  # we want different functions
                    yield tm

                tm.set_can_deadlock()

            if generate_level <= BASIC_TEST_LEVEL:
                return
