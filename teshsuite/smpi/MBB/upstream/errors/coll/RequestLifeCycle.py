#! /usr/bin/python3

from Infrastructure.ErrorGenerator import ErrorGenerator
from Infrastructure.Instruction import Instruction
from Infrastructure.MPICallFactory import MPICallFactory, CorrectMPICallFactory
from Infrastructure.CorrectParameter import CorrectParameterFactory, get_matching_recv
from Infrastructure.Template import TemplateManager
from Infrastructure.TemplateFactory import get_send_recv_template, get_collective_template
from Infrastructure.Variables import *


class CorrectColl(ErrorGenerator):
    nbfunc_to_use = ["mpi_ibarrier", "mpi_iallreduce", "mpi_ialltoall", "mpi_ibcast", "mpi_ireduce", "mpi_iscatter",
                     "mpi_igather", "mpi_iscan"]
    functions_not_supported_yet = ["mpi_igatherv", "mpi_iscatterv"]

    def __init__(self):
        pass

    def get_feature(self):
        return ["COLL"]

    def generate(self, generate_level, real_world_score_table):

        for func_to_use in self.nbfunc_to_use:
            tm = get_collective_template(func_to_use)

            tm.set_description("RequestLifeCycle-" + func_to_use,
                               func_to_use + " is not associated with a completion operation (missing wait)")

            for call in tm.get_instruction("MPICALL", return_list=True):
                wait = tm.get_instruction("WAIT", return_list=True)
                tm.remove_instruction(instruction=wait)
                call.set_has_error()

            yield tm

            if generate_level >= BASIC_TEST_LEVEL:
                return
