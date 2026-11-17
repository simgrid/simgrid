#! /usr/bin/python3
from Infrastructure.Variables import *

from Infrastructure.ErrorGenerator import ErrorGenerator
from Infrastructure.TemplateFactory import get_collective_template


class InvalidRankErrorColl(ErrorGenerator):
    invalid_ranks = ["-1", "nprocs", "MPI_PROC_NULL"]
    functions_to_use = ["mpi_reduce", "mpi_bcast", "mpi_gather", "mpi_scatter", "mpi_ireduce", "mpi_ibcast",
                        "mpi_igather", "mpi_iscatter"]
    functions_not_supported_yet = ["mpi_gatherv", "mpi_scatterv", "mpi_igatherv", "mpi_iscatterv"]

    def __init__(self):
        pass

    def get_feature(self):
        return ["COLL"]

    def generate(self, generate_level, real_world_score_table):

        for func_to_use in self.functions_to_use:
            for rank_to_use in self.invalid_ranks:
                tm = get_collective_template(func_to_use)

                tm.set_description("InvalidParam-Rank-" + func_to_use, "Invalid Rank: %s" % rank_to_use)
                for call in tm.get_instruction("MPICALL", return_list=True):
                    call.set_arg("root", rank_to_use)
                    call.set_has_error()

                yield tm
                if generate_level <= BASIC_TEST_LEVEL:
                    return
