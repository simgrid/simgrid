#! /usr/bin/python3
from Infrastructure.Variables import *

from Infrastructure.ErrorGenerator import ErrorGenerator
from Infrastructure.Instruction import Instruction

from Infrastructure.MPICallFactory import CorrectMPICallFactory, MPICallFactory
from Infrastructure.TemplateFactory import get_send_recv_template, get_invalid_param_p2p_case, replace_wait, \
    insert_probe


class InvalidStatusErrorP2P(ErrorGenerator):
    # statusES ignore is only allowed in test/waitALL functions
    invalid_status = ["NULL", "MPI_STATUSES_IGNORE"]
    recv_funcs = ["mpi_recv"]
    probe_recv_funcs = ["mpi_mprobe", "mpi_improbe"]
    test_funcs = ["mpi_wait", "mpi_test", "mpi_waitall", "mpi_testall", "mpi_waitany", "mpi_testany", "mpi_waitsome",
                  "mpi_testsome"]
    # , "mpi_testall", "mpi_testsome", "mpi_testany"]
    sendrecv_funcs = ["mpi_sendrecv", "mpi_sendrecv_replace"]

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

        recv_call = tm.get_instruction(identifier="MPICALL", rank_excuting=0)
        probe_call = insert_probe(tm, probe_to_use, recv_call)
        probe_call.set_arg("status", status)
        probe_call.set_has_error()

        return tm

    def generate(self, generate_level, real_world_score_table):

        check_recv = True
        send_func = "mpi_send"
        recv_func = "mpi_recv"
        for status in self.invalid_status:
            tm = get_invalid_param_p2p_case("status", status, check_recv, send_func, recv_func)
            tm.set_description("InvalidParam-Status-" + "mpi_recv", "Invalid Status: %s" % status)
            if status == "MPI_STATUSES_IGNORE":
                tm.set_description("InvalidParam-Status-" + "mpi_recv",
                                   "Invalid Status: %s\n wrong usage of status_ignore vs statusES_ignore" % status)
            yield tm

        if generate_level <= BASIC_TEST_LEVEL:
            return

        for wait_func in self.test_funcs:
            for status in self.invalid_status:
                tm = get_send_recv_template("mpi_send", "mpi_irecv")
                tm.set_description("InvalidParam-Status-" + wait_func, "Invalid Status: %s" % status)
                if status == "MPI_STATUSES_IGNORE":
                    tm.set_description("InvalidParam-Status-" + wait_func,
                                       "Invalid Status: %s\n wrong usage of status_ignore vs statusES_ignore" % status)
                # replace with wrong wait call
                replace_wait(tm.get_instruction(identifier="WAIT"), tm, wait_func)
                wait_call = tm.get_instruction(identifier="WAIT")
                if wait_call.has_arg("status"):
                    wait_call.set_arg("status", status)
                else:
                    s_to_use = status
                    if status == "MPI_STATUSES_IGNORE":
                        s_to_use = "MPI_STATUS_IGNORE"
                    wait_call.set_arg("array_of_statuses", s_to_use)
                wait_call.set_has_error()

                yield tm

        for status in self.invalid_status:
            yield self.generate_probe("mpi_probe", status)
            yield self.generate_probe("mpi_iprobe", status)
            yield self.generate_mprobe("mpi_mprobe", "mpi_mrecv", True, status)
            yield self.generate_mprobe("mpi_mprobe", "mpi_mrecv", False, status)
            yield self.generate_mprobe("mpi_improbe", "mpi_mrecv", True, status)

            # sendrecv cases
            for func in self.sendrecv_funcs:
                tm = get_send_recv_template(func, func)
                tm.set_description("InvalidParam-Status-" + func, "Invalid Status: %s" % status)
                if status == "MPI_STATUSES_IGNORE":
                    tm.set_description("InvalidParam-Status-" + func,
                                       "Invalid Status: %s\n wrong usage of status_ignore vs statusES_ignore" % status)
                for call in tm.get_instruction(identifier="MPICALL", return_list=True):
                    call.set_has_error()
                    call.set_arg("status", status)
                yield tm
