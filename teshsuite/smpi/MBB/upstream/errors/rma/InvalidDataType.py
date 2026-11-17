#! /usr/bin/python3
from __future__ import annotations
from Infrastructure.Variables import *

from Infrastructure.ErrorGenerator import ErrorGenerator
from Infrastructure.Instruction import Instruction

from Infrastructure.MPICallFactory import CorrectMPICallFactory
from Infrastructure.TemplateFactory import get_invalid_param_rma_case


class InvalidDatatypeErrorRMA(ErrorGenerator):
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
            if getattr(CorrectMPICallFactory, func)().has_arg("target_datatype"):
                # Use MPI_DATATYPE_NULL for target_datatype, TODO: Is this really undefined?
                tm = get_invalid_param_rma_case("target_datatype", func, "MPI_DATATYPE_NULL")
                tm.get_instruction("RMAOP").set_has_error()
                tm.set_description("InvalidParam",
                                    "Invalid Datatype: MPI_DATATYPE_NULL")
                yield tm

                # # Use freed datatype
                tm = get_invalid_param_rma_case("target_datatype", func, "type")
                datatype_register_free = Instruction("MPI_Datatype type; MPI_Type_contiguous (2, MPI_INT, &type); MPI_Type_commit(&type);MPI_Type_free(&type);", rank=0)
                tm.get_instruction("RMAOP").set_has_error()
                tm.insert_instruction(before_instruction="RMAOP", new_instruction=datatype_register_free)
                tm.set_description("InvalidParam",
                                    "Invalid Datatype: Datatype is freed before it is actually used")
                yield tm

