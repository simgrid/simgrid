#! /usr/bin/python3
from __future__ import annotations

import inspect
import os
import importlib
import importlib.util
import subprocess
import typing
from pathlib import Path

# for printing a nice progress bar
import tqdm

from Infrastructure.ErrorGenerator import ErrorGenerator
from Infrastructure.Template import TemplateManager
from Infrastructure.Variables import *

# number of digits to use numbering filenames
digits_to_use = 3


def import_module(root, file):
    """
    Private function: imports a python module from a file
    """
    full_fname = os.path.join(root, file)
    name = file[:-3]  # name without .py suffix
    spec = importlib.util.spec_from_file_location(name, full_fname)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class GeneratorManager:
    def __init__(self, path, score_table=None, print_discovery=True, skip_invalid=False):
        """
        Instantiates an GeneratorManager and discovers ErrorGenerator classes from Python files in the specified path.

        Parameters:
            - `path` (str): The path to search for Python files containing ErrorGenerator classes.
            - `print_discovery` (bool, optional): Whether to print the discovered generators. Defaults to True.
            - `skip_invalid` (bool, optional): Whether to skip invalid generators. Defaults to False.

        Returns:
            None

        Raises:
            AssertionError: If an ErrorGenerator class is found with unknown features and `skip_invalid` is False.

        Note:
            - Discovers the generators in Python files with the '.py' extension in the specified path and its subdirectories.
        """
        self.generators = []
        self.score_table = score_table
        self.discover_generators(path, print_discovery, skip_invalid)
        self.case_names = {}
        # discover all Error Generators
        pass

    def get_filename(self, case_name, suffix=".c"):
        """
        Private Function: Helps to generate filenames for the generated testcases
        """
        num = 0
        if case_name in self.case_names:
            num = self.case_names[case_name]
        num = num + 1
        self.case_names[case_name] = num

        return case_name + "-" + str(num).zfill(digits_to_use) + suffix

    def generate(self, outpath: str | Path | os.PathLike[str], filterlist_: typing.Sequence[str] = None,
                 print_progress_bar: bool = True, overwrite: bool = True, generation_level: int = BASIC_TEST_LEVEL,
                 try_compile: bool = False, max_mpi_version: str = "4.0", use_clang_format: bool = False):
        """
        Generates test cases based on the specified parameters.
        Parameters:
            - `outpath` (str): The path where the generated test cases will be saved.
            - `filterlist_` (list, optional): A list of features to filter the generators. Defaults to None (no filters).
            - `print_progress_bar` (bool, optional): Whether to print a progress bar. Defaults to True.
            - `overwrite` (bool, optional): Whether to overwrite existing files. Defaults to True.
            - `generation_level` (int, optional): Level of cases to generate (1,2,3). Defaults to 1, only basic testcases, 2: testcases that should have sufficient coverage, 3: al cases..
            - `try_compile` (bool, optional): Whether to try compiling the generated test cases. Defaults to False.
            - `use_clang_format` (bool, optional): Whether to format the generated test cases. Defaults to False.
            - `max_mpi_version` (float, optional): The maximum MPI version allowed for generated test cases. Defaults to 4.0.

        Returns:
            None

        Raises:
            AssertionError: If the environment variable 'MPICC' is not set when `try_compile` is True.
            CalledProcessError: If compilation fails during the try_compile process or clang-formart fails.

        Note:
            - The progress bar is printed using the tqdm module.
            - If `try_compile` is True, it uses the compiler specified via 'MPICC' environment variable to attempt compilation.
            - the Features and if a test belongs to the base of frull test set is defined by the respective generators
            - Generators can Raise CorrectTestcase, therefore the number of discovered testcases may not match the number of generated cases
        """
        filterlist = filterlist_
        if filterlist is None:
            filterlist = featurelist

        if try_compile:
            mpicc = os.environ.get('MPICC')
            assert mpicc and "Environment var MPICC not set"

        # use generator if at least one feature of the generator matches the filterlist
        generators_to_use = [g for g in self.generators if any(elem in filterlist for elem in g.get_feature())]

        print("Generate Testcases using %d generators" % len(generators_to_use))

        # prints a nice progress bar
        if print_progress_bar:
            progress_bar = tqdm.tqdm(total=len(generators_to_use))
        cases_generated = 0

        for generator in generators_to_use:
            # use first feature as category if generator has multiple
            category_path = os.path.join(outpath, generator.get_feature()[0])
            os.makedirs(category_path, exist_ok=True)

            for result_error in generator.generate(generation_level, self.score_table):
                result_error.set_category(generator.get_feature()[0])
                assert isinstance(result_error, TemplateManager)

                if not float(result_error.get_version()) > float(max_mpi_version):
                    case_name = result_error.get_short_descr()
                    fname = self.get_filename(case_name)
                    full_name = os.path.join(category_path, fname)

                    if not overwrite and os.path.isfile(full_name):
                        assert False and "File Already exists"

                    result_str = str(result_error)
                    if use_clang_format:
                        result_str = subprocess.check_output(["clang-format"], text=True,
                                                             input=result_str)

                    with open(full_name, "w") as text_file:
                        text_file.write(result_str)
                    cases_generated += 1

                    if try_compile:
                        subprocess.check_call([mpicc, full_name])
                        # raises CalledProcessError if code does not compile

            if print_progress_bar:
                progress_bar.update(1)
        if print_progress_bar:
            progress_bar.close()

        print("Finished. Generated %i cases" % cases_generated)

        pass

    def count_generated_cases(self, filterlist_: typing.Sequence[str] = None,
                              print_progress_bar: bool = True, generation_level: int = BASIC_TEST_LEVEL,
                              max_mpi_version: str = "4.0"):
        """
        Only counts the cases to be generated by the generators and do not write them to disk
        """

        filterlist = filterlist_
        if filterlist is None:
            filterlist = featurelist

        # use generator if at least one feature of the generator matches the filterlist
        generators_to_use = [g for g in self.generators if any(elem in filterlist for elem in g.get_feature())]

        print("Count Testcases generated if using %d generators" % len(generators_to_use))

        # prints a nice progress bar
        if print_progress_bar:
            progress_bar = tqdm.tqdm(total=len(generators_to_use))
        cases_generated = 0
        faulty_cases_generated_per_cat = {}
        correct_cases_generated_per_cat = {}
        for feature in filterlist:
            faulty_cases_generated_per_cat[feature] = 0
            correct_cases_generated_per_cat[feature] = 0

        for generator in generators_to_use:

            for result_case in generator.generate(generation_level, self.score_table):
                assert isinstance(result_case, TemplateManager)

                if not float(result_case.get_version()) > float(max_mpi_version):
                    cases_generated += 1

                    if "Correct" in result_case.get_short_descr():
                        correct_cases_generated_per_cat[generator.get_feature()[0]] += 1
                    else:
                        faulty_cases_generated_per_cat[generator.get_feature()[0]] += 1

            if print_progress_bar:
                progress_bar.update(1)
        if print_progress_bar:
            progress_bar.close()

        print("Total: Generated %i cases" % cases_generated)
        print(" Feat:\tCorr\tBug\tTotal")
        total = 0
        for f in featurelist:
            print(" %s:\t%i\t%i\t%i" % (f, correct_cases_generated_per_cat[f], faulty_cases_generated_per_cat[f],
                                        correct_cases_generated_per_cat[f] + faulty_cases_generated_per_cat[f]))
            total += correct_cases_generated_per_cat[f] + faulty_cases_generated_per_cat[f]
        assert total == cases_generated

        pass

    def discover_generators(self, path, print_discovery=True, skip_invalid=False):
        """
        Private Function. see Documentation for __init__()
        """
        if print_discovery:
            print("Discover Generators:")
        for root, dirs, files in os.walk(path):
            # sort the traversal: the order of the generators decides the numbering of the cases
            dirs.sort()
            for file in sorted(files):
                if file.endswith('.py'):
                    module = import_module(root, file)
                    for name, obj in inspect.getmembers(module):
                        # if it is a class derived from ErrorGenerator (and is not the interface class itself)
                        if inspect.isclass(obj) and issubclass(obj, ErrorGenerator) and not obj is ErrorGenerator:
                            if print_discovery:
                                print("Found Generator %s" % name)
                            # instantiate the object
                            generator = obj()
                            valid = True
                            for feature in generator.get_feature():
                                if feature not in featurelist:
                                    if print_discovery:
                                        print("Generator has unknown feature: %s" % feature)
                                    valid = False
                            if valid:
                                self.generators.append(generator)
                            else:
                                if not skip_invalid:
                                    assert False and "Invalid Generator"
                                else:
                                    pass  # just skip
