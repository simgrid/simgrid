#! /usr/bin/python3
from Infrastructure.Variables import *
from copy import copy

from Infrastructure.AllocCall import AllocCall
from Infrastructure.ErrorGenerator import ErrorGenerator
from Infrastructure.Instruction import Instruction
from Infrastructure.MPICallFactory import MPICallFactory, CorrectMPICallFactory
from Infrastructure.CorrectParameter import CorrectParameterFactory
from Infrastructure.Template import TemplateManager
from Infrastructure.TemplateFactory import get_send_recv_template, get_invalid_param_p2p_case

from itertools import chain


class CorrectP2P(ErrorGenerator):
    send_funcs = ["mpi_send",
                  "mpi_isend", "mpi_ssend", "mpi_issend", "mpi_rsend", "mpi_irsend", "mpi_bsend", "mpi_ibsend",
                  "mpi_send_init", "mpi_ssend_init", "mpi_bsend_init", "mpi_rsend_init", "mpi_psend_init",
                  ]

    recv_funcs = ["mpi_recv", "mpi_irecv", "mpi_recv_init", "mpi_precv_init"]

    sendrecv_funcs = ["mpi_sendrecv", "mpi_sendrecv_replace"]

    def __init__(self):
        pass

    def get_feature(self):
        return ["P2P"]

    def generate(self, generate_level, real_world_score_table):
        for func in self.send_funcs + self.recv_funcs:
            if func in self.recv_funcs:
                check_recv = True
                send_func = "mpi_send"
                recv_func = func
            else:
                check_recv = False
                send_func = func
                recv_func = "mpi_irecv"

            if send_func == "mpi_psend_init":
                recv_func = "mpi_precv_init"
            if recv_func == "mpi_precv_init":
                send_func = "mpi_psend_init"

            tm = get_send_recv_template(send_func, recv_func)
            tm.set_description("Correct-"+func,"Correct usage of "+func)
            yield tm
            if generate_level <= BASIC_TEST_LEVEL:
                return

        for func in ["mpi_mrecv", "mpi_imrecv"]:
            tm = get_send_recv_template("mpi_isend", ["mpi_mprobe", func])
            tm.set_description("Correct-"+func,"Correct usage of "+func)
            yield tm
    #TODO missing: probe waitall/testall