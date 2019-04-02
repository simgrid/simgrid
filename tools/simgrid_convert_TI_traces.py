#!/usr/bin/env python3

'''
This script is intended to convert SMPI time independent traces (TIT) from the
old format (simgrid version <= 3.19) to the new format.

On the previous format each MPI_wait calls were associated to the last ISend of
IRecv call arbitrarily.

This new that includes tags field that links MPI_wait calls to the
MPI_ISend or MPI_IRecv associated to this wait.

This script reproduce the old behavior of simgrid because informations are
missing to add the tags properly. It also lower case all the mpi calls.

It takes in input (as argument or in stdin) the trace list file that is only a
simple text file that contains path of actual TIT files split by rank.

It creates a directory to put the traces on ("converted_traces" by default)
'''

import os
import pathlib
import shutil


def insert_elem(split_line, position, elem):
    split_line.insert(position, elem)
    return " ".join(split_line)


def convert_trace(trace_path, base_path, output_path, trace_version="1.0"):
    old_file_path = os.path.join(base_path, trace_path)
    with open(old_file_path) as trace_file:

        new_file_path = os.path.join(output_path, trace_path)
        pathlib.Path(os.path.dirname(new_file_path)).mkdir(
            parents=True, exist_ok=True)

        with open(new_file_path, "w") as new_trace:
            # Write header
            new_trace.write("# version: " + trace_version + "\n")

            last_async_call_src = None
            last_async_call_dst = None
            for line_num, line in enumerate(trace_file.readlines()):
                # print(line)
                new_line = None
                split_line = line.lower().strip().split(" ")
                mpi_call = split_line[1]

                if mpi_call == "recv" or mpi_call == "send":
                    new_line = insert_elem(split_line, 3, "0")

                elif mpi_call == "irecv" or mpi_call == "isend":
                    last_async_call_src = split_line[0]
                    last_async_call_dst = split_line[2]
                    new_line = insert_elem(split_line, 3, "0")
                    # print("found async call in line: "+ str(line_num))

                elif mpi_call == "wait":
                    # print("found wait call in line: "+ str(line_num))
                    if (last_async_call_src is None
                            or last_async_call_dst is None):
                        raise Exception("Invalid traces: No Isend or Irecv "
                                        "found before the wait in line " +
                                        str(line_num) + " in file " + old_file_path)
                    new_line = insert_elem(split_line, 2, last_async_call_src)
                    new_line = insert_elem(split_line, 3, last_async_call_dst)
                    new_line = insert_elem(split_line, 4, "0")

                if new_line is not None:
                    # print("line: "+ str(line_num) + " in file " + old_file_path +
                    #        " processed\n:OLD: " + line + "\n:NEW: " + new_line)
                    new_trace.write(new_line + "\n")
                else:
                    new_trace.write(line.lower())


if __name__ == "__main__":
    import argparse
    import sys

    parser = argparse.ArgumentParser(description=sys.modules[__name__].__doc__)

    parser.add_argument('trace_list_file', type=argparse.FileType('r'),
                        default=sys.stdin, help="The trace list file (e.g. smpi_simgrid.txt)")

    parser.add_argument('--output_path', '-o', default="converted_traces",
                        help="The path where converted traces will be put")

    args = parser.parse_args()

    trace_list_file_path = args.trace_list_file.name

    # creates results dir
    pathlib.Path(args.output_path).mkdir(
        parents=True, exist_ok=True)

    # copy trace list file
    try:
        shutil.copy(trace_list_file_path, args.output_path)
    except shutil.SameFileError:
        print("ERROR: Inplace replacement of the trace is not supported: "
              "Please, select another output path")
        sys.exit(-1)

    with open(trace_list_file_path) as tracelist_file:
        trace_list = tracelist_file.readlines()

    # get based path relative to trace list file
    base_path = os.path.dirname(trace_list_file_path)

    trace_list = [x.strip() for x in trace_list]

    # process trace files
    for trace_path in trace_list:
        if os.path.isabs(trace_path):
            sys.exit("ERROR: Absolute path in the trace list file is not supported")
        convert_trace(trace_path, base_path, args.output_path)

    print("Traces converted!")
    print("Result directory:\n" + args.output_path)
