#! /usr/bin/python3
from __future__ import annotations
from Infrastructure.Variables import *

from Infrastructure.ErrorGenerator import ErrorGenerator
from Infrastructure.Instruction import Instruction

from Infrastructure.MPICallFactory import CorrectMPICallFactory
from Infrastructure.TemplateFactory import get_invalid_param_rma_case, get_rma_template
from Infrastructure.MPICall import MPICall


class InvalidBufferErrorRMA(ErrorGenerator):
    def __init__(self):
        pass

    def get_feature(self):
        return ["RMA"]

    def generate(self, generate_level, real_world_score_table):
        if generate_level <= BASIC_TEST_LEVEL:
            rma_funcs = ["mpi_get"]
        else:
            rma_funcs = ["mpi_get", "mpi_rget", "mpi_put", "mpi_rput", "mpi_accumulate", "mpi_raccumulate",
                 "mpi_get_accumulate", "mpi_rget_accumulate", "mpi_fetch_and_op", "mpi_compare_and_swap"]
        # go through alloc functions (Win_alloc, Win_create) and set NULL
        for alloc_call in ["mpi_win_allocate", "mpi_win_create"]:
            tm = get_rma_template(win_alloc_func=alloc_call)
            for call in tm.get_instruction(identifier="RMA_WIN_ALLOC", return_list=True):
                for buffer_arg in ["base", "baseptr"]:
                    if isinstance(call, MPICall) and call.has_arg(buffer_arg):
                        call.set_arg(buffer_arg, "NULL")
                        call.set_has_error() 
                        tm.set_description("InvalidParam",
                                "Invalid Buffer in " + call.get_function())
                        yield tm

        # go through RMA op buffers and set NULL
        for func in rma_funcs:
            tm = get_invalid_param_rma_case("origin_addr", func, "NULL")
            tm.get_instruction("RMAOP").set_has_error()
            tm.set_description("InvalidParam",
                    "Invalid Buffer in " + func)
            yield tm

