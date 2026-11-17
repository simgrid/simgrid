#! /usr/bin/python3
from __future__ import annotations
from Infrastructure.Variables import *

from Infrastructure.ErrorGenerator import ErrorGenerator
from Infrastructure.Instruction import Instruction

from Infrastructure.MPICallFactory import CorrectMPICallFactory
from Infrastructure.TemplateFactory import get_invalid_param_rma_case


class InvalidRankErrorRMA(ErrorGenerator):
    invalid_ranks = ["-1", "nprocs"]
    correct_ranks = ["MPI_PROC_NULL"]

    def __init__(self):
        pass

    def get_feature(self):
        return ["RMA"]

    def generate(self, generate_level, real_world_score_table):
        rma_funcs = []
        if generate_level <= BASIC_TEST_LEVEL:
            rma_funcs = ["mpi_get"]
        else:
            rma_funcs = ["mpi_get", "mpi_rget", "mpi_put", "mpi_rput", "mpi_accumulate", "mpi_raccumulate",
                 "mpi_get_accumulate", "mpi_rget_accumulate", "mpi_fetch_and_op", "mpi_compare_and_swap"]

        for func in rma_funcs:
            for rank_to_use in self.invalid_ranks:
                tm = get_invalid_param_rma_case("target_rank", func, rank_to_use)
                tm.get_instruction("RMAOP").set_has_error()
                tm.set_description("InvalidParam-Rank",
                                "Invalid Rank: %s" % rank_to_use)
                yield tm
            for rank_to_use in self.correct_ranks:
                tm = get_invalid_param_rma_case("target_rank", func, rank_to_use)
                tm.set_description("Correct-Rank",
                                f"Use of special rank {rank_to_use} in valid communication call")
                yield tm
