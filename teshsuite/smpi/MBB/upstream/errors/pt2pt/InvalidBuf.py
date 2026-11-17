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


class InvalidBufErrorP2P(ErrorGenerator):
    invalid_bufs = [ "NULL", 'MPI_BOTTOM', 'MPI_IN_PLACE']
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

            for buf_to_use in self.invalid_bufs:
                tm = get_invalid_param_p2p_case("buf", buf_to_use, check_recv, send_func, recv_func)
                tm.set_description("InvalidParam-Buffer-" + func, "Invalid Buffer: "+buf_to_use)
                yield tm

            tm = get_invalid_param_p2p_case("buf", "not_allocated", check_recv, send_func, recv_func)
            tm.set_description("InvalidParam-Buffer-" + func, "Invalid Buffer: Not allocated")
            tm.insert_instruction(Instruction("int* not_allocated;"), before_instruction="ALLOC",
                                  before_first_of_list=True)
            yield tm

            # use buffer after free
            tm = get_invalid_param_p2p_case("buf", "buf", check_recv, send_func, recv_func)
            tm.set_description("InvalidParam-Buffer-" + func, "Invalid Buffer: Use after Free")
            call_list = tm.get_instruction(identifier="FREE", return_list=True)
            tm.remove_instruction(identifier="FREE")
            tm.insert_instruction(call_list, before_instruction="MPICALL", before_first_of_list=True)

            yield tm

            if generate_level <= BASIC_TEST_LEVEL:
                return

        for func in ["mpi_mrecv", "mpi_imrecv"]:
            for buf_to_use in self.invalid_bufs:
                tm = get_send_recv_template("mpi_isend", ["mpi_mprobe", func])
                tm.set_description("InvalidParam-Buffer-" + func, "Invalid Buffer: "+buf_to_use)
                for call in tm.get_instruction(identifier="MATCHEDRECEIVE", return_list=True):
                    call.set_arg("buf", buf_to_use)
                    call.set_has_error()
                yield tm

            tm = get_send_recv_template("mpi_isend", ["mpi_mprobe", func])
            tm.set_description("InvalidParam-Buffer-" + func, "Invalid Buffer: Not allocated")
            tm.insert_instruction(Instruction("int* not_allocated;"), before_instruction="ALLOC",
                                  before_first_of_list=True)
            for call in tm.get_instruction(identifier="MATCHEDRECEIVE", return_list=True):
                call.set_arg("buf", "not_allocated")
                call.set_has_error()
            yield tm

            tm = get_send_recv_template("mpi_isend", ["mpi_mprobe", func])
            tm.set_description("InvalidParam-Buffer-" + func, "use after free")
            tm.insert_instruction(Instruction("int* not_allocated;"), before_instruction="ALLOC",
                                  before_first_of_list=True)

            call_list = tm.get_instruction(identifier="FREE", return_list=True)
            tm.remove_instruction(identifier="FREE")
            tm.insert_instruction(call_list, before_instruction="MPICALL", before_first_of_list=True)
            for call in tm.get_instruction(identifier="MATCHEDRECEIVE", return_list=True):
                call.set_has_error()
            yield tm


class MessageRaceErrorSendRecv(ErrorGenerator):

    def __init__(self):
        pass

    def get_feature(self):
        return ["P2P"]

    def generate(self, generate_level, real_world_score_table):

        for buf_to_use in ["buf", "MPI_IN_PLACE"]:
            tm = get_send_recv_template("mpi_sendrecv", "mpi_sendrecv")
            tm.set_description("InvalidParam-Buffer-mpi_sendrecv", "send and recv buffer must be disjoint in sendrecv")

            for call in tm.get_instruction(identifier="MPICALL", return_list=True):
                call.set_arg("recvbuf", buf_to_use)
                call.set_has_error()
            yield tm

        # NULL
        tm = get_send_recv_template("mpi_sendrecv", "mpi_sendrecv")
        tm.set_description("InvalidParam-Buffer-mpi_sendrecv", "NULL as receive buffer")

        for call in tm.get_instruction(identifier="MPICALL", return_list=True):
            call.set_arg("recvbuf", "NULL")
            call.set_has_error()
        yield tm

        tm = get_send_recv_template("mpi_sendrecv", "mpi_sendrecv")
        tm.set_description("InvalidParam-Buffer-mpi_sendrecv", "NULL as send buffer")

        for call in tm.get_instruction(identifier="MPICALL", return_list=True):
            call.set_arg("sendbuf", "NULL")
            call.set_has_error()
        yield tm

        # use without alloc
        tm = get_send_recv_template("mpi_sendrecv", "mpi_sendrecv")
        tm.set_description("InvalidParam-Buffer-mpi_sendrecv", "not allocated receive buffer")
        tm.insert_instruction(Instruction("int* not_allocated;"), before_instruction="ALLOC", before_first_of_list=True)

        for call in tm.get_instruction(identifier="MPICALL", return_list=True):
            call.set_arg("recvbuf", "not_allocated")
            call.set_has_error()
        yield tm

        tm = get_send_recv_template("mpi_sendrecv", "mpi_sendrecv")
        tm.set_description("InvalidParam-Buffer-mpi_sendrecv", "not allocated send buffer")
        tm.insert_instruction(Instruction("int* not_allocated;"), before_instruction="ALLOC", before_first_of_list=True)

        for call in tm.get_instruction(identifier="MPICALL", return_list=True):
            call.set_arg("sendbuf", "not_allocated")
            call.set_has_error()
        yield tm

        # use after free
        tm = get_send_recv_template("mpi_sendrecv", "mpi_sendrecv")
        tm.set_description("InvalidParam-Buffer-mpi_sendrecv", "use after free for buffers")
        call_list = tm.get_instruction(identifier="FREE", return_list=True)
        tm.remove_instruction(identifier="FREE")
        tm.insert_instruction(call_list, before_instruction="MPICALL", before_first_of_list=True)
        yield tm


class Overlapping_buf(ErrorGenerator):

    def __init__(self):
        pass

    def get_feature(self):
        return ["P2P"]

    def generate(self, generate_level, real_world_score_table):
        # TODO this case for recv_init, imrecv and precv_init in all combinations?
        recv_func = "mpi_irecv"
        tm = get_send_recv_template("mpi_send", recv_func)
        tm.set_description("LocalConcurrency-" + recv_func, "Overlapping recv buffers")
        recv = tm.get_instruction(identifier="MPICALL", rank_excuting=0)
        buf_var = recv.get_arg("buf")
        buf_len = recv.get_arg("count")
        second_recv = copy(recv)
        req = tm.add_stack_variable("MPI_Request")
        second_recv.set_arg("request", "&" + req)
        second_recv.set_has_error()
        tm.insert_instruction(second_recv, after_instruction=recv)  #
        wait = tm.get_instruction(identifier="WAIT", rank_excuting=0)
        second_wait = copy(wait)
        second_wait.set_arg("request", "&" + req)
        tm.insert_instruction(second_wait, after_instruction=wait)
        send = tm.get_instruction(identifier="MPICALL", rank_excuting=1)
        # same envelope is used
        second_send = copy(send)
        tm.insert_instruction(second_send, after_instruction=send)
        yield tm

        if generate_level <= BASIC_TEST_LEVEL:
            return

        tm.set_description("LocalConcurrency-" + recv_func, "partially overlapping recv buffers")
        recv.set_arg("count", buf_len + "/2")
        second_recv.set_arg("count", buf_len + "/2")
        second_recv.set_arg("buf", "&" + buf_var + "[" + buf_len + "/4]")
        second_recv.set_has_error()
        send.set_arg("count", buf_len + "/2")
        second_send.set_arg("count", buf_len + "/2")
        yield tm
