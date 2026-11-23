#! /usr/bin/python3
from __future__ import annotations

from Infrastructure.Variables import *

from Infrastructure.ErrorGenerator import ErrorGenerator
from Infrastructure.Instruction import Instruction
from Infrastructure.MPICall import MPICall
from Infrastructure.MPICallFactory import MPICallFactory, CorrectMPICallFactory
from Infrastructure.CorrectParameter import CorrectParameterFactory
from Infrastructure.Template import TemplateManager
from Infrastructure.TemplateFactory import get_allocated_window, get_rma_call
from Infrastructure.AllocCall import AllocCall

import itertools

from typing import Tuple, List


class GlobalConcurrencyErrorRMA(ErrorGenerator):
    def __init__(self):
        self.cfmpi = CorrectMPICallFactory()

        # RMA calls that perform a local buffer access
        localbufwrite = CorrectMPICallFactory().mpi_get()
        localbufwrite.set_arg(
            "origin_addr", CorrectParameterFactory().winbuf_var_name)
        localbufwrite.set_rank_executing(1)
        localbufwrite.set_arg("origin_count", "1")
        localbufwrite.set_arg("target_count", "1")
        localbufwrite.set_arg("target_rank", "0")

        localbufread = CorrectMPICallFactory().mpi_put()
        localbufread.set_arg(
            "origin_addr", CorrectParameterFactory().winbuf_var_name)
        localbufread.set_rank_executing(1)
        localbufread.set_arg("origin_count", "1")
        localbufread.set_arg("target_count", "1")
        localbufread.set_arg("target_rank", "0")

        self.buf_instructions = {
            "bufread": Instruction(f'printf("winbuf is %d\\n", {CorrectParameterFactory().winbuf_var_name}[1]);', 1, "bufread"),
            "bufwrite": Instruction(f'{CorrectParameterFactory().winbuf_var_name}[1] = 42;', 1, "bufwrite"),
            "localbufread": localbufread,
            "localbufwrite": localbufwrite
        }

    def get_feature(self):
        return ["RMA"]

    def fence(self, tm: TemplateManager, alloc_inst: Instruction, alloc1: List[Instruction], op1: Instruction, alloc2: List[Instruction], op2: Instruction, shouldsync: bool):
        # open access epoch + sync
        tm.register_instruction(self.cfmpi.mpi_win_fence())

        tm.register_instruction(alloc_inst)
        tm.register_instruction(alloc1)
        tm.register_instruction(op1, "OP1")

        # if accesses should be synced, add fence
        if shouldsync:
            tm.register_instruction(
                self.cfmpi.mpi_win_fence(), rank_to_execute="all")

        tm.register_instruction(alloc2)
        tm.register_instruction(op2, "OP2")

        # finish access epoch + sync
        tm.register_instruction(self.cfmpi.mpi_win_fence())

        return True

    def lockall(self, tm: TemplateManager, alloc_inst: Instruction, alloc1: List[Instruction], op1: Instruction, alloc2: List[Instruction], op2: Instruction, shouldsync: bool):
        # open access epoch + sync
        tm.register_instruction(
            self.cfmpi.mpi_win_lock_all(), rank_to_execute="all")

        tm.register_instruction(alloc_inst)
        tm.register_instruction(alloc1)
        tm.register_instruction(op1, "OP1")

        tm.register_instruction(
            self.cfmpi.mpi_win_flush_all(), rank_to_execute="all")

        # if accesses should be synced, add barrier
        if shouldsync:
            tm.register_instruction(
                self.cfmpi.mpi_barrier(), rank_to_execute="all")

        tm.register_instruction(alloc2)
        tm.register_instruction(op2, "OP2")

        # finish access epoch + sync
        tm.register_instruction(
            self.cfmpi.mpi_win_unlock_all(), rank_to_execute="all")

        return True

    def lock(self, tm: TemplateManager, alloc_inst: Instruction, alloc1: List[Instruction], op1: Instruction, alloc2: List[Instruction], op2: Instruction, shouldsync: bool):
        lock0 = self.cfmpi.mpi_win_lock()
        unlock0 = self.cfmpi.mpi_win_unlock()
        lock1 = self.cfmpi.mpi_win_lock()
        unlock1 = self.cfmpi.mpi_win_unlock()
        lock0.set_arg("rank", "1")
        unlock0.set_arg("rank", "1")
        lock1.set_arg("rank", "1")
        unlock1.set_arg("rank", "1")

        if not shouldsync:
            # set lock to shared (instead of exclusive) so that no synchronization is ensured
            lock0.set_arg("lock_type", "MPI_LOCK_SHARED")
            lock1.set_arg("lock_type", "MPI_LOCK_SHARED")

        tm.register_instruction(alloc_inst)

        tm.register_instruction(lock0, rank_to_execute=0)
        tm.register_instruction(alloc1)
        tm.register_instruction(op1, "OP1")
        tm.register_instruction(unlock0, rank_to_execute=0)

        tm.register_instruction(
            lock1, rank_to_execute=op2.get_rank_executing())
        # the local buffer accesses are RMA calls to another target, which needs its own access epoch
        op2_target = op2.get_arg("target_rank") if isinstance(op2, MPICall) and op2.has_arg("target_rank") else "1"
        if op2_target != "1":
            lock_target = self.cfmpi.mpi_win_lock()
            lock_target.set_arg("rank", op2_target)
            lock_target.set_arg("lock_type", "MPI_LOCK_SHARED")
            unlock_target = self.cfmpi.mpi_win_unlock()
            unlock_target.set_arg("rank", op2_target)
            tm.register_instruction(lock_target, rank_to_execute=op2.get_rank_executing())
        tm.register_instruction(alloc2)
        tm.register_instruction(op2, "OP2")
        if op2_target != "1":
            tm.register_instruction(unlock_target, rank_to_execute=op2.get_rank_executing())
        tm.register_instruction(
            unlock1, rank_to_execute=op2.get_rank_executing())

        return True

    def rmarequest(self, tm: TemplateManager, alloc_inst: Instruction, alloc1: List[Instruction], op1: Instruction, alloc2: List[Instruction], op2: Instruction, shouldsync: bool):
        # only consider combination where the first operation is a request-based RMA call
        if not isinstance(op1, MPICall) or not op1.has_arg("request"):
            return False
        
        # we assume that the first operation is request-based
        wait = self.cfmpi.mpi_wait()
        wait.set_arg("request", op1.get_arg("request"))

        # open access epoch + sync
        tm.register_instruction(self.cfmpi.mpi_win_lock_all())

        tm.register_instruction(alloc_inst)
        tm.register_instruction(alloc1)
        tm.register_instruction(op1, "OP1")

        # if accesses should be synced, wait for local completion of operation here
        if shouldsync:
            tm.register_instruction(wait, rank_to_execute=0)

        tm.register_instruction(alloc2)
        tm.register_instruction(op2, "OP2")

        # finish access epoch + sync
        tm.register_instruction(self.cfmpi.mpi_win_unlock_all())

        return True

    def pscw(self, tm: TemplateManager, alloc_inst: Instruction, alloc1: List[Instruction], op1: Instruction, alloc2: List[Instruction], op2: Instruction, shouldsync: bool):
        tm.register_instruction("MPI_Group world_group;")
        tm.register_instruction(
            "MPI_Comm_group(MPI_COMM_WORLD, &world_group);")
        tm.register_instruction("MPI_Group mpi_group_0;")

        tm.register_instruction(alloc_inst)
    
        tm.register_instruction(
            "int destrank = 1; MPI_Group_incl(world_group, 1, &destrank, &mpi_group_0); MPI_Win_start(mpi_group_0, 0, mpi_win_0);", rank_to_execute=0)

        tm.register_instruction(alloc1)
        tm.register_instruction(op1, "OP1")

        tm.register_instruction(
                self.cfmpi.mpi_win_complete(), rank_to_execute=0)
        

        tm.register_instruction(
            "int srcrank = 0; MPI_Group_incl(world_group, 1, &srcrank, &mpi_group_0);", rank_to_execute=op2.get_rank_executing())
        tm.register_instruction(
            "MPI_Win_post(mpi_group_0, 0, mpi_win_0);", rank_to_execute=op2.get_rank_executing())

        # if accesses should be synced, end exposure epoch here
        if shouldsync:
            tm.register_instruction(
                self.cfmpi.mpi_win_wait(), rank_to_execute=op2.get_rank_executing())

        tm.register_instruction(alloc2)
        tm.register_instruction(op2, "OP2")

        # if accesses should not be synced, end access epoch here
        if not shouldsync:
            tm.register_instruction(
                self.cfmpi.mpi_win_wait(), rank_to_execute=op2.get_rank_executing())

        return True

    def get_mem_op(self, name: str, rank) -> Tuple[List[Instruction], Instruction, Instruction | None]:
        if name.startswith("mpi"):
            return get_rma_call(self.tm, name, rank, name.replace("mpi_", ""))
        else:
            return ([], self.buf_instructions[name], None)

    def generate(self, generate_level, real_world_score_table):

        if generate_level == 1:
            # only basic calls
            remote_read = ["mpi_get"]
            remote_write = ["mpi_put"]
            remote_atomic_update = ["mpi_accumulate"]
        else:
            # everything
            remote_read = ["mpi_get", "mpi_rget"]
            remote_write = [
                "mpi_put",
                "mpi_rput",
            ]
            remote_atomic_update = [
                "mpi_accumulate",
                "mpi_raccumulate",
                "mpi_get_accumulate",
                "mpi_rget_accumulate",
                "mpi_fetch_and_op",
                "mpi_compare_and_swap",
            ]

        cf = CorrectParameterFactory()


        if generate_level <= BASIC_TEST_LEVEL:
            # possible combinations of remote / local accesses (hasconflict = True | False)
            remote_access_combinations: List[Tuple[List[str], List[str], bool]] = [
                (remote_read, ["bufwrite", "localbufwrite"], True),
                (remote_read, remote_write, True),
                (remote_write, remote_write, True),
                # atomics
                (remote_atomic_update, remote_atomic_update, False),
                (remote_atomic_update, remote_write, True),
            ]
            sync_modes = [self.fence, self.lockall, self.lock]
        else:
            # possible combinations of remote / local accesses (hasconflict = True | False)
            remote_access_combinations: List[Tuple[List[str], List[str], bool]] = [
                (remote_read, remote_read, False),
                (remote_read, ["bufread", "localbufread"], False),
                (remote_read, ["bufwrite", "localbufwrite"], True),
                (remote_read, remote_write, True),
                (remote_write, ["bufread", "localbufread"], True),
                (remote_write, ["bufwrite", "localbufwrite"], True),
                (remote_write, remote_write, True),
                # atomics
                (remote_atomic_update, remote_atomic_update, False),
                (remote_atomic_update, remote_read, True),
                (remote_atomic_update, remote_write, True),
                (remote_atomic_update, ["bufread", "localbufread"], True),
                (remote_atomic_update, ["bufwrite", "localbufwrite"], True),
            ]
            sync_modes = [self.fence, self.lockall, self.lock, self.rmarequest]

        if generate_level <= SUFFICIENT_TEST_LEVEL:
            # go through all sync modes, but only one access combination per sync mode
            combos = [(comb, sync_modes[i % len(sync_modes)]) for (i, comb) in enumerate(remote_access_combinations)]
        else:
            # combine everything (= nested for loop)
            combos = itertools.product(remote_access_combinations, sync_modes)

        for (ops1, ops2, hasconflict), sync_mode in combos:
            for shouldsync in [False, True]:
                for (op1, op2) in itertools.product(ops1, ops2):
                    # a third rank is only needed when both accesses are remote ones
                    self.tm = TemplateManager(min_ranks=2 if op2 in self.buf_instructions else 3)
                    (win_declare, win_alloc, win_free) = get_allocated_window(
                        "mpi_win_create", cf.get("win"), cf.winbuf_var_name, "int", "10")
                    # window allocation boilerplate
                    self.tm.register_instruction(win_declare)
                    self.tm.register_instruction(win_alloc)

                    # local buffer allocation, can be used by calls from different ranks
                    alloc_inst = AllocCall(
                        cf.dtype[0], cf.buf_size, cf.buf_var_name, use_malloc=False, identifier="alloc", rank="all")
                    op1_name = op1.replace("mpi_", "")
                    op2_name = op2.replace("mpi_", "")

                    alloc1, inst1, inst1_free = self.get_mem_op(op1, 0)
                    alloc2, inst2, inst2_free = self.get_mem_op(op2, 2)

                    # if the operations are not conflicting and we should sync, we do not have to generate this test case
                    if not hasconflict and shouldsync:
                        continue

                    # if the operations are conflicting *and* we perform no synchronization between them, we have a race
                    if hasconflict and not shouldsync:
                        inst1.set_has_error(True)
                        inst2.set_has_error(True)
                    else:
                        inst1.set_has_error(False)
                        inst2.set_has_error(False)

                    # generate code for the given sync_mode
                    valid_case = sync_mode(self.tm, alloc_inst, alloc1, inst1, alloc2, inst2, shouldsync)

                    if not valid_case:
                        # this case is not possible / redundant for this sync_mode, continue
                        continue

                    # finalize RMA call (if needed)
                    if inst1_free is not None:
                        self.tm.register_instruction(inst1_free)
                    if inst2_free is not None:
                        self.tm.register_instruction(inst2_free)

                    # window free boilerplate
                    self.tm.register_instruction(win_free)

                    self.tm.set_description(
                        ("GlobalConcurrency" if hasconflict and not shouldsync else "Correct") +
                        "-"
                        + sync_mode.__name__
                        + "-"
                        + op1_name
                        + "_"
                        + op2_name,
                        "full description",
                    )
                    yield self.tm
