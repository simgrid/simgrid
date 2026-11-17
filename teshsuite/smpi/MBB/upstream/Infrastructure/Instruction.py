#! /usr/bin/python3
from __future__ import annotations

from Infrastructure.Variables import ERROR_MARKER_COMMENT_BEGIN, ERROR_MARKER_COMMENT_END


class Instruction(object):
    """
    Base class to represent an Instruction
    the identifier is used, in order to reference that instruction in the Template Manager (e.g. to change it). can be None
    """

    def __init__(self, str_representation: str, rank: str | int = 'all', identifier: str = None):
        self._str_representation = str_representation
        self._has_error = False
        self._identifier = identifier
        if isinstance(rank, str):
            assert rank in ['all', 'not0']
        self._rank = rank

    def set_has_error(self, has_error: bool = True):
        self._has_error = has_error

    def has_error(self):
        return self._has_error

    def get_identifier(self) -> str:
        return self._identifier

    def set_identifier(self, identifier: str):
        self._identifier = identifier

    def get_rank_executing(self) -> str | int:
        return self._rank

    def set_rank_executing(self, rank: str | int):
        if isinstance(rank, str):
            assert rank in ['all', 'not0']
        self._rank = rank

    def __str__(self):
        if self._has_error:
            return ERROR_MARKER_COMMENT_BEGIN + self._str_representation + ERROR_MARKER_COMMENT_END
        else:
            return self._str_representation
