#! /usr/bin/python3
from Infrastructure.Template import TemplateManager
from Infrastructure.Variables import *
from Infrastructure.ErrorGenerator import ErrorGenerator
from Infrastructure.Instruction import Instruction
from Infrastructure.MPICallFactory import MPICallFactory
from Infrastructure.TemplateFactory import get_communicator, get_intercomm, \
    get_invalid_param_p2p_case, get_send_recv_template

sendrecv_funcs = ["mpi_sendrecv", "mpi_sendrecv_replace"]


class Initialized(ErrorGenerator):

    def __init__(self):
        pass

    def get_feature(self):
        return ["other"]

    def generate(self, generate_level, real_world_score_table):
        tm = TemplateManager()
        tm.set_description("InvalidParam-flag-mpi_initialized","Null ptr as flag")

        c = MPICallFactory.mpi_initialized("NULL")
        c.set_has_error()
        tm.register_instruction(c)
        yield tm

        tm = get_send_recv_template("mpi_send","mpi_recv")

        tm._has_init = False

        tm.set_description("MissingCall-mpi_init", "No MPI_Init")

        for call in tm.get_instruction(identifier="MPICALL",return_list=True):
            call.set_has_error()

        yield tm