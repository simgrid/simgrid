#! /usr/bin/python3
from Infrastructure.Variables import *

from Infrastructure.ErrorGenerator import ErrorGenerator
from Infrastructure.Instruction import Instruction
from Infrastructure.TemplateFactory import get_send_recv_template

sendrecv_funcs = ["mpi_sendrecv", "mpi_sendrecv_replace"]


class LocalConcurrencyErrorP2P(ErrorGenerator):
    functions_to_check = ["mpi_irecv",
                          "mpi_isend", "mpi_issend", "mpi_irsend", "mpi_ibsend",
                          "mpi_send_init", "mpi_ssend_init", "mpi_bsend_init", "mpi_rsend_init", "mpi_psend_init",
                          "mpi_precv_init", "mpi_recv_init", "mpi_imrecv"
                          ]
    recv_funcs = ["mpi_irecv", "mpi_recv_init", "mpi_precv_init", "mpi_imrecv"]

    def __init__(self):
        pass

    def get_feature(self):
        return ["P2P"]

    def generate(self, generate_level, real_world_score_table):
        for send_func in self.functions_to_check:

            check_receive = False
            recv_func = "mpi_irecv"
            if send_func in self.recv_funcs:
                check_receive = True
                recv_func = send_func
                send_func = "mpi_isend"

            if send_func == "mpi_psend_init":
                recv_func = "mpi_precv_init"
            if recv_func == "mpi_precv_init":
                send_func = "mpi_psend_init"

            recv_func_to_use = recv_func
            if recv_func == "mpi_imrecv":
                recv_func_to_use = ["mpi_mprobe", "mpi_imrecv"]
            tm = get_send_recv_template(send_func, recv_func_to_use)

            if check_receive:
                tm.set_description("LocalConcurrency-receive-" + recv_func,
                                   "usage of receive buffer before operation is completed")
            else:
                tm.set_description("LocalConcurrency-send-" + send_func,
                                   "usage of send buffer before operation is completed")

            conflicting_inst = Instruction("buf[2]=1;")
            conflicting_inst.set_has_error()
            kind = 1
            if check_receive:
                kind = 0
            conflicting_inst.set_rank_executing(kind)

            if recv_func != "mpi_imrecv":
                for c in tm.get_instruction("MPICALL", return_list=True):
                    if c.get_rank_executing() == kind:
                        c.set_has_error()
            else:
                tm.get_instruction("MATCHEDRECEIVE").set_has_error()


            for c in tm.get_instruction("WAIT", return_list=True):
                if c.get_rank_executing() == kind:
                    tm.insert_instruction(conflicting_inst,before_instruction=c)

            #TODO WHY NOT ERROR

            yield tm
            if generate_level <= BASIC_TEST_LEVEL:
                return
