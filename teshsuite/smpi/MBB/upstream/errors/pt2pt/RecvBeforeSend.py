#! /usr/bin/python3
from Infrastructure.Variables import *
import itertools

from Infrastructure.ErrorGenerator import ErrorGenerator
from Infrastructure.Instruction import Instruction
from Infrastructure.MPICallFactory import CorrectMPICallFactory
from Infrastructure.TemplateFactory import get_send_recv_template, insert_probe


class RecvBeforeSend(ErrorGenerator):
    recv_funcs = ["mpi_recv", "mpi_irecv", "mpi_recv_init", "mpi_precv_init"]
    probe_recv_funcs = ["mpi_mprobe", "mpi_improbe"]
    probe_funcs = ["mpi_probe", "mpi_iprobe"]

    def __init__(self):
        pass

    def get_feature(self):
        return ["P2P"]

    def insert_reverse_msg(self, tm):
        send = CorrectMPICallFactory.mpi_send()
        send.set_arg("dest", "1")
        send.set_identifier("REVERSE_MSG_SEND")
        send.set_rank_executing(0)
        tm.insert_instruction(send, before_instruction="FREE", before_first_of_list=True)
        recv = CorrectMPICallFactory.mpi_recv()
        recv.set_arg("source", "0")
        recv.set_rank_executing(1)
        recv.set_identifier("REVERSE_MSG_RECEIVE")
        tm.insert_instruction(recv, before_instruction="FREE", before_first_of_list=True)

    def generate_impl(self, send_func, recv_func):

        if send_func == "mpi_psend_init":
            recv_func = "mpi_precv_init"
        if recv_func == "mpi_precv_init":
            return

        tm = get_send_recv_template(send_func, recv_func)

        tm.set_description("CallOrdering-" + send_func + "-" + recv_func,
                           "Call Ordering: both ranks try to receive before sending")
        tm.set_can_deadlock()

        self.insert_reverse_msg(tm)
        # this is a correct case with no deadlock

        # introduce deadlock by moving rank 1s send after the recv
        rank_1_send_instr = (
                [c for c in tm.get_instruction("MPI_CALL", return_list=True) if c.get_rank_executing() == 1]
                + [c for c in tm.get_instruction("START", return_list=True) if
                   c.get_rank_executing() == 1])

        wait_calls = [c for c in tm.get_instruction("WAIT", return_list=True) if c.get_rank_executing() == 1]

        for c in rank_1_send_instr + wait_calls:
            tm.remove_instruction(instruction=c)
        tm.insert_instruction(rank_1_send_instr, before_instruction="FREE", before_first_of_list=True)
        tm.insert_instruction(wait_calls, before_instruction="FREE", before_first_of_list=True)

        yield tm
        assert send_func == "mpi_send"
        tm.set_description("MissingCall-" + send_func, "Deadlock, as call to send is missing")
        tm.set_can_deadlock()

        for c in tm.get_instruction("REVERSE_MSG_SEND", return_list=True)+tm.get_instruction("MPICALL", return_list=True):
            if c.get_function() == send_func:
                tm.remove_instruction(instruction=c)
            else:
                c.set_has_error()
        yield tm

    def generate_probe(self, probe_to_use):
        tm = get_send_recv_template("mpi_send", "mpi_recv")

        tm.set_description("CallOrdering-" + probe_to_use + "-" + "mpi_send",
                           "Call Ordering: probe for message before it is going to be send")
        tm.set_can_deadlock()

        self.insert_reverse_msg(tm)

        # add the probe to rank 1 before the send
        recv_call = tm.get_instruction(identifier="MPICALL", rank_excuting=0)
        probe_call = insert_probe(tm, probe_to_use, recv_call)
        probe_call.set_arg("source", "0")  # recv from 1 is the default
        probe_call.set_has_error()

        yield tm

    def generate_mprobe(self, probe_to_use):
        tm = get_send_recv_template("mpi_send", "mpi_recv")
        self.insert_reverse_msg(tm)

        tm.add_stack_variable("MPI_Message")

        tm.set_description("CallOrdering-" + probe_to_use + "-" + "mpi_send",
                           "Call Ordering: probe for message before it is going to be send")
        tm.set_can_deadlock()

        # use mrecv instead of recv
        mrecv = CorrectMPICallFactory.mpi_mrecv()
        mrecv.set_rank_executing(1)
        tm.replace_instruction(mrecv, identifier="REVERSE_MSG_RECEIVE")

        # add the probe to rank 1 before the send
        probecall = CorrectMPICallFactory.get(probe_to_use)
        probecall.set_arg("source", "0")  # recv from 1 is the default
        probecall.set_has_error()
        probecall.set_rank_executing(1)
        if probe_to_use == "mpi_improbe":
            tm.add_stack_variable("int")  # the flag
            tm.insert_instruction(Instruction("int flag=0;", rank=1), before_instruction="MPICALL",
                                  before_first_of_list=True)
            tm.insert_instruction(Instruction("while (!flag){", rank=1), before_instruction="MPICALL",
                                  before_first_of_list=True)
            tm.insert_instruction(probecall, before_instruction="MPICALL", before_first_of_list=True)
            tm.insert_instruction(Instruction("}", rank=1), before_instruction="MPICALL",
                                  before_first_of_list=True)  # end while
        else:
            tm.insert_instruction(probecall, before_instruction="MPICALL", before_first_of_list=True)
        yield tm

    def generate(self, generate_level, real_world_score_table):
        for recv_func in self.recv_funcs:
            yield from self.generate_impl("mpi_send", recv_func)
            if generate_level <= BASIC_TEST_LEVEL:
                return

        yield from self.generate_probe("mpi_probe")
        yield from self.generate_probe("mpi_iprobe")
        yield from self.generate_mprobe("mpi_mprobe")
        yield from self.generate_mprobe("mpi_improbe")
