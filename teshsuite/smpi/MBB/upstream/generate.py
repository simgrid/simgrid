#! /usr/bin/python3
import os

import argparse

from Infrastructure.GeneratorManager import GeneratorManager
from Infrastructure.Variables import *

from Infrastructure.ScoingModule.ScoringTable import get_scoring_table
import pandas as pd

if __name__ == "__main__":
    parser = argparse.ArgumentParser("generate MBB Testcases")
    parser.add_argument("--outpath", action="store", default="gencodes", required=False,
                        help="Path to generate the codes into")
    parser.add_argument("--level", action="store", default=1, type=int, required=False,
                        help="Level of tests to generate. 1: simple cases, 2: caseswith sufficient coverage, 3: usage combinations found in real wolrd dataset, 4: all possible usage combinations")
    parser.add_argument("--real_world_data", default=None, required=False, action="store",
                        help="file of real world dataset, only relevant if level is 3")
    parser.add_argument("--mpi_version", default="4.1", required=False, help="maximum MPI versions to use")
    parser.add_argument("--count_only", default=False, action="store_true", required=False,
                        help="Only count the cases dont generate them")

    debug = parser.add_argument_group("Debug", "Arguments used to debug and test the generation progess")
    debug.add_argument("--generator_dir", action="store", default="scripts/errors", required=False,
                       help="Directory where the generators are")
    debug.add_argument("--compile", action="store_true", default=False, required=False,
                       help="Check if the generated codes compile")
    debug.add_argument("--remove_previous_generation_results", action="store_true", default=False, required=False,
                       help="Clear the outpath before generation (for ease of debugging)")
    ARGS = parser.parse_args()

    gencodes_dir = ARGS.outpath

    score_table = None
    if ARGS.level in REAL_WORLD_FILTERING_LEVELS:
        print("Read real world dataset and build scoring table")
        score_table = get_scoring_table(pd.read_csv(ARGS.real_world_data, header=0, low_memory=False), c_only=True)

    gm = GeneratorManager(ARGS.generator_dir, score_table)

    # remove all testcases from previous execution (ease of debugging)
    if ARGS.remove_previous_generation_results:
        for root, dirs, files in os.walk(gencodes_dir):
            for file in files:
                if file.endswith(".c"):
                    os.remove(os.path.join(root, file))

    if ARGS.count_only:
        gm.count_generated_cases(generation_level=ARGS.level, max_mpi_version=ARGS.mpi_version)
        exit()

    gm.generate(gencodes_dir, try_compile=ARGS.compile, generation_level=ARGS.level, max_mpi_version=ARGS.mpi_version)

    pass
