#! /usr/bin/python3

# ABC in python is abstract Base Class
from abc import ABC, abstractmethod

import pandas as pd


class ErrorGenerator(ABC):
    """
        Abstract Class Overview:
            The `ErrorGenerator` class is an abstract class defining the interface for error generators.

        Methods:
            - `__init__(self)`: Abstract method to initialize an instance of the ErrorGenerator class.
            - `get_feature(self)`: Abstract method to get the features used in this generator (list).
            - `get_num_errors(self)`: Abstract method to get the number of errors to produce in the normal mode.
            - `get_num_errors_extended(self)`: Abstract method to get the number of errors in extended mode.
            - `generate(self, i)`: Abstract method to generate error number i, returning a TemplateManager instance.
    """

    @abstractmethod
    def __init__(self):
        pass

    # get the Features used in this generator (list)
    @abstractmethod
    def get_feature(self):
        """
        Abstract method to get the features used in this generator.

        Returns:
            list: List of features used.
        """
        pass

    @abstractmethod
    def generate(self, generation_level: int, real_world_score_table: pd.DataFrame):
        """
        Abstract method to generate error cases. Implemented as a python generator.
        Meaning that the cases should be yielded instead of returned

        Parameters:
            - generation_level: if the generator should generate the extended full set of cases.
            - real_world_score_table: used to decide which cases to generate

        yield:
            TemplateManager: An instantiated and set up TemplateManager with the error case.

        """
        pass
