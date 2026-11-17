#! /usr/bin/python3
from __future__ import annotations
from Infrastructure.Variables import *

from Infrastructure.ErrorGenerator import ErrorGenerator
from Infrastructure.Instruction import Instruction

from Infrastructure.MPICallFactory import CorrectMPICallFactory
from Infrastructure.CorrectParameter import CorrectParameterFactory
from Infrastructure.TemplateFactory import get_invalid_param_rma_case, get_rma_template
from Infrastructure.MPICall import MPICall


class InvalidWinErrorRMA(ErrorGenerator):
    def __init__(self):
        pass

    def get_feature(self):
        return ["RMA"]

    def generate(self, generate_level, real_world_score_table):
        tm = get_rma_template()
        tm.remove_instruction("RMA_WIN_ALLOC") # remove window allocation
        # opening epoch on non-initialized window is the actual error
        tm.get_instruction("STARTRMAEPOCH").set_has_error()
        tm.set_description("InvalidParam",
                                "RMA on non-initialized window")
        tm.set_can_deadlock()
        yield tm

        # free window too early
        tm = get_rma_template()
        win_free_early = Instruction(f"MPI_Win_free(&{CorrectParameterFactory().get('win')});")
        tm.insert_instruction(new_instruction=win_free_early, before_instruction="STARTRMAEPOCH")

        # opening epoch on already freed window is the actual error
        tm.get_instruction("STARTRMAEPOCH").set_has_error()
        tm.set_description("InvalidParam",
                                "RMA on freed  window")
        tm.set_can_deadlock()
        yield tm
