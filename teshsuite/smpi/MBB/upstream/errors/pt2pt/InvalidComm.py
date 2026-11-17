#! /usr/bin/python3
from Infrastructure.Variables import *
from Infrastructure.ErrorGenerator import ErrorGenerator
from Infrastructure.Instruction import Instruction
from Infrastructure.MPICallFactory import MPICallFactory
from Infrastructure.TemplateFactory import get_communicator, get_intercomm, \
    get_invalid_param_p2p_case

sendrecv_funcs = ["mpi_sendrecv", "mpi_sendrecv_replace"]


class InvalidCommErrorP2P(ErrorGenerator):
    invalid_comm = ["MPI_COMM_NULL", "NULL"]

    missmatching_comms = ["MPI_COMM_SELF", "mpi_comm_dup", "mpi_comm_dup_with_info", "mpi_comm_idup",
                          "mpi_comm_idup_with_info", "mpi_comm_create", "mpi_comm_create_group", "mpi_comm_split",
                          "mpi_comm_split_type", "mpi_comm_create_from_group"
                          ]
    intercomms = ["mpi_intercomm_create", "mpi_intercomm_merge", "mpi_intercomm_create_from_groups"]

    # as extended testcases

    comms_to_check = invalid_comm + missmatching_comms + intercomms

    mprobe_funcs = ["mpi_mprobe", "mpi_improbe"]
    probe_funcs = ["mpi_probe", "mpi_iprobe"]

    functions_to_check = ["mpi_send",
                          "mpi_recv", "mpi_irecv",
                          "mpi_isend", "mpi_ssend", "mpi_issend", "mpi_rsend", "mpi_irsend", "mpi_bsend", "mpi_ibsend",
                          "mpi_send_init", "mpi_ssend_init", "mpi_bsend_init", "mpi_rsend_init", "mpi_psend_init",
                          "mpi_precv_init", "mpi_recv_init"
                          ] + sendrecv_funcs + mprobe_funcs + probe_funcs

    recv_funcs = ["mpi_recv", "mpi_irecv", "mpi_recv_init",
                  "mpi_precv_init"] + sendrecv_funcs + mprobe_funcs + probe_funcs

    def __init__(self):
        pass

    def get_feature(self):
        return ["P2P"]

    def generate(self, generate_level, real_world_score_table):
        for func in self.functions_to_check:
            if func in self.recv_funcs:
                check_recv = True
                send_func = "mpi_send"
                recv_func = func
            else:
                check_recv = False
                send_func = func
                recv_func = "mpi_irecv"
            if func in self.mprobe_funcs:
                send_func = "mpi_send"
                recv_func = [func, "mpi_mrecv"]
            if func in self.probe_funcs:
                send_func = "mpi_send"
                recv_func = "mpi_irecv"
                # not implemented
                continue
                # TODO add probe call
            if send_func == "mpi_psend_init":
                recv_func = "mpi_precv_init"
            if recv_func == "mpi_precv_init":
                send_func = "mpi_psend_init"

            i = 0
            for comm_to_use in self.comms_to_check:
                if comm_to_use in self.missmatching_comms + self.intercomms and recv_func == "mpi_irecv" and generate_level < FULL_TEST_LEVEL:
                    # combination repeated
                    continue
                i += 1

                tm = get_invalid_param_p2p_case("comm", comm_to_use, check_recv, send_func, recv_func)

                tm.set_can_deadlock()# deadlock if msg is send one one comm and not received on the other comm
                # deadlock can be considered  to occur even if comm=NULL as send/receive will not be matched

                error_string = "GlobalParameterMissmatch"
                if comm_to_use in self.invalid_comm:
                    error_string = "InvalidParam"
                tm.set_description(error_string + "-Comm-" + func, error_string + ": %s" % comm_to_use)

                if comm_to_use in self.missmatching_comms and comm_to_use != "MPI_COMM_SELF":
                    comm_var_name = get_communicator(comm_to_use, tm)
                    #  change the arg in the MPI call to the result variable name
                    for call in tm.get_instruction(identifier="MPICALL", return_list=True):
                        if call.get_arg("comm") == comm_to_use:
                            call.set_arg("comm", comm_var_name)

                if comm_to_use in self.intercomms:
                    comm_var_name = get_intercomm(comm_to_use, tm)
                    for call in tm.get_instruction(identifier="MPICALL", return_list=True):
                        if call.get_arg("comm") == comm_to_use:
                            call.set_arg("comm", comm_var_name)

                # if intercomm: set rank to 0 instead of 1 as ther is only one rank in intercomm
                if comm_to_use in self.intercomms and not comm_to_use == "mpi_intercomm_merge":
                    for call in tm.get_instruction(identifier="MPICALL", return_list=True):
                        if call.has_arg("source"):
                            call.set_arg("source", "0")

                error_string = "ParamMatching"
                if comm_to_use in self.invalid_comm:
                    error_string = "InvalidParam"
                yield tm
                if generate_level <= BASIC_TEST_LEVEL and i > 3:
                    return
            # end for comm to check
        # end for send_func in funcs_to_check
