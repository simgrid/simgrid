#! /usr/bin/python3
from __future__ import annotations
from Infrastructure.Variables import *

from Infrastructure.ErrorGenerator import ErrorGenerator
from Infrastructure.Instruction import Instruction

from Infrastructure.MPICallFactory import CorrectMPICallFactory
from Infrastructure.TemplateFactory import get_invalid_param_rma_case, get_rma_template
from Infrastructure.MPICall import MPICall

import copy


class EpochLifeCycleRMA(ErrorGenerator):
    def __init__(self):
        pass

    def get_feature(self):
        return ["RMA"]

    def generate(self, generate_level, real_world_score_table):
        if generate_level <= BASIC_TEST_LEVEL:
            sync_modes = ["fence"]
        else:
            sync_modes = ["fence", "winlockall", "winlock"]

        for sync_mode in sync_modes:
            for rma_func in ["mpi_get", "mpi_put", "mpi_accumulate"]:
                # epoch is not closed
                tm = get_rma_template(rmaop_func=rma_func, sync_mode=sync_mode)
                tm.remove_instruction("ENDRMAEPOCH")
                # we set the corresponding RMA call as erroneous operation since it is not completed correctly
                tm.get_instruction("RMAOP").set_has_error()
                tm.set_description("EpochLifeCycle", "RMA epoch not closed")
                tm.set_can_deadlock()
                yield tm

                # epoch is not opened
                tm = get_rma_template(rmaop_func=rma_func, sync_mode=sync_mode)
                tm.remove_instruction("STARTRMAEPOCH")
                # we set the corresponding RMA call as erroneous operation since it is not completed correctly
                tm.get_instruction("RMAOP").set_has_error()
                tm.set_description("EpochLifeCycle", "RMA epoch not closed")
                tm.set_can_deadlock()
                yield tm

                # double open of epoch
                tm = get_rma_template(rmaop_func=rma_func, sync_mode=sync_mode)
                # workaround to double epoch instruction
                startrmaepoch = copy.deepcopy(tm.get_instruction("STARTRMAEPOCH"))
                startrmaepoch.set_rank_executing(0)
                startrmaepoch.set_has_error()
                tm.insert_instruction(new_instruction=startrmaepoch, after_instruction="STARTRMAEPOCH")
                tm.set_description("EpochLifeCycle", "RMA epoch opened twice")
                tm.set_can_deadlock()
                yield tm
                if generate_level <= BASIC_TEST_LEVEL:
                    break

        for rma_func in ["mpi_get", "mpi_put", "mpi_accumulate"]:
            # mix fence with lockall, this should not be done at all
            tm = get_rma_template(rmaop_func=rma_func, sync_mode="fence")
            lock_all = CorrectMPICallFactory.mpi_win_lock_all()
            lock_all.set_has_error()
            unlock_all = CorrectMPICallFactory.mpi_win_unlock_all()
            tm.insert_instruction(new_instruction=lock_all, after_instruction="STARTRMAEPOCH")
            tm.insert_instruction(new_instruction=unlock_all, before_instruction="ENDRMAEPOCH")
            tm.set_description("EpochLifeCycle", "Mixing fence with lock_all synchronization")
            tm.set_can_deadlock()
            yield tm

            # mix fence with lock, this should not be done at all
            tm = get_rma_template(rmaop_func=rma_func, sync_mode="fence")
            lock = CorrectMPICallFactory.mpi_win_lock()
            lock.set_has_error()
            lock.set_rank_executing(0)
            unlock = CorrectMPICallFactory.mpi_win_unlock()
            unlock.set_rank_executing(0)
            tm.insert_instruction(new_instruction=lock, after_instruction="STARTRMAEPOCH")
            tm.insert_instruction(new_instruction=unlock, before_instruction="ENDRMAEPOCH")
            tm.set_description("EpochLifeCycle", "Mixing fence with lock synchronization")
            tm.set_can_deadlock()
            yield tm
            if generate_level <= BASIC_TEST_LEVEL:
                break
