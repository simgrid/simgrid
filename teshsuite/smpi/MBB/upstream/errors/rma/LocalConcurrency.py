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


class LocalConcurrencyErrorRMA(ErrorGenerator):
    def __init__(self):
        self.cfmpi = CorrectMPICallFactory()
        # generate standard buffer access instructions
        self.buf_instructions = {
            "bufread": Instruction(f'printf("buf is %d\\n", {CorrectParameterFactory().buf_var_name}[0]);', 0, "bufread"),
            "bufwrite": Instruction(f'{CorrectParameterFactory().buf_var_name}[0] = 42;', 0, "bufwrite")
        }

    def get_feature(self):
        return ["RMA"]

    def fence(self, tm: TemplateManager, alloc_inst: Instruction, alloc1: List[Instruction], op1: Instruction, alloc2: List[Instruction], op2: Instruction, shouldsync: bool):
        # open access epoch + sync
        tm.register_instruction(self.cfmpi.mpi_win_fence())

        tm.register_instruction(alloc_inst)
        tm.register_instruction(alloc1)
        tm.register_instruction(op1, "OP1")

        # if accesses should be synced, add another fence (rank 0)
        if shouldsync:
            tm.register_instruction(
                self.cfmpi.mpi_win_fence(), rank_to_execute=0)

        tm.register_instruction(alloc2)
        tm.register_instruction(op2, "OP2")

        # if accesses should be synced, add another fence (rank 1)
        if shouldsync:
            tm.register_instruction(
                self.cfmpi.mpi_win_fence(), rank_to_execute=1)

        # finish access epoch + sync
        tm.register_instruction(self.cfmpi.mpi_win_fence())

        return True

    def lockallflush(self, tm: TemplateManager, alloc_inst: Instruction, alloc1: List[Instruction], op1: Instruction, alloc2: List[Instruction], op2: Instruction, shouldsync: bool):
        # open access epoch + sync
        tm.register_instruction(self.cfmpi.mpi_win_lock_all())

        tm.register_instruction(alloc_inst)
        tm.register_instruction(alloc1)
        tm.register_instruction(op1, "OP1")

        # if accesses should be synced, add flush
        if shouldsync:
            tm.register_instruction(
                self.cfmpi.mpi_win_flush_all(), rank_to_execute=0)

        tm.register_instruction(alloc2)
        tm.register_instruction(op2, "OP2")

        # finish access epoch + sync
        tm.register_instruction(self.cfmpi.mpi_win_unlock_all())

        return True
    
    def lockallflushlocal(self, tm: TemplateManager, alloc_inst: Instruction, alloc1: List[Instruction], op1: Instruction, alloc2: List[Instruction], op2: Instruction, shouldsync: bool):
        # should sync: MPI_Win_lock_all - op1 - MPI_Win_flush_local_all - op2 - MPI_Win_unlock_all
        # shold not sync: MPI_Win_lock_all - op1 - op2 - MPI_Win_unlock_all

        # open access epoch + sync
        tm.register_instruction(self.cfmpi.mpi_win_lock_all())

        tm.register_instruction(alloc_inst)
        tm.register_instruction(alloc1)
        tm.register_instruction(op1, "OP1")

        # if accesses should be synced, add flush_local
        if shouldsync:
            tm.register_instruction(
                self.cfmpi.mpi_win_flush_local_all(), rank_to_execute=0)

        tm.register_instruction(alloc2)
        tm.register_instruction(op2, "OP2")

        # finish access epoch + sync
        tm.register_instruction(self.cfmpi.mpi_win_unlock_all())

        return True
    
    def lockunlock(self, tm: TemplateManager, alloc_inst: Instruction, alloc1: List[Instruction], op1: Instruction, alloc2: List[Instruction], op2: Instruction, shouldsync: bool):
        # should sync: MPI_Win_lock - op1 - MPI_Win_unlock - op2
        # shold not sync: MPI_Win_lock - op1 - op2 - MPI_Win_unlock

        lock = self.cfmpi.mpi_win_lock()
        unlock = self.cfmpi.mpi_win_unlock()
        lock.set_arg("rank", "1")
        unlock.set_arg("rank", "1")

        tm.register_instruction(lock, rank_to_execute=0)

        tm.register_instruction(alloc_inst)
        tm.register_instruction(alloc1)
        tm.register_instruction(op1, "OP1")

        # if accesses should be synced, add flush here
        if shouldsync:
            tm.register_instruction(unlock, rank_to_execute=0)
            tm.register_instruction(alloc2)
            tm.register_instruction(op2, "OP2")
        else:
            tm.register_instruction(alloc2)
            tm.register_instruction(op2, "OP2")
            tm.register_instruction(unlock, rank_to_execute=0)

        return True

    def lockflush(self, tm: TemplateManager, alloc_inst: Instruction, alloc1: List[Instruction], op1: Instruction, alloc2: List[Instruction], op2: Instruction, shouldsync: bool):
        lock = self.cfmpi.mpi_win_lock()
        flush = self.cfmpi.mpi_win_flush()
        unlock = self.cfmpi.mpi_win_unlock()
        lock.set_arg("rank", "1")
        flush.set_arg("rank", "1")
        unlock.set_arg("rank", "1")

        tm.register_instruction(lock, rank_to_execute=0)

        tm.register_instruction(alloc_inst)
        tm.register_instruction(alloc1)
        tm.register_instruction(op1, "OP1")

        # if accesses should be synced, add flush here
        if shouldsync:
            tm.register_instruction(flush, rank_to_execute=0)

        tm.register_instruction(alloc2)
        tm.register_instruction(op2, "OP2")

        # finish access epoch + sync
        tm.register_instruction(unlock, rank_to_execute=0)

        return True
    
    def lockflushlocal(self, tm: TemplateManager, alloc_inst: Instruction, alloc1: List[Instruction], op1: Instruction, alloc2: List[Instruction], op2: Instruction, shouldsync: bool):
        lock = self.cfmpi.mpi_win_lock()
        flush_local = self.cfmpi.mpi_win_flush_local()
        unlock = self.cfmpi.mpi_win_unlock()
        lock.set_arg("rank", "1")
        flush_local.set_arg("rank", "1")
        unlock.set_arg("rank", "1")

        tm.register_instruction(lock, rank_to_execute=0)

        tm.register_instruction(alloc_inst)
        tm.register_instruction(alloc1)
        tm.register_instruction(op1, "OP1")

        # if accesses should be synced, add flush here
        if shouldsync:
            tm.register_instruction(flush_local, rank_to_execute=0)

        tm.register_instruction(alloc2)
        tm.register_instruction(op2, "OP2")

        # finish access epoch + sync
        tm.register_instruction(unlock, rank_to_execute=0)

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

        # if accesses should be synced, end access epoch here
        if shouldsync:
            tm.register_instruction(
                self.cfmpi.mpi_win_complete(), rank_to_execute=0)

        tm.register_instruction(alloc2)
        tm.register_instruction(op2, "OP2")

        # if accesses should not be synced, end access epoch here
        if not shouldsync:
            tm.register_instruction(
                self.cfmpi.mpi_win_complete(), rank_to_execute=0)

        tm.register_instruction(
            "int srcrank = 0; MPI_Group_incl(world_group, 1, &srcrank, &mpi_group_0);", rank_to_execute=1)
        tm.register_instruction(
            "MPI_Win_post(mpi_group_0, 0, mpi_win_0);", rank_to_execute=1)
        tm.register_instruction(
            self.cfmpi.mpi_win_wait(), rank_to_execute=1)

        return True

    def get_mem_op(self, name: str, rank) -> Tuple[List[Instruction], Instruction, Instruction | None]:
        if name.startswith("mpi"):
            return get_rma_call(self.tm, name, rank, name.replace("mpi_", ""))
        else:
            return ([], self.buf_instructions[name], None)

    def generate(self, generate_level, real_world_score_table):
        # build set of calls based on generate level, for level 1 just a few basic calls,
        # for level >= 2 all calls

        if generate_level <= BASIC_TEST_LEVEL:
            # only basic calls
            local_origin_addr_read = ["mpi_put", "mpi_accumulate"]
            local_origin_addr_write = ["mpi_get"]
        else:
            # everything
            local_origin_addr_read = ["mpi_put", "mpi_accumulate", "mpi_rput",
                                      "mpi_raccumulate",
                                      "mpi_get_accumulate",
                                      "mpi_rget_accumulate",
                                      "mpi_fetch_and_op",
                                      "mpi_compare_and_swap"]
            local_origin_addr_write = ["mpi_get", "mpi_rget"]

        cf = CorrectParameterFactory()


        if generate_level <= BASIC_TEST_LEVEL:
            # possible combinations of local buffer accesses (hasconflict = True | False)
            local_access_combinations: List[Tuple[List[str], List[str], bool]] = [
                # buffer access before RMA access, no conflict
                (["bufread"], local_origin_addr_write, False),
                # buffer access after RMA access, conflict depends on combination
                (local_origin_addr_read, ["bufread"], False),
                (["mpi_rget"], ["bufwrite"], True),
                (["mpi_get"], ["bufwrite"], True),
                # combinations of RMA reads / writes
                (local_origin_addr_read, local_origin_addr_write, True),
                (local_origin_addr_write, local_origin_addr_write, True),
            ]
            sync_modes = [self.lockallflush, self.fence, self.rmarequest, self.lockunlock]
        else:
            # possible combinations of local buffer accesses (hasconflict = True | False)
            local_access_combinations: List[Tuple[List[str], List[str], bool]] = [
                # buffer access before RMA access, no conflict
                (["bufread"], local_origin_addr_read,  False),
                (["bufwrite"], local_origin_addr_read,  False),
                (["bufread"], local_origin_addr_write, False),
                (["bufwrite"], local_origin_addr_write, False),
                # buffer access after RMA access, conflict depends on combination
                (local_origin_addr_read, ["bufread"], False),
                (local_origin_addr_read, ["bufwrite"], True),
                (local_origin_addr_write, ["bufread"], True),
                (local_origin_addr_write, ["bufwrite"], True),
                # combinations of RMA reads / writes
                (local_origin_addr_read, local_origin_addr_read, False),
                (local_origin_addr_read, local_origin_addr_write, True),
                (local_origin_addr_write, local_origin_addr_read, True),
                (local_origin_addr_write, local_origin_addr_write, True),
            ]
            sync_modes = [self.fence, self.lockallflush, self.lockallflushlocal, self.lockflush, self.lockflushlocal, self.lockunlock, self.rmarequest]


        if generate_level <= SUFFICIENT_TEST_LEVEL:
            # go through all sync modes, but only one access combination per sync mode
            combos = [(comb, sync_modes[i % len(sync_modes)]) for (i, comb) in enumerate(local_access_combinations)]
        else:
            # combine everything (= nested for loop)
            combos = itertools.product(local_access_combinations, sync_modes)

        for (ops1, ops2, hasconflict), sync_mode in combos:
            for shouldsync in [False, True]:
                for (op1, op2) in itertools.product(ops1, ops2):
                    self.tm = TemplateManager()
                    (win_declare, win_alloc, win_free) = get_allocated_window(
                        "mpi_win_create", cf.get("win"), cf.winbuf_var_name, "int", "20")
                    # window allocation boilerplate
                    self.tm.register_instruction(win_declare)
                    self.tm.register_instruction(win_alloc)

                    # local buffer allocation
                    alloc_inst = AllocCall(
                        cf.dtype[0], cf.buf_size, cf.buf_var_name, use_malloc=False, identifier="alloc", rank=0)
                    op1_name = op1.replace("mpi_", "")
                    op2_name = op2.replace("mpi_", "")

                    alloc1, inst1, inst1_free = self.get_mem_op(op1, 0)
                    alloc2, inst2, inst2_free = self.get_mem_op(op2, 0)

                    if sync_mode not in [self.lockflush, self.lockflushlocal, self.lockunlock]:
                        # avoid race at the target rank by setting the target rank to 0 for the second operation
                        if isinstance(inst2, MPICall):
                            inst2.set_arg("target_rank", "0")
                    else:
                        # cannot set target rank to 0, because this would be not allowed with exclusive locks here
                        # avoid race at the target rank by setting the target disp to 10 for the second operation
                        if isinstance(inst2, MPICall):
                            inst2.set_arg("target_disp", "10")

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

                    # finalize RMA call (if needed).
                    # Avoid unmatched wait if its correct request-based rma, as then wait already happens in per-rank branch.
                    if inst1_free is not None and not (sync_mode == self.rmarequest and shouldsync):
                        self.tm.register_instruction(inst1_free)
                    if inst2_free is not None:
                        self.tm.register_instruction(inst2_free)

                    # window free boilerplate
                    self.tm.register_instruction(win_free)

                    self.tm.set_description(
                        ("LocalConcurrency" if hasconflict and not shouldsync else "Correct") +
                        "-"
                        + sync_mode.__name__
                        + "-"
                        + op1_name
                        + "_"
                        + op2_name,
                        "full description",
                    )
                    yield self.tm
