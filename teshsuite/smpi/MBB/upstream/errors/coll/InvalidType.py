#! /usr/bin/python3
from Infrastructure.Variables import *

from Infrastructure.ErrorGenerator import ErrorGenerator
from Infrastructure.Instruction import Instruction
from Infrastructure.MPICallFactory import MPICallFactory, CorrectMPICallFactory
from Infrastructure.CorrectParameter import CorrectParameterFactory
from Infrastructure.Template import TemplateManager
from Infrastructure.TemplateFactory import get_collective_template


class InvalidComErrorColl(ErrorGenerator):
    invalid_type = ["MPI_DATATYPE_NULL", "NULL"]
    functions_to_use = ["mpi_bcast", "mpi_ibcast", "mpi_reduce", "mpi_ireduce", "mpi_exscan", "mpi_scan", "mpi_iscan",
                        "mpi_gather", "mpi_igather", "mpi_allgather", "mpi_iallgather", "mpi_allreduce",
                        "mpi_iallreduce", "mpi_alltoall", "mpi_ialltoall", "mpi_scatter", "mpi_iscatter"]
    func_one_type_arg = ["mpi_bcast", "mpi_reduce", "mpi_exscan", "mpi_scan", "mpi_ibcast", "mpi_ireduce", "mpi_iscan",
                         "mpi_allreduce", "mpi_iallreduce"]
    functions_not_supported_yet = ["mpi_reduce_scatter_block", "mpi_allgatherv", "mpi_alltoallv", "mpi_alltoallw",
                                   "mpi_gatherv", "mpi_reduce_scatter", "mpi_scatterv"]

    def __init__(self):
        pass

    def get_feature(self):
        return ["COLL"]

    def generate(self, generate_level, real_world_score_table):
        for type_to_use in self.invalid_type:
            for func_to_use in self.functions_to_use:
                tm = get_collective_template(func_to_use)
                tm.set_description("InvalidParam-Type-" + func_to_use, "Invalid datatype: %s" % type_to_use)

                if func_to_use in self.func_one_type_arg:
                    for call in tm.get_instruction("MPICALL", return_list=True):
                        # if call.has_arg("recvtype"): # sendtype
                        # call.set_arg("recvtype", type_to_use)
                        call.set_arg("datatype", type_to_use)
                        call.set_has_error()
                    yield tm

                if generate_level <= BASIC_TEST_LEVEL:
                    return
