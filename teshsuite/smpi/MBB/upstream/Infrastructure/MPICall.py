#! /usr/bin/python3
from __future__ import annotations

import typing
from typing_extensions import override

from Infrastructure.Instruction import Instruction
from Infrastructure.Variables import ERROR_MARKER_COMMENT_BEGIN, ERROR_MARKER_COMMENT_END


class MPICall(Instruction):

    @override
    def __init__(self, function: str, args: typing.OrderedDict[str, str], version: str, rank: str | int = 'all',
                 identifier: str = None):
        super().__init__("", rank, identifier)
        self._function = function
        self._args = args
        self._version = version

    @override
    def __str__(self):
        s = self._function + "("
        for k, v in self._args.items():
            assert v is not None
            s += str(v) + ", "
        s = s[:-2]  # remove last , and space
        s += ");"
        if self._has_error:
            s = ERROR_MARKER_COMMENT_BEGIN + s + ERROR_MARKER_COMMENT_END

        return s

    def set_arg(self, arg: str, value: str):
        if not self.has_arg(arg):
            print("Unknown arg " + arg + " in " + self._function)
            assert self.has_arg(arg)
        self._args[arg] = value

    def get_arg(self, arg: str) -> str:
        assert self.has_arg(arg)
        return self._args[arg]

    def has_arg(self, arg: str) -> bool:
        return arg in self._args

    def get_function(self) -> str:
        return self._function.lower()

    def get_version(self) -> str:
        return self._version

    def __copy__(self) -> MPICall:
        return MPICall(self._function, self._args.copy(), self._version, self.get_rank_executing(), self.get_identifier())
