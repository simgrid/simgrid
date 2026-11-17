#! /usr/bin/python3
from Infrastructure.Variables import *

from Infrastructure.ErrorGenerator import ErrorGenerator
from Infrastructure.Instruction import Instruction
from Infrastructure.MPICall import MPICall
from Infrastructure.TemplateFactory import get_send_recv_template, replace_wait
from Infrastructure.MPICallFactory import MPICallFactory


class InvalidRequestErrorP2P(ErrorGenerator):
    invalid_requests = ["MPI_REQUEST_NULL",  # probably triggers compiler warning
                        # "&MPI_REQUEST_NULL" #TODO test this as well?? depending on MPI implementation: may be compiler error
                        "NULL"]
    functions_to_check = ["mpi_irecv", "mpi_isend",
                          "mpi_issend", "mpi_irsend", "mpi_ibsend",
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
            for req_to_use in self.invalid_requests:

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
                    tm.set_description("InvalidParam-Request-" + recv_func, "Invalid Request: %s" % req_to_use)
                else:
                    tm.set_description("InvalidParam-Request-" + send_func, "Invalid Request: %s" % req_to_use)

                kind = 1
                if check_receive:
                    kind = 0

                for inst in tm.get_instruction(identifier="MPICALL", return_list=True) + tm.get_instruction(
                        identifier="MATCHEDRECEIVE", return_list=True):
                    if inst.get_rank_executing() == kind:
                        if isinstance(inst, MPICall) and inst.has_arg("request"):
                            inst.set_arg("request", req_to_use)
                            inst.set_has_error()

                yield tm

            if generate_level <= BASIC_TEST_LEVEL:
                return


class InvalidRequestArrayP2P(ErrorGenerator):
    test_funcs = ["mpi_testall", "mpi_waitany", "mpi_testany", "mpi_waitsome", "mpi_testsome"]

    def __init__(self):
        pass

    def get_feature(self):
        return ["P2P"]

    def generate(self, generate_level, real_world_score_table):
        for wait_func in self.test_funcs:
            tm = get_send_recv_template("mpi_send", "mpi_irecv")
            tm.set_description("InvalidParam-Request-" + wait_func,
                               "Invalid Request Array: the same request is not allowed to be in the array multiple times")
            replace_wait(tm.get_instruction(identifier="WAIT"), tm, wait_func)
            wait_call = tm.get_instruction(identifier="WAIT")
            wait_call.set_has_error()

            prev_req = wait_call.get_arg("array_of_requests")[1:]  # remove leading &
            wait_call.set_arg("array_of_requests", "req_array")

            tm.insert_instruction(Instruction("MPI_Request req_array[2];", rank=wait_call.get_rank_executing()),
                                  before_instruction=wait_call)
            tm.insert_instruction(Instruction("req_array[0] = " + prev_req + ";", rank=wait_call.get_rank_executing()),
                                  before_instruction=wait_call)
            tm.insert_instruction(Instruction("req_array[1] = " + prev_req + ";", rank=wait_call.get_rank_executing()),
                                  before_instruction=wait_call)

            yield tm
            if generate_level <= BASIC_TEST_LEVEL:
                return

            wait_call.set_arg("array_of_requests", "NULL")
            tm.set_description("InvalidParam-Request-" + wait_func,
                               "Invalid Request Array: NULL")
            yield tm

        # NULL error also for wait and test
        tm = get_send_recv_template("mpi_send", "mpi_irecv")
        tm.set_description("InvalidParam-Request-mpi_wait",
                           "Invalid Request: NULL")
        wait_call = tm.get_instruction(identifier="WAIT")
        wait_call.set_has_error()
        wait_call.set_arg("request", "NULL")
        yield tm

        replace_wait(wait_call, tm, "mpi_test")
        wait_call = tm.get_instruction(identifier="WAIT")
        wait_call.set_has_error()
        wait_call.set_arg("request", "NULL")
        tm.set_description("InvalidParam-Request-mpi_test",
                           "Invalid Request: NULL")
        yield tm


# TODO startall
class InvalidPersistentRequestUsage(ErrorGenerator):
    def __init__(self):
        pass

    def get_feature(self):
        return ["P2P"]

    def generate(self, generate_level, real_world_score_table):
        tm = get_send_recv_template("mpi_send_init", "mpi_recv_init")
        start_call = tm.get_instruction(identifier="START", rank_excuting=0)

        tm.set_description("InvalidParam-Request-mpi_start", "Invalid Param: NULL")

        start_call.set_arg("request", "NULL")
        start_call.set_has_error()
        yield tm

        tm = get_send_recv_template("mpi_send_init", "mpi_recv_init")
        start_call = tm.get_instruction(identifier="START", rank_excuting=0)

        tm.set_description("InvalidParam-Request-mpi_start", "Starting an active request again")

        second_start = MPICallFactory.mpi_start(start_call.get_arg("request"))
        second_start.set_rank_executing(0)
        second_start.set_has_error()

        tm.insert_instruction(second_start, after_instruction=start_call)

        yield tm

        tm = get_send_recv_template("mpi_send_init", "mpi_recv_init")

        tm.set_description("InvalidParam-Request-mpi_request_free", "freeing an active request")

        # move wait calls to the end
        wait_call = tm.get_instruction(identifier="WAIT", rank_excuting=0)
        tm.remove_instruction(instruction=wait_call)
        tm.register_instruction(wait_call)
        wait_call = tm.get_instruction(identifier="WAIT", rank_excuting=1)
        tm.remove_instruction(instruction=wait_call)
        tm.register_instruction(wait_call)

        for call in tm.get_instruction("FREE", return_list=True):
            if isinstance(call, MPICall):
                if call.get_function() == "mpi_request_free":
                    call.set_has_error()

        # yield tm
        # according to the standard this is permitted (although strongly discouraged) #see section 3.7.3

        tm = get_send_recv_template("mpi_send_init", "mpi_recv_init")
        start_call = tm.get_instruction(identifier="START", rank_excuting=0)

        tm.set_description("InvalidParam-Request-mpi_startall", "Starting an request twice")

        prev_req = start_call.get_arg("request")
        if prev_req.startswith("&"):
            prev_req = prev_req[1:]  # remove leading &

        tm.insert_instruction(Instruction("MPI_Request req_array[2];", rank=start_call.get_rank_executing()),
                              before_instruction=start_call)
        tm.insert_instruction(Instruction("req_array[0] = " + prev_req + ";", rank=start_call.get_rank_executing()),
                              before_instruction=start_call)
        tm.insert_instruction(Instruction("req_array[1] = " + prev_req + ";", rank=start_call.get_rank_executing()),
                              before_instruction=start_call)

        startall_call = MPICallFactory.mpi_startall(2, "req_array")
        startall_call.set_has_error()
        startall_call.set_rank_executing(start_call.get_rank_executing())
        tm.insert_instruction(startall_call, before_instruction=start_call)
        tm.remove_instruction(start_call)

        yield tm

        tm = get_send_recv_template("mpi_send_init", "mpi_recv_init")
        start_call = tm.get_instruction(identifier="START", rank_excuting=0)

        tm.set_description("InvalidParam-Request-mpi_startall", "array contains NULL")

        prev_req = start_call.get_arg("request")
        if prev_req.startswith("&"):
            prev_req = prev_req[1:]  # remove leading &

        tm.insert_instruction(Instruction("MPI_Request req_array[2];", rank=start_call.get_rank_executing()),
                              before_instruction=start_call)
        tm.insert_instruction(Instruction("req_array[0] = " + prev_req + ";", rank=start_call.get_rank_executing()),
                              before_instruction=start_call)
        tm.insert_instruction(Instruction("req_array[1] = " + "NULL" + ";", rank=start_call.get_rank_executing()),
                              before_instruction=start_call)

        startall_call = MPICallFactory.mpi_startall(2, "req_array")
        startall_call.set_has_error()
        startall_call.set_rank_executing(start_call.get_rank_executing())
        tm.insert_instruction(startall_call, before_instruction=start_call)
        tm.remove_instruction(start_call)

        yield tm
