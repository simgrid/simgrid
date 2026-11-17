#! /usr/bin/python3

# The 'Magic' Comments denoting where the Error is in a testcase
ERROR_MARKER_COMMENT_BEGIN = "/*MBBERROR_BEGIN*/"
ERROR_MARKER_COMMENT_END = "/*MBBERROR_END*/"

# The List of  Features considered
featurelist = ["P2P", "COLL", "RMA", "TOOL", "other"]

BASIC_TEST_LEVEL = 1
SUFFICIENT_REAL_WORLD_TEST_LEVEL = 2
SUFFICIENT_TEST_LEVEL = 3
FULL_REAL_WORLD_TEST_LEVEL = 4
FULL_TEST_LEVEL = 5

REAL_WORLD_FILTERING_LEVELS = [SUFFICIENT_REAL_WORLD_TEST_LEVEL, FULL_REAL_WORLD_TEST_LEVEL]

# list of all error possible classes
ERROR_CLASSES = [
    "Correct",  # no error

    "LocalConcurrency",
    "GlobalConcurrency",

    "CallOrdering",
    "MissingCall",

    "EpochLifeCycle",
    "RequestLifeCycle",

    "LocalParameterMissmatch",
    "GlobalParameterMissmatch",

    "InvalidParam",

]
