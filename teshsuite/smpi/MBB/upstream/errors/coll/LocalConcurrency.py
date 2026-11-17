#! /usr/bin/python3

from Infrastructure.ErrorGenerator import ErrorGenerator
from Infrastructure.Instruction import Instruction
from Infrastructure.MPICallFactory import MPICallFactory, CorrectMPICallFactory
from Infrastructure.CorrectParameter import CorrectParameterFactory, get_matching_recv
from Infrastructure.Template import TemplateManager
from Infrastructure.TemplateFactory import get_collective_template
from Infrastructure.Variables import *


class LocalConcurrencyErrorColl(ErrorGenerator):
    nbfunc_to_use = ["mpi_iallreduce", "mpi_ialltoall", "mpi_ibcast", "mpi_ireduce", "mpi_iscatter", "mpi_igather",
                     "mpi_iscan"]

    def __init__(self):
        pass

    def get_feature(self):
        return ["COLL"]

    def generate(self, generate_level, real_world_score_table):

        for func_to_use in self.nbfunc_to_use:
            tm = get_collective_template(func_to_use)

            tm.set_description("LocalConcurrency-" + func_to_use, "Usage of buffer before operation is completed")

            conflicting_inst = Instruction("buf[2]=1;")
            conflicting_inst.set_has_error()
            wait = tm.get_instruction("WAIT", return_list=True)
            tm.insert_instruction(conflicting_inst, before_instruction=wait)

            yield tm

            if generate_level <= BASIC_TEST_LEVEL:
                return
