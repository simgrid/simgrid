#! /usr/bin/python3
from __future__ import annotations

from typing_extensions import override

from Infrastructure.Instruction import Instruction
from Infrastructure.Variables import ERROR_MARKER_COMMENT_BEGIN, ERROR_MARKER_COMMENT_END

alloc_template = """
@{TYPE}@* @{NAME}@ = @{FUNCTION}@(@{NUM}@ @{SEP}@ sizeof(*@{NAME}@)); 
"""

"""
 Class Overview:
            The `AllocCall` class is a helper for creationg allocation calls (with malloc or calloc)

        Methods:
            - `__init__(self)`: Initializes a new Alloc Call
            - `__str__(self)`: Converts the AllocCall  instance to a string, replacing placeholders.
            - `set_num_elements(self, num_elements)`: Sets number of elements to allocate
            - `set_type(self, type)`: Sets the type of allocated elements
            - `set_name(self, name)`: Sets the name of allocated variable
            - `set_use_calloc(self)`: Use calloc
            - `set_use_malloc(self)`: Use malloc
"""


class AllocCall(Instruction):
    @override
    def __init__(self, type: str, num_elements: str, name: str = "buf", use_malloc: bool = False,
                 rank: str | int = 'all', identifier: str = None):
        """
        Creates a New allocation Call

        Args:
            type: Type to allocate (int not MPI_INT)
            num_elements: number of buffer elements
            name: name of buffer variable
            use_malloc: True: use Malloc, False: use calloc for allocation
        """
        super().__init__("", rank,identifier)
        self._use_malloc = use_malloc
        self._type = type
        self._num_elements = num_elements
        self._name = name

    @override
    def __str__(self):
        if self._use_malloc:
            delim = '*'
            func = "malloc"
        else:
            delim = ','
            func = "calloc"

        s = (alloc_template
             .replace("@{NAME}@", self._name)
             .replace("@{TYPE}@", self._type)
             .replace("@{FUNCTION}@", func)
             .replace("@{NUM}@", str(self._num_elements))
             .replace("@{SEP}@", delim))

        if self._has_error:
            s = ERROR_MARKER_COMMENT_BEGIN + s + ERROR_MARKER_COMMENT_END

        return s

    def set_num_elements(self, num_elements: str):
        self._num_elements = num_elements

    def set_name(self, name: str):
        self._name = name

    def set_type(self, type: str):
        self._type = type

    def set_use_malloc(self):
        self._use_malloc = True

    def set_use_calloc(self):
        self._use_malloc = False

    def get_num_elements(self) -> str:
        return self._num_elements

    def get_name(self) -> str:
        return self._name

    def get_type(self) -> str:
        return self._type

    def get_use_malloc(self) -> bool:
        return self._use_malloc


def get_free(alloc_call: AllocCall) -> Instruction:
    assert isinstance(alloc_call, AllocCall)
    return Instruction("free(" + alloc_call.get_name() + ");")
