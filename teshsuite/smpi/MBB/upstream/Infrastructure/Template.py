#! /usr/bin/python3
from __future__ import annotations

import typing

from Infrastructure.Instruction import Instruction
from Infrastructure.MPICall import MPICall
from Infrastructure.CorrectParameter import CorrectParameterFactory
from Infrastructure.Variables import ERROR_CLASSES

deadlock_marker = "\n\n This testcase can result in a Deadlock\n"

"""
THIS BLOCK IS CURRENTLY NOT USED:
BEGIN_MPI_FEATURES
  P2P!basic: @{p2pfeature}@
  P2P!nonblocking: @{ip2pfeature}@
  P2P!persistent: @{persfeature}@
  COLL!basic: Lacking
  COLL!nonblocking: Lacking
  COLL!persistent: Lacking
  COLL!tools: Yes
  RMA: Lacking
END_MPI_FEATURES
"""

template = """/* ///////////////////////// The MPI Bug Bench ////////////////////////

  Description: @{desc}@

  Version of MPI: @{version}@

  Category: @{category}@

BEGIN_MBB_TESTS
  $ mpirun -np @{min_num_ranks}@ ${EXE}
  | @{outcome}@
  | @{errormsg}@
END_MBB_TESTS
//////////////////////       End of MBI headers        /////////////////// */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

int main(int argc, char **argv) {
  int nprocs = -1;
  int rank = -1;
  @{stack_vars}@

  @{mpi_init}@
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if (nprocs < @{min_num_ranks}@)
    printf("MBB ERROR: This test needs at least @{min_num_ranks}@ processes to produce a bug!\\n");

  @{test_code}@

  @{mpi_finalize}@
  printf("Rank %d finished normally\\n", rank);
  return 0;
}
"""

init_thread_template = """int provided;
MPI_Init_thread(&argc, &argv, @{thread_level}@, &provided);
if (@{thread_level}@ < provided)
    printf("MBI ERROR: The MPI Implementation does not provide the required thread level!\\n");
"""


class TemplateManager:
    """
    Class Overview:
        The `TemplateManager` class is responsible for managing MPI error case templates.

    Attributes:
        _descr_full (str): The full description of the template.
        _descr_short (str): The short description of the template.
        _instructions (list): List of instruction blocks.
        _thread_level (str): The MPI thread level to use.
        _min_ranks (int): The minimum number of MPI ranks.
        _has_finalize (bool): Whether to include a call to MPI_Finalize.
        _has_init (bool): Whether to include a call to MPI_Init.

    Methods:
        - __init__(self, min_ranks: int = 2, thread_level: str = None, has_finalize: bool = True,
                 has_init: bool = True): Initializes a new instance of the TemplateManager class.
        - __str__(self): Converts the TemplateManager instance to a string, replacing placeholders.
        - register_instruction(self, inst: Instruction | typing.List[Instruction]): Registers an instruction with the template.
        - get_version(self) -> str: Retrieves the minimum required MPI version.
        - set_description(self, descr_short: str, descr_full: str): Sets the short and full descriptions for the template.
        - get_short_descr(self) -> str: Retrieves the short description (to use as a filename).
        - get_instruction(self, identifier: str = None, return_list=False, idx: int = None) -> Instruction | typing.List[Instruction]: Retrieves an instruction or list of them based on its identifier or index.
        - insert_instruction(self, new_instruction: Instruction, after_instruction: str | int = None, before_instruction: str | int = None): Inserts a new instruction into the template.
        - remove_instruction(self, identifier: str = None, idx: int | typing.List[int] = None): Removes an instruction from the template.
        - replace_instruction(self, new_instruction=Instruction, identifier: str = None, idx: int | typing.List[int] = None): Replaces an instruction in the template with a new one.
    """

    def __init__(self, min_ranks: int = 2, thread_level: str = None, has_finalize: bool = True, has_init: bool = True,
                 can_deadlock=False,
                 allow_reorder: bool = True):
        """
        Initialize the TemplateManager

        Parameters:
            min_ranks (int): the number of MPI ranks to use for this case
            thread_level : the MPI Thread Level to use (None means use MPI_Init instead of Init_Thread)
            has_finalize (bool) : if call to MPI Finalize should be included
            has_init (bool) : if call to MPI Init should be included
            allow_reorder (bool): allow reordering of instructions of different ranks for better redability
            can_deadlock (bool): if the testcase can lead to a deadlock
        """
        self._descr_full = ""
        self._descr_short = ""
        self._instructions = []
        self._thread_level = thread_level
        self._min_ranks = min_ranks
        self._has_finalize = has_finalize
        self._has_init = has_init
        self._can_deadlock = can_deadlock
        self._stack_variables = {}
        self._allow_reorder = allow_reorder
        self._category = ""

    def __str__(self):
        """
        Converts the TemplateManager to a string, replacing placeholders.
        """
        version = self.get_version()
        if self._can_deadlock:
            # append the deadlock info after teh version info
            version = version + deadlock_marker
        code_string = ""
        current_rank = 'all'

        has_error = False
        for i in self._instructions:
            if i.has_error():
                has_error = True

        short_short_descr = self._descr_short.split("-")[0]
        outcome_str = "OK"
        if has_error:
            outcome_str = "ERROR " + short_short_descr

        instr_copy = []
        if self._allow_reorder:
            # "bucket-sort" the instructions to group those from different ranks together
            buckets = [[] for _ in range(self._min_ranks)]
            used_buckets = False
            for instr in self._instructions:
                rank = instr.get_rank_executing()
                if rank != 'all' and rank != 'not0':
                    used_buckets = True
                    buckets[rank].append(instr)
                else:
                    # can not reorder beyond this point
                    if used_buckets:
                        for bucket in buckets:
                            instr_copy = instr_copy + bucket
                        # empty buckets
                        buckets = [[] for _ in range(self._min_ranks)]
                        used_buckets = False
                    instr_copy.append(instr)  # current inst

            if used_buckets:
                for bucket in buckets:
                    instr_copy = instr_copy + bucket
        else:
            # no re-ordering
            instr_copy = self._instructions.copy()

        for inst in instr_copy:
            if inst.get_rank_executing() != current_rank:
                if current_rank != 'all':
                    code_string = code_string + "}\n"
                    # end previous if
                current_rank = inst.get_rank_executing()
                if current_rank == 'not0':
                    code_string = code_string + "if (rank!=0){\n"
                elif current_rank != 'all':
                    code_string = code_string + "if (rank==%d){\n" % current_rank

            code_string += str(inst) + "\n"
        # end for inst

        if current_rank != 'all':
            code_string = code_string + "}\n"  # end previous if

        init_string = ""
        if self._has_init:
            if self._thread_level == None:
                init_string = "MPI_Init(&argc, &argv);"
            else:
                init_string = init_thread_template.replace("@{thread_level}@", self._thread_level)

        finalize_string = ""
        if self._has_finalize:
            finalize_string = "MPI_Finalize();"

        stack_vars_str = ""
        for var_type, number in self._stack_variables.items():
            for i in range(number):
                stack_vars_str += (var_type + " " + var_type.lower() + "_" + str(i)
                                   + " = " + CorrectParameterFactory().get_initializer(var_type) + ";\n")

        return (template
                .replace("@{min_num_ranks}@", str(self._min_ranks))
                .replace("@{outcome}@", outcome_str)
                .replace("@{stack_vars}@", stack_vars_str)
                .replace("@{mpi_init}@", init_string)
                .replace("@{mpi_finalize}@", finalize_string)
                .replace("@{desc}@", self._descr_full)
                .replace("@{category}@", self._category)
                .replace("@{errormsg}@", self._descr_short)
                .replace("@{version}@", version)
                .replace("@{test_code}@", code_string))

    def register_instruction(self, inst: str | Instruction | typing.List[Instruction], identifier: str = None,
                             rank_to_execute: str | int = None):
        """
        Registers an instruction block with the template. inserting it at the end, before the mpi finalize
        Parameters:
           - inst: The instruction to register.
           - optional: identifier: overwirtes the identifier of the  instructioneith the provided one (no override if None)
           - optional: rank_to_execute: overwirtes the rank_to_execute of the instructioneith the provided one (no override if None)
        """
        if isinstance(inst, list):
            if identifier is not None:
                for i in inst:
                    i.set_identifier(identifier)
            if rank_to_execute is not None:
                for i in inst:
                    i.set_rank_executing(rank_to_execute)
            self._instructions.extend(inst)
        elif isinstance(inst, str):
            if rank_to_execute is not None:
                self._instructions.append(Instruction(inst, rank=rank_to_execute, identifier=identifier))
            else:
                # use default ('all')
                self._instructions.append(Instruction(inst, identifier=identifier))
        else:
            assert isinstance(inst, Instruction)
            if identifier is not None:
                inst.set_identifier(identifier)
            if rank_to_execute is not None:
                inst.set_rank_executing(rank_to_execute)
            self._instructions.append(inst)

    def add_stack_variable(self, variable_type: str) -> str:
        """
        Adds a new stack variable and returns its name to be used in the code.
        the variable is initialized with the initializer given by the correctParameter class
        returns the name of the new variable
        names follow the sceme: mpi_request_0, mpi_request_1, ...
        """
        # currently supported:
        assert variable_type.startswith("MPI") or variable_type in ["int"]
        # does not support ptr types !
        if variable_type not in self._stack_variables:
            self._stack_variables[variable_type] = 0

        name = variable_type.lower() + "_" + str(self._stack_variables[variable_type])
        self._stack_variables[variable_type] = self._stack_variables[variable_type] + 1
        return name

    def get_version(self) -> str:
        """
        Retrieves the minimum required MPI version.
        Returns:
            str: The MPI version used.
        """
        max_v = "0.0"
        for inst in self._instructions:
            if isinstance(inst, MPICall):
                max_v = max(inst.get_version(), max_v)
        return max_v

    def set_description(self, descr_short: str, descr_full: str):
        """
        Sets the short and full descriptions for the template.
        Parameters:
            - descr_short: The short description for use as the filename.
            - descr_full: The full description for use in the header.
        """
        self._descr_full = descr_full
        self._descr_short = descr_short
        #check if it provides a valid error category
        error_class = descr_short.split("-",1)[0].strip()
        assert error_class in ERROR_CLASSES

    def get_short_descr(self) -> str:
        """
        Retrieves the short description to use as a filename.
        Returns:
            str: The short description.
        """
        assert self._descr_short != ""
        return self._descr_short

    def set_can_deadlock(self, can_deadlock=True):
        self._can_deadlock = can_deadlock

    def get_instruction(self, identifier: str = None, return_list=False, rank_excuting: int | str = None,
                        idx: int = None) -> Instruction | typing.List[Instruction]:
        """
        Retrieves an instruction or list fh them  based on its identifier or index.

        Parameters:
            identifier (str): The identifier of the instruction.
            return_list (bool): Whether to return a list of instruction.
            idx (int): The index of the instruction block.
            rank_excuting(None |int|'all'|'not0'): get instruction from given rank; None: consider instructions from all ranks

        Returns:
            Instruction | List[Instruction]: The instruction block(s).
        Raises:
            ValueError: if both index and identifier are provided
            IndexError: if return_list is False and not exactely one instruction is found by index
        """
        # assert only one param is not None
        parameters = [identifier, idx]
        if parameters.count(None) != 1:
            raise ValueError("Only one parameter is allowed to be specified")

        if identifier is not None:
            to_return = [i for i in self._instructions if i.get_identifier() == identifier]
            if rank_excuting is not None:
                to_return = [i for i in to_return if i.get_rank_executing() == rank_excuting]
            if return_list:
                return to_return

            if len(to_return) == 1:
                return to_return[0]
            if len(to_return) == 0:
                raise IndexError("Found no matching Instruction")
            raise IndexError("Found too many elements")

        if idx is not None:
            if rank_excuting is not None:
                if self._instructions[idx].get_rank_executing() != rank_excuting:
                    raise IndexError("Found no matching Instruction")
            if return_list:
                return [self._instructions[idx]]
            return self._instructions[idx]

        raise ValueError("Neither Both block name nor index is given")

    def __get_instruction_index(self, ident: str | Instruction | typing.List[Instruction]) -> typing.List[int]:
        """
         internal helper function to receive the indices of instructions with ghe given identifier oris the given instruction
         """
        if isinstance(ident, str):
            return [idx for idx, inst in enumerate(self._instructions) if inst.get_identifier() == ident]
        if isinstance(ident, Instruction):
            return [idx for idx, inst in enumerate(self._instructions) if inst == ident]
        if isinstance(ident, list):
            return [idx for idx, inst in enumerate(self._instructions) if inst in ident]
        raise ValueError("Provide string or instruction")

    def insert_instruction(self, new_instruction: typing.List[Instruction] | Instruction,
                           after_instruction: typing.List[Instruction] | Instruction | str | int = None,
                           before_instruction: Instruction | str | int = None, after_last_of_list: bool = False,
                           before_first_of_list: bool = False):
        """
        Inserts a new instruction into the template.

        Parameters:
            new_instruction (Instruction): The new instruction to insert.
            after_instruction (List[Instruction]|Instruction|str | int): The instruction after which to insert the new one (identifier or index).
            before_instruction (Instruction|str | int): The instruction before which to insert the new one (identifier or index).#
            after_last_of_list and before_first_of_list allow inserting after/before a list of instructions
        Raises:
            ValueError: if both before and after are provided
            IndexError: if it finds multiple places to insert by identifier
        Note: the parameter combination after_instruction [list] and before_first_of_list is allowed in this case it will insert AFTER the first list entry usage of this combination is discuraged
        """
        # assert only one param is not None
        parameters = [after_instruction, before_instruction]
        if parameters.count(None) != 1:
            raise ValueError("Only one parameter is allowed to be specified")

        if isinstance(new_instruction, Instruction):
            new_instruction = [new_instruction]

        idx_to_use = None
        inst_idx_list = []
        if after_instruction is not None:
            if isinstance(after_instruction, int):
                inst_idx_list = [1 + after_instruction]
            else:
                inst_idx_list = [1 + x for x in self.__get_instruction_index(after_instruction)]
        if before_instruction is not None:
            if isinstance(before_instruction, int):
                inst_idx_list = [1 + before_instruction]
            else:
                inst_idx_list = self.__get_instruction_index(before_instruction)

        if after_last_of_list:
            assert not before_first_of_list
            idx_to_use = sorted(inst_idx_list)[-1]
        if before_first_of_list:
            assert not after_last_of_list
            idx_to_use = sorted(inst_idx_list)[0]
        if len(inst_idx_list) == 1:
            idx_to_use = inst_idx_list[0]

        if idx_to_use is None:
            raise IndexError("did not find place to insert")

        self._instructions = self._instructions[0:idx_to_use] + new_instruction + self._instructions[idx_to_use:]

    def remove_instruction(self, identifier: str = None, idx: int | typing.List[int] = None,
                           instruction: Instruction | typing.List[Instruction] = None):
        """
            Removes an instruction from the template.

            Parameters:
                identifier (str): The identifier of the instruction to remove.
                idx (int | List[int]): The index or list of indices of the instruction(s) to remove.
                instruction (Instruction | List[Instruction]) the list of instructions to remove
        """

        # assert only one param is not None
        parameters = [identifier, idx, instruction]
        if parameters.count(None) != 2:
            raise ValueError("Only one parameter is allowed to be specified")

        if instruction is not None:
            if isinstance(instruction, Instruction):
                instruction = [instruction]
            for instruction in instruction:
                self._instructions.remove(instruction)
            return

        idxs_to_remove = []
        if idx is not None:
            if isinstance(idx, int):
                idxs_to_remove = [idx]
            else:
                idxs_to_remove = idx
        if identifier is not None:
            idxs_to_remove = self.__get_instruction_index(identifier)

        if len(idxs_to_remove) == 0:
            # TODO
            # may also be a silen No-Op?
            raise ValueError("Nothing to remove")

        self._instructions = [elem for idx, elem in enumerate(self._instructions) if idx not in idxs_to_remove]

    def replace_instruction(self, new_instruction=Instruction, old_instruction: Instruction | List[Instruction] = None,
                            identifier: str = None,
                            idx: int | typing.List[int] = None):
        """
        Replaces an instruction in the template with a new one.

        Parameters:
            new_instruction (Instruction | List[Instruction]): The new instruction(s) to replace with.
            old_instruction (Instruction | List[Instruction]): The old instruction(s) to replace.
            identifier (str): The identifier of the instruction to replace.
            idx (int | List[int]): The index or list of indices of the instruction(s) to replace.
        Raises
        ValueError: if the number of instructions to replace does not match the number of instructions provided
                    or if both idx and identifier are provided
        Notes: The instructions to be replaced must not be in contiguous order
        """
        parameters = [old_instruction, identifier, idx]
        if parameters.count(None) != 2:
            raise ValueError("Only one parameter is allowed to be specified")

        new_instruction_list = []
        if isinstance(new_instruction, Instruction):
            new_instruction_list = [new_instruction]
        else:
            new_instruction_list = new_instruction

        idxs_to_replace = []
        if idx is not None:
            if isinstance(idx, int):
                idxs_to_replace = [idx]
            else:
                idxs_to_replace = idx
        if identifier is not None:
            idxs_to_replace = self.__get_instruction_index(identifier)

        if old_instruction is not None:
            if isinstance(old_instruction, Instruction):
                old_instruction = [old_instruction]
            idxs_to_replace = self.__get_instruction_index(old_instruction)

        if len(idxs_to_replace) != len(new_instruction_list):
            raise ValueError("Number of instructions to Replace does not match number of given instructions")

        for (index, replacement) in zip(idxs_to_replace, new_instruction_list):
            self._instructions[index] = replacement

    def set_category(self, category: str):
        # used in generated code to denote category of the test
        self._category = category
