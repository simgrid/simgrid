#! /usr/bin/python3
from Infrastructure.Variables import *

from Infrastructure.ErrorGenerator import ErrorGenerator
from Infrastructure.Instruction import Instruction

from Infrastructure.MPICallFactory import CorrectMPICallFactory, MPICallFactory
from Infrastructure.TemplateFactory import get_send_recv_template, get_invalid_param_p2p_case, replace_wait


class InvalidFlagErrorP2P(ErrorGenerator):
    # statusES ignore is only allowed in test/waitALL functions
    invalid_flag = ["NULL", "not_allocated"]
    test_funcs = ["mpi_test", "mpi_testall", "mpi_waitany", "mpi_testany", "mpi_waitsome", "mpi_testsome"]

    def __init__(self):
        pass

    def get_feature(self):
        return ["P2P"]

    def generate_mprobe(self, probe_func, recv_func, test_probe, status):
        send_func = "mpi_send"

        func_to_test = recv_func
        if test_probe:
            func_to_test = probe_func
        tm = get_send_recv_template(send_func, [probe_func, recv_func])
        tm.set_description("InvalidParam-Status-" + func_to_test, "Invalid Status: %s" % status)
        if status == "MPI_STATUSES_IGNORE":
            tm.set_description("InvalidParam-Status-" + func_to_test,
                               "Invalid Status: %s\n wrong usage of status_ignore vs statusES_ignore" % status)

        if test_probe:
            for call in tm.get_instruction(identifier="MPICALL", return_list=True):
                if call.get_rank_executing() == 0:
                    call.set_arg("status", status)
                    call.set_has_error()
        else:
            for call in tm.get_instruction(identifier="MATCHEDRECEIVE", return_list=True):
                if call.get_rank_executing() == 0:
                    call.set_arg("status", status)
                    call.set_has_error()

        return tm

    def generate_probe(self, probe_to_use, status):
        tm = get_send_recv_template("mpi_send", "mpi_recv")
        tm.set_description("InvalidParam-Status-" + probe_to_use, "Invalid Status: %s" % status)
        if status == "MPI_STATUSES_IGNORE":
            tm.set_description("InvalidParam-Status-" + probe_to_use,
                               "Invalid Status: %s\n wrong usage of status_ignore vs statusES_ignore" % status)

        probe_call = CorrectMPICallFactory.get(probe_to_use)
        probe_call.set_arg("status", status)
        probe_call.set_has_error()
        probe_call.set_rank_executing(0)

        if probe_to_use == "mpi_iprobe":
            tm.add_stack_variable("int")  # the flag
            tm.insert_instruction(Instruction("int flag=0;", rank=0), before_instruction="MPICALL",
                                  before_first_of_list=True)
            tm.insert_instruction(Instruction("while (!flag){", rank=0), before_instruction="MPICALL",
                                  before_first_of_list=True)
            tm.insert_instruction(probe_call, before_instruction="MPICALL", before_first_of_list=True)
            tm.insert_instruction(Instruction("}", rank=0), before_instruction="MPICALL",
                                  before_first_of_list=True)  # end while
        else:
            tm.insert_instruction(probe_call, before_instruction="MPICALL", before_first_of_list=True)

        return tm

    def generate(self, generate_level, real_world_score_table):

        for wait_func in self.test_funcs:
            for flag in self.invalid_flag:
                tm = get_send_recv_template("mpi_send", "mpi_irecv")
                replace_wait(tm.get_instruction(identifier="WAIT"), tm, wait_func)
                wait_call = tm.get_instruction(identifier="WAIT")
                wait_call.set_has_error()
                if flag == "not_allocated":
                    tm.insert_instruction(Instruction("int* not_allocated;"), before_instruction="ALLOC",
                                          before_first_of_list=True)  # not allocated

                for arg in ["flag", "outcount", "indx", "array_of_indices"]:

                    if wait_call.has_arg(arg):
                        prev_value = wait_call.get_arg(arg)
                        wait_call.set_arg(arg, flag)
                        tm.set_description("InvalidParam-" + arg + "-" + wait_func, ("Invalid %s: %s" % (arg, flag)))
                        yield tm
                        wait_call.set_arg(arg, prev_value)

            if generate_level <= BASIC_TEST_LEVEL:
                return

        for flag in self.invalid_flag:
            tm = get_send_recv_template("mpi_send", "mpi_recv")
            if flag == "not_allocated":
                tm.insert_instruction(Instruction("int* not_allocated;"), before_instruction="ALLOC",
                                      before_first_of_list=True)  # not allocated
            arg = "flag"
            wait_func="mpi_iprobe"
            tm.set_description("InvalidParam-" + arg + "-" + wait_func, ("Invalid %s: %s" % (arg, flag)))
            probe_call = CorrectMPICallFactory.get("mpi_iprobe")
            probe_call.set_arg("flag", flag)
            probe_call.set_has_error()
            probe_call.set_rank_executing(0)
            tm.insert_instruction(Instruction("int flag=0;", rank=0), before_instruction="MPICALL",
                                  before_first_of_list=True)
            tm.insert_instruction(Instruction("while (!flag){", rank=0), before_instruction="MPICALL",
                                  before_first_of_list=True)
            tm.insert_instruction(probe_call, before_instruction="MPICALL", before_first_of_list=True)
            tm.insert_instruction(Instruction("}", rank=0), before_instruction="MPICALL", before_first_of_list=True)
            yield tm
            tm = get_send_recv_template("mpi_send", ["mpi_improbe", "mpi_mrecv"])
            arg = "flag"
            wait_func = "mpi_improbe"
            tm.set_description("InvalidParam-" + arg + "-" + wait_func, ("Invalid %s: %s" % (arg, flag)))
            if flag == "not_allocated":
                tm.insert_instruction(Instruction("int* not_allocated;"), before_instruction="ALLOC",
                                      before_first_of_list=True)  # not allocated
            for call in tm.get_instruction(identifier="MPICALL", return_list=True):
                if call.get_rank_executing() == 0:
                    call.set_arg("flag", flag)
                    call.set_has_error()
            yield tm
