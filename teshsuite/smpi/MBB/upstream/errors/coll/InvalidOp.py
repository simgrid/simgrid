#! /usr/bin/python3
from Infrastructure.Variables import *

from Infrastructure.ErrorGenerator import ErrorGenerator
from Infrastructure.TemplateFactory import get_collective_template


class InvalidComErrorColl(ErrorGenerator):
    invalid_op = ["MPI_OP_NULL"]
    functions_to_use = ["mpi_reduce", "mpi_ireduce", "mpi_allreduce", "mpi_iallreduce"]

    # TODO invalid op+ type combinations aka MPI_MAXLOC with MPI_BYTE or something klie this

    def __init__(self):
        pass

    def get_feature(self):
        return ["COLL"]

    def generate(self, generate_level, real_world_score_table):
        for op_to_use in self.invalid_op:
            for func_to_use in self.functions_to_use:
                tm = get_collective_template(func_to_use)
                tm.set_description("InvalidParam-Op-" + func_to_use, "Invalid operator: %s" % op_to_use)

                for call in tm.get_instruction("MPICALL", return_list=True):
                    call.set_arg("op", op_to_use)
                    call.set_has_error()
                yield tm

                if generate_level <= BASIC_TEST_LEVEL:
                    return
