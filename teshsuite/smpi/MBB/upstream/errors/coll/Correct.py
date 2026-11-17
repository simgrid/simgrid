#! /usr/bin/python3

from Infrastructure.ErrorGenerator import ErrorGenerator
from Infrastructure.Instruction import Instruction
from Infrastructure.MPICallFactory import MPICallFactory, CorrectMPICallFactory
from Infrastructure.CorrectParameter import CorrectParameterFactory, get_matching_recv
from Infrastructure.Template import TemplateManager
from Infrastructure.TemplateFactory import get_collective_template, get_two_collective_template
from Infrastructure.Variables import *


class CorrectColl(ErrorGenerator):
    functions_to_use = ["mpi_allgather", "mpi_allreduce", "mpi_alltoall", "mpi_barrier", "mpi_bcast", "mpi_reduce",
                        "mpi_scatter", "mpi_exscan", "mpi_gather", "mpi_reduce_scatter_block", "mpi_scan",
                        "mpi_ibarrier", "mpi_iallreduce", "mpi_ialltoall", "mpi_ibcast", "mpi_ireduce", "mpi_iscatter",
                        "mpi_igather", "mpi_iscan"]
    functions_not_supported_yet = ["mpi_gatherv", "mpi_scatterv", "mpi_igatherv", "mpi_iscatterv"]
    topology_functions = ["mpi_cart_create"]

    def __init__(self):
        pass

    def get_feature(self):
        return ["COLL"]

    def generate(self, generate_level, real_world_score_table):

        # Only one function called by all processes
        for func_to_use in self.functions_to_use:
            tm = get_collective_template(func_to_use)
            tm.set_description("Correct-" + func_to_use, "Correct code")
            yield tm
            if generate_level <= BASIC_TEST_LEVEL:
                break

        # Separate function called depending on process ID
        for func_to_use in self.functions_to_use:
            tm = get_two_collective_template(func_to_use, func_to_use)
            tm.set_description("Correct-" + func_to_use + "-" + func_to_use, "Correct code")
            yield tm
            if generate_level <= BASIC_TEST_LEVEL:
                break

        # Generate scenarios with topology functions
        for func_to_use in self.topology_functions:
            tm = get_collective_template(func_to_use)
            tm.set_description("Correct-" + func_to_use, "Correct code")
            comm_free = CorrectMPICallFactory().mpi_comm_free()
            comm_free.set_arg("comm", "&mpi_comm_0")
            tm.register_instruction(comm_free)
            yield tm
            tm.set_description("Correct-" + func_to_use + "-mpi_cart_get", "Correct code")
            cart_get = CorrectMPICallFactory().mpi_cart_get()
            cart_get.set_arg("comm_cart", "mpi_comm_0")
            tm.insert_instruction(cart_get,before_instruction=comm_free)
            yield tm
            if generate_level <= BASIC_TEST_LEVEL:
                break
