#! /usr/bin/python3
from Infrastructure.Instruction import Instruction
from Infrastructure.Variables import *

from Infrastructure.ErrorGenerator import ErrorGenerator
from Infrastructure.Template import TemplateManager
from Infrastructure.TemplateFactory import get_send_recv_template


class UnmatchedP2Pcall(ErrorGenerator):
    send_funcs = ["mpi_send",
                  "mpi_isend", "mpi_ssend", "mpi_issend", "mpi_rsend", "mpi_irsend", "mpi_bsend", "mpi_ibsend",
                  "mpi_send_init", "mpi_ssend_init", "mpi_bsend_init", "mpi_rsend_init", "mpi_psend_init"
                  ]
    recv_funcs = ["mpi_recv", "mpi_irecv", "mpi_recv_init", "mpi_precv_init"]
    probe_recv_funcs = ["mpi_mprobe", "mpi_improbe"]

    def __init__(self):
        pass

    def get_feature(self):
        return ["P2P"]

    def generate_impl(self, func_to_check, check_receive):

        send_func = "mpi_send"
        recv_func = "mpi_irecv"
        if check_receive:
            recv_func = func_to_check
            if func_to_check in self.probe_recv_funcs:
                recv_func = [func_to_check, "mpi_mrecv"]
        else:
            send_func = func_to_check
        if send_func == "mpi_psend_init":
            recv_func = "mpi_precv_init"
        if recv_func == "mpi_precv_init":
            send_func = "mpi_psend_init"

        tm = get_send_recv_template(send_func, recv_func)

        tm.set_description("Correct-" + func_to_check,
                           "correct message exchange")
        yield tm

        tm.set_description("CallOrdering-unmatched-" + func_to_check,
                           "The %s does not have a counterpart" % ("receive" if check_receive else "send"))
        tm.set_can_deadlock()

        calls = tm.get_instruction(identifier="MPICALL", return_list=True)
        for call in calls:
            call.set_has_error()

        # remove send/recv COUNTER-part
        kind = 0
        if check_receive:
            kind = 1
            # comm calls and the corresponding wait calls
        for call in calls + tm.get_instruction(identifier="WAIT", return_list=True):
            if call.get_rank_executing() == kind:
                tm.remove_instruction(instruction=call)
        yield tm

    def generate(self, generate_level, real_world_score_table):
        for func in self.recv_funcs + self.probe_recv_funcs:
            yield from self.generate_impl(func, True)
            if generate_level <= BASIC_TEST_LEVEL:
                return
        for func in self.send_funcs:
            yield from self.generate_impl(func, False)


class ComplexMissmach(ErrorGenerator):

    def __init__(self):
        pass

    def get_feature(self):
        return ["P2P"]

    def generate(self, generate_level, real_world_score_table):
        code = """
#define MSG_TAG_A 124523
#define N 10
#define EVEN 0

  int buffer[N];
  int i;

  MPI_Request request;
  MPI_Status status;

  int countEvenNumbers = 0;

  for (i = 0; i < 10; i++) {
    if (rank == 0) {
      int tag_sender = i * N;
      MPI_Isend(buffer, 1, MPI_INT, 1, tag_sender, MPI_COMM_WORLD, &request);
      MPI_Wait(&request, &status);
    }

    else if (rank == 1) {
        int tag_receiver = i * N;
        
        if (i % 2 == EVEN) {
        (countEvenNumbers)++;
        }

        if ((countEvenNumbers) == (N / 2)) {
            tag_receiver++; // mismatch
        }

      printf(\"Count Even Numbers: %d \\n\", countEvenNumbers);
      MPI_Irecv(buffer, 1, MPI_INT, 0, tag_receiver, MPI_COMM_WORLD, &request);
      MPI_Wait(&request, &status);
    }
  }
"""
        tm = TemplateManager()
        i = Instruction(code)
        i.set_has_error()
        tm.register_instruction(i)

        tm.set_can_deadlock()

        #TODO SET_HAS_ERROR

        tm.set_description("GlobalParameterMissmatch-tag-mpi_send", "Missmatching message tags in iteration 10")
        yield tm
