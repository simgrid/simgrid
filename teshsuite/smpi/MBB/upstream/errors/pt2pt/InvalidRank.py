#! /usr/bin/python3
from Infrastructure.Variables import *

from Infrastructure.ErrorGenerator import ErrorGenerator
from Infrastructure.Instruction import Instruction

from Infrastructure.MPICallFactory import CorrectMPICallFactory
from Infrastructure.TemplateFactory import get_send_recv_template, get_invalid_param_p2p_case, insert_probe


class InvalidRankErrorP2P(ErrorGenerator):
    invalid_ranks = ["-1", "nprocs"] # "MPI_PROC_NULL"
    send_funcs = ["mpi_send",
                  "mpi_isend", "mpi_ssend", "mpi_issend", "mpi_rsend", "mpi_irsend", "mpi_bsend", "mpi_ibsend",
                  "mpi_send_init", "mpi_ssend_init", "mpi_bsend_init", "mpi_rsend_init", "mpi_psend_init"
                  ]
    recv_funcs = ["mpi_recv", "mpi_irecv", "mpi_recv_init", "mpi_precv_init"]
    probe_recv_funcs = ["mpi_mprobe", "mpi_improbe"]

    sendrecv_funcs = ["mpi_sendrecv", "mpi_sendrecv_replace"]

    def __init__(self):
        pass

    def get_feature(self):
        return ["P2P"]

    def generate_mprobe(self, recv_func, rank_to_use):
        send_func = "mpi_isend"

        tm = get_send_recv_template(send_func, [recv_func, "mpi_mrecv"])

        tm.set_description("InvalidParam-Rank-" + recv_func, "Invalid Rank: %s" % rank_to_use)

        for call in tm.get_instruction(identifier="MPICALL", return_list=True):
            if call.get_rank_executing() == 0:
                call.set_arg("source", rank_to_use)
                call.set_has_error()

        return tm

    def generate_probe(self, probe_to_use, rank_to_use):
        tm = get_send_recv_template("mpi_send", "mpi_recv")

        tm.set_description("InvalidParam-Rank-" + probe_to_use, "Invalid Rank: %s" % rank_to_use)

        recv_call = tm.get_instruction(identifier="MPICALL", rank_excuting=0)
        probe_call = insert_probe(tm, probe_to_use, recv_call)
        probe_call.set_arg("source", rank_to_use)
        probe_call.set_has_error()

        return tm

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

            for rank_to_use in self.invalid_ranks:

                if check_recv:
                    tm = get_invalid_param_p2p_case("source", rank_to_use, check_recv, send_func, recv_func)
                else:
                    tm = get_invalid_param_p2p_case("dest", rank_to_use, check_recv, send_func, recv_func)

                tm.set_description("InvalidParam-Rank-" + func, "Invalid Rank: %s" % rank_to_use)

                yield tm

            if generate_level <= BASIC_TEST_LEVEL:
                return

        for rank_to_use in self.invalid_ranks:
            check_recv = False
            func = "mpi_sendrecv"
            send_func = func
            recv_func = func
            tm = get_invalid_param_p2p_case("dest", rank_to_use, check_recv, send_func, recv_func)
            tm.set_description("InvalidParam-Rank-" + func, "Invalid Rank: %s" % rank_to_use)
            yield tm

            check_recv = True
            tm = get_invalid_param_p2p_case("source", rank_to_use, check_recv, send_func, recv_func)
            tm.set_description("InvalidParam-Rank-" + func, "Invalid Rank: %s" % rank_to_use)
            yield tm

        for rank_to_use in self.invalid_ranks:
            yield self.generate_probe("mpi_probe", rank_to_use)
            yield self.generate_probe("mpi_iprobe", rank_to_use)
            yield self.generate_mprobe("mpi_mprobe", rank_to_use)
            yield self.generate_mprobe("mpi_improbe", rank_to_use)
