#! /usr/bin/python3
from Infrastructure.Variables import *

from Infrastructure.CorrectParameter import CorrectParameterFactory
from Infrastructure.ErrorGenerator import ErrorGenerator
from Infrastructure.Instruction import Instruction
from Infrastructure.MPICall import MPICall
from Infrastructure.MPICallFactory import CorrectMPICallFactory
from Infrastructure.TemplateFactory import get_send_recv_template, get_invalid_param_p2p_case, insert_probe


class InvalidTagErrorP2P(ErrorGenerator):
    invalid_tags = ["-1", "MPI_TAG_UB+1", CorrectParameterFactory.tag + 2, "MPI_ANY_TAG"]
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

    def generate_impl(self, send_func, recv_func, check_receive):
        if send_func == "mpi_psend_init":
            recv_func = "mpi_precv_init"
        if recv_func == "mpi_precv_init":
            send_func = "mpi_psend_init"

        for tag_to_use in self.invalid_tags:
            tm = get_invalid_param_p2p_case("tag", tag_to_use, check_receive, send_func, recv_func)

            error_string = "InvalidParam"
            if tag_to_use == CorrectParameterFactory.tag + 2:
                error_string = "GlobalParameterMissmatch"
                tm.set_can_deadlock()

            if check_receive:
                if tag_to_use == "MPI_ANY_TAG":
                    # correct case
                    continue
                if tag_to_use == CorrectParameterFactory().tag + 2 and recv_func == "mpi_irecv":
                    # combination repeated
                    continue
                tm.set_description(error_string + "-Tag-" + recv_func, "Invalid Tag: %s" % tag_to_use)
            else:
                tm.set_description(error_string + "-Tag-" + send_func, "Invalid Tag: %s" % tag_to_use)

            if tag_to_use == CorrectParameterFactory.tag + 2:
                # missmatch is between both, mark other as well
                for c in tm.get_instruction("MPICALL", return_list=True):
                    if isinstance(c, MPICall):
                        c.set_has_error()

            yield tm

    def generate_mprobe(self, probe_to_use):
        for tag_to_use in self.invalid_tags:
            if tag_to_use == "MPI_ANY_TAG":
                # correct case
                continue

            tm = get_send_recv_template("mpi_send", [probe_to_use, "mpi_mrecv"])

            error_string = "InvalidParam"
            if tag_to_use == CorrectParameterFactory.tag + 2:
                error_string = "GlobalParameterMissmatch"
                tm.set_can_deadlock()

            tm.set_description(error_string + "-Tag-" + probe_to_use, "Invalid Tag: %s" % tag_to_use)

            for call in tm.get_instruction("MPICALL", return_list=True):
                if call.get_rank_executing() == 0:
                    call.set_arg("tag", tag_to_use)
                    call.set_has_error()

            yield tm

    def generate_probe(self, probe_to_use):
        for tag_to_use in self.invalid_tags:
            if tag_to_use == "MPI_ANY_TAG":
                # correct case
                continue

            tm = get_send_recv_template("mpi_send", "mpi_recv")

            error_string = "InvalidParam"
            if tag_to_use == CorrectParameterFactory.tag + 2:
                error_string = "GlobalParameterMissmatch"
                tm.set_can_deadlock()

            tm.set_description(error_string + "-Tag-" + probe_to_use, "Invalid Tag: %s" % tag_to_use)

            recv_call = tm.get_instruction(identifier="MPICALL", rank_excuting=0)
            probe_call = insert_probe(tm, probe_to_use, recv_call)
            probe_call.set_arg("tag", tag_to_use)
            probe_call.set_has_error()

            yield tm

    def generate(self, generate_level, real_world_score_table):

        for send_func in self.send_funcs:
            yield from self.generate_impl(send_func, "mpi_irecv", False)
            if generate_level <= BASIC_TEST_LEVEL:
                return
        for recv_func in self.recv_funcs:
            yield from self.generate_impl("mpi_send", recv_func, True)

        for func in self.sendrecv_funcs:
            # yield from self.generate_impl(func, func, True)
            # yield from self.generate_impl(func, func, False)
            pass

        yield from self.generate_mprobe("mpi_mprobe")
        yield from self.generate_mprobe("mpi_improbe")
        yield from self.generate_probe("mpi_probe")
        yield from self.generate_probe("mpi_iprobe")
