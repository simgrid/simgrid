#! /usr/bin/python3
from Infrastructure.Variables import *

from Infrastructure.CorrectParameter import CorrectParameterFactory
from Infrastructure.ErrorGenerator import ErrorGenerator
from Infrastructure.MPICallFactory import CorrectMPICallFactory
from Infrastructure.Template import TemplateManager


class MessageRaceErrorAnyTag(ErrorGenerator):
    # TODO do we need to generate it for all combinations of send and recv?

    def __init__(self):
        pass

    def get_feature(self):
        return ["P2P"]

    def generate(self, generate_level, real_world_score_table):
        tm = TemplateManager()
        tm.set_description("CallOrdering-ANY_TAG", "order of messages is indeterministic, may lead to a deadlock")
        tm.set_can_deadlock()

        tm.register_instruction(CorrectParameterFactory().get_buffer_alloc())


        # send part
        tm.register_instruction("for(int i =0; i < 10; ++i) {", rank_to_execute=1)
        tm.register_instruction("buf[0]=i;", rank_to_execute=1)
        send_call = CorrectMPICallFactory().mpi_send()
        send_call.set_arg("tag", "i")
        tm.register_instruction(send_call, rank_to_execute=1)
        tm.register_instruction("}", rank_to_execute=1)

        # the final msg after the loop
        send_call = CorrectMPICallFactory().mpi_send()

        tm.register_instruction(send_call, rank_to_execute=1)
        # recv part
        tm.register_instruction("for(int i =0; i < 10; ++i) {", rank_to_execute=0)
        recv_call = CorrectMPICallFactory().mpi_recv()
        recv_call.set_arg("tag", "MPI_ANY_TAG")
        recv_call.set_rank_executing(0)
        tm.register_instruction(recv_call)

        tm.register_instruction("if(buf[0]!=i){", rank_to_execute=0)
        additional_recv = CorrectMPICallFactory().mpi_recv()
        additional_recv.set_has_error()  # additional recv may lead to deadlock
        tm.register_instruction(additional_recv,rank_to_execute=0)
        tm.register_instruction(" }", rank_to_execute=0)  # end if
        tm.register_instruction("}", rank_to_execute=0)  # end for

        tm.register_instruction(CorrectParameterFactory().get_buffer_free())

        yield tm


class MessageRaceErrorAnysource(ErrorGenerator):
    # TODO do we need to generate it for all combinations of send and recv?

    def __init__(self):
        pass

    def get_feature(self):
        return ["P2P"]

    def generate(self, generate_level, real_world_score_table):
        tm = TemplateManager(min_ranks=3)
        tm.set_description("CallOrdering-ANY_SOURCE", "order of messages is indeterministic, may lead to a deadlock")
        tm.set_can_deadlock()

        tm.register_instruction(CorrectParameterFactory().get_buffer_alloc())

        # send part
        tm.register_instruction("buf[0]=rank;", rank_to_execute='not0')
        send_call = CorrectMPICallFactory().mpi_send()
        tm.register_instruction(send_call, rank_to_execute='not0')
        # rank 1 sends an additional msg
        send_call = CorrectMPICallFactory().mpi_send()
        tm.register_instruction(send_call, rank_to_execute=1)

        # recv part
        tm.register_instruction("for(int i =1; i < nprocs; ++i) {", rank_to_execute=0)
        recv_call = CorrectMPICallFactory().mpi_recv()
        recv_call.set_arg("source", "MPI_ANY_SOURCE")
        tm.register_instruction(recv_call, rank_to_execute=0)
        tm.register_instruction("if(buf[0]!=i){", rank_to_execute=0)
        additional_recv = CorrectMPICallFactory().mpi_recv()
        additional_recv.set_has_error()  # additional recv leads to deadlock
        tm.register_instruction(additional_recv, rank_to_execute=0)
        tm.register_instruction(" }", rank_to_execute=0)  # end if
        tm.register_instruction("}", rank_to_execute=0)  # end for

        tm.register_instruction(CorrectParameterFactory().get_buffer_free())


        yield tm
