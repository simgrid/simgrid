# Copyright 2021-2022. The MBI project. All rights reserved.
# This program is free software; you can redistribute it and/or modify it under the terms of the license (GNU GPL).

import os
import time
import subprocess
import sys
import re
import shlex
import select
import signal
import hashlib

class AbstractTool:
    def ensure_image(self, params="", dockerparams=""):
        """Verify that this is executed from the right docker image, and complain if not."""
        if os.path.exists("/MBI") or os.path.exists("trust_the_installation"):
            print("This seems to be a MBI docker image. Good.")
        else:
            print("Please run this script in a MBI docker image. Run these commands:")
            print("  docker build -f Dockerfile -t mpi-bugs-initiative:latest . # Only the first time")
            print(f"  docker run -it --rm --name MIB --volume $(pwd):/MBI {dockerparams}mpi-bugs-initiative /MBI/MBI.py {params}")
            sys.exit(1)

    def build(self, rootdir, cached=True):
        """Rebuilds the tool binaries. By default, we try to reuse the existing build."""
        print("Nothing to do to rebuild the tool binaries.")

    def setup(self, rootdir):
        """
        Ensure that this tool (previously built) is usable in this environment: setup the PATH, etc.
        This is called only once for all tests, from the logs directory.
        """
        # pass

    def run(self, execcmd, filename, binary, num_id, timeout, batchinfo):
        """Compile that test code and anaylse it with the Tool if needed (a cache system should be used)"""
        # pass

    def teardown(self):
        """
        Clean the results of all test runs: remove temp files and binaries.
        This is called only once for all tests, from the logs directory.
        """
        # pass

    def parse(self, cachefile):
        """Read the result of a previous run from the cache, and compute the test outcome"""
        return 'failure'

# Associate all possible detailed outcome to a given error scope. Scopes must be sorted alphabetically.
possible_details = {
    # scope limited to one call
    'InvalidBuffer':'AInvalidParam', 'InvalidCommunicator':'AInvalidParam', 'InvalidDatatype':'AInvalidParam', 'InvalidRoot':'AInvalidParam', 'InvalidTag':'AInvalidParam', 'InvalidWindow':'AInvalidParam', 'InvalidOperator':'AInvalidParam', 'InvalidOtherArg':'AInvalidParam', 'ActualDatatype':'AInvalidParam',
    'InvalidSrcDest':'AInvalidParam',
    # scope: Process-wide
#    'OutOfInitFini':'BInitFini',
    'CommunicatorLeak':'BResLeak', 'DatatypeLeak':'BResLeak', 'GroupLeak':'BResLeak', 'OperatorLeak':'BResLeak', 'TypeLeak':'BResLeak', 'RequestLeak':'BResLeak',
    'MissingStart':'BReqLifecycle', 'MissingWait':'BReqLifecycle',
    'LocalConcurrency':'BLocalConcurrency',
    # scope: communicator
    'CallMatching':'DMatch',
    'CommunicatorMatching':'CMatch', 'DatatypeMatching':'CMatch', 'OperatorMatching':'CMatch', 'RootMatching':'CMatch', 'TagMatching':'CMatch',
    'MessageRace':'DRace',

    'GlobalConcurrency':'DGlobalConcurrency',
    # larger scope
    'BufferingHazard':'EBufferingHazard',
    'OK':'FOK'}

error_scope = {
    'AInvalidParam':'single call',
    'BResLeak':'single process',
#    'BInitFini':'single process',
    'BReqLifecycle':'single process',
    'BLocalConcurrency':'single process',
    'CMatch':'multi-processes',
    'DRace':'multi-processes',
    'DMatch':'multi-processes',
    'DGlobalConcurrency':'multi-processes',
    'EBufferingHazard':'system',
    'FOK':'correct executions'
}

displayed_name = {
    'AInvalidParam':'Invalid parameter',
    'BResLeak':'Resource leak',
#    'BInitFini':'MPI call before initialization/after finalization',
    'BReqLifecycle':'Request lifecycle',
    'BLocalConcurrency':'Local concurrency',
    'CMatch':'Parameter matching',
    'DMatch':"Call ordering",
    'DRace':'Message race',
    'DGlobalConcurrency':'Global concurrency',
    'EBufferingHazard':'Buffering hazard',
    'FOK':"Correct execution",

    'aislinn':'Aislinn', 'civl':'CIVL', 'hermes':'Hermes', 'isp':'ISP', 'itac':'ITAC', 'simgrid':'Mc SimGrid', 'smpi':'SMPI', 'smpivg':'SMPI+VG', 'mpisv':'MPI-SV', 'must':'MUST', 'parcoach':'PARCOACH'
}

def parse_one_code(filename):
    """
    Reads the header of the provided filename, and extract a list of todo item, each of them being a (cmd, expect, test_num) tupple.
    The test_num is useful to build a log file containing both the binary and the test_num, when there is more than one test in the same binary.
    """
    res = []
    test_num = 0
    with open(filename, "r") as input_file:
        state = 0  # 0: before header; 1: in header; 2; after header
        line_num = 1
        for line in input_file:
            if re.match(".*BEGIN_MBI_TESTS.*", line):
                if state == 0:
                    state = 1
                else:
                    raise ValueError(f"MBI_TESTS header appears a second time at line {line_num}: \n{line}")
            elif re.match(".*END_MBI_TESTS.*", line):
                if state == 1:
                    state = 2
                else:
                    raise ValueError(f"Unexpected end of MBI_TESTS header at line {line_num}: \n{line}")
            if state == 1 and re.match(r'\s+\$ ?.*', line):
                m = re.match(r'\s+\$ ?(.*)', line)
                cmd = m.group(1)
                nextline = next(input_file)
                detail = 'OK'
                if re.match('[ |]*OK *', nextline):
                    expect = 'OK'
                else:
                    m = re.match('[ |]*ERROR: *(.*)', nextline)
                    if not m:
                        raise ValueError(
                            f"\n{filename}:{line_num}: MBI parse error: Test not followed by a proper 'ERROR' line:\n{line}{nextline}")
                    expect = 'ERROR'
                    detail = m.group(1)
                    if detail not in possible_details:
                        raise ValueError(
                            f"\n{filename}:{line_num}: MBI parse error: Detailled outcome {detail} is not one of the allowed ones.")
                test = {'filename': filename, 'id': test_num, 'cmd': cmd, 'expect': expect, 'detail': detail}
                res.append(test.copy())
                test_num += 1
                line_num += 1

    if state == 0:
        raise ValueError(f"MBI_TESTS header not found in file '{filename}'.")
    if state == 1:
        raise ValueError(f"MBI_TESTS header not properly ended in file '{filename}'.")

    if len(res) == 0:
        raise ValueError(f"No test found in {filename}. Please fix it.")
    return res

def categorize(tool, toolname, test_id, expected):
    outcome = tool.parse(test_id)

    if not os.path.exists(f'{test_id}.elapsed') and not os.path.exists(f'logs/{toolname}/{test_id}.elapsed'):
        if outcome == 'failure':
            elapsed = 0
        else:
            raise ValueError(f"Invalid test result: {test_id}.txt exists but not {test_id}.elapsed")
    else:
        with open(f'{test_id}.elapsed' if os.path.exists(f'{test_id}.elapsed') else f'logs/{toolname}/{test_id}.elapsed', 'r') as infile:
            elapsed = infile.read()

    # Properly categorize this run
    if outcome == 'timeout':
        res_category = 'timeout'
        if elapsed is None:
            diagnostic = 'hard timeout'
        else:
            diagnostic = f'timeout after {elapsed} sec'
    elif outcome == 'failure' or outcome == 'segfault':
        res_category = 'failure'
        diagnostic = 'tool error, or test not run'
    elif outcome == 'UNIMPLEMENTED':
        res_category = 'unimplemented'
        diagnostic = 'coverage issue'
    elif outcome == 'other':
        res_category = 'other'
        diagnostic = 'inconclusive run'
    elif expected == 'OK':
        if outcome == 'OK':
            res_category = 'TRUE_NEG'
            diagnostic = 'correctly reported no error'
        else:
            res_category = 'FALSE_POS'
            diagnostic = 'reported an error in a correct code'
    elif expected == 'ERROR':
        if outcome == 'OK':
            res_category = 'FALSE_NEG'
            diagnostic = 'failed to detect an error'
        else:
            res_category = 'TRUE_POS'
            diagnostic = 'correctly detected an error'
    else:
        raise ValueError(f"Unexpected expectation: {expected} (must be OK or ERROR)")

    return (res_category, elapsed, diagnostic, outcome)


def run_cmd(buildcmd, execcmd, cachefile, filename, binary, timeout, batchinfo, read_line_lambda=None):
    """
    Runs the test on need. Returns True if the test was ran, and False if it was cached.

    The result is cached if possible, and the test is rerun only if the `test.txt` (containing the tool output) or the `test.elapsed` (containing the timing info) do not exist, or if `test.md5sum` (containing the md5sum of the code to compile) does not match.

    Parameters:
     - buildcmd and execcmd are shell commands to run. buildcmd can be any shell line (incuding && groups), but execcmd must be a single binary to run.
     - cachefile is the name of the test
     - filename is the source file containing the code
     - binary the file name in which to compile the code
     - batchinfo: something like "1/1" to say that this run is the only batch (see -b parameter of MBI.py)
     - read_line_lambda: a lambda to which each line of the tool output is feed ASAP. It allows MUST to interrupt the execution when a deadlock is reported.
    """
    if os.path.exists(f'{cachefile}.txt') and os.path.exists(f'{cachefile}.elapsed') and os.path.exists(f'{cachefile}.md5sum'):
        hash_md5 = hashlib.md5()
        with open(filename, 'rb') as sourcefile:
            for chunk in iter(lambda: sourcefile.read(4096), b""):
                hash_md5.update(chunk)
        newdigest = hash_md5.hexdigest()
        with open(f'{cachefile}.md5sum', 'r') as md5file:
            olddigest = md5file.read()
        #print(f'Old digest: {olddigest}; New digest: {newdigest}')
        if olddigest == newdigest:
            print(f" (result cached -- digest: {olddigest})")
            return False
        os.remove(f'{cachefile}.txt')

    print(f"Wait up to {timeout} seconds")

    start_time = time.time()
    if buildcmd is None:
        output = f"No need to compile {binary}.c (batchinfo:{batchinfo})\n\n"
    else:
        output = f"Compiling {binary}.c (batchinfo:{batchinfo})\n\n"
        output += f"$ {buildcmd}\n"

        compil = subprocess.run(buildcmd, shell=True, check=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if compil.stdout is not None:
            output += str(compil.stdout, errors='replace')
        if compil.returncode != 0:
            output += f"Compilation of {binary}.c raised an error (retcode: {compil.returncode})"
            for line in (output.split('\n')):
                print(f"| {line}", file=sys.stderr)
            with open(f'{cachefile}.elapsed', 'w') as outfile:
                outfile.write(str(time.time() - start_time))
            with open(f'{cachefile}.txt', 'w') as outfile:
                outfile.write(output)
            return True

    output += f"\n\nExecuting the command\n $ {execcmd}\n"
    for line in (output.split('\n')):
        print(f"| {line}", file=sys.stderr)

    # We run the subprocess and parse its output line by line, so that we can kill it as soon as it detects a timeout
    process = subprocess.Popen(shlex.split(execcmd), stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, preexec_fn=os.setsid)
    poll_obj = select.poll()
    poll_obj.register(process.stdout, select.POLLIN)

    pid = process.pid
    pgid = os.getpgid(pid)  # We need that to forcefully kill subprocesses when leaving
    while True:
        if poll_obj.poll(5):  # Something to read? Do check the timeout status every 5 sec if not
            line = process.stdout.readline()
            # From byte array to string, replacing non-representable strings with question marks
            line = str(line, errors='replace')
            output = output + line
            print(f"| {line}", end='', file=sys.stderr)
            if read_line_lambda != None:
                read_line_lambda(line, process)
        if time.time() - start_time > timeout:
            with open(f'{cachefile}.timeout', 'w') as outfile:
                outfile.write(f'{time.time() - start_time} seconds')
            break
        if process.poll() is not None:  # The subprocess ended. Grab all existing output, and return
            line = 'more'
            while line != None and line != '':
                line = process.stdout.readline()
                if line is not None:
                    # From byte array to string, replacing non-representable strings with question marks
                    line = str(line, errors='replace')
                    output = output + line
                    print(f"| {line}", end='', file=sys.stderr)

            break

    # We want to clean all forked processes in all cases, no matter whether they are still running (timeout) or supposed to be off. The runners easily get clogged with zombies :(
    try:
        os.killpg(pgid, signal.SIGTERM)  # Terminate all forked processes, to make sure it's clean whatever the tool does
        process.terminate()  # No op if it's already stopped but useful on timeouts
        time.sleep(0.2)  # allow some time for the tool to finish its childs
        os.killpg(pgid, signal.SIGKILL)  # Finish 'em all, manually
        os.kill(pid, signal.SIGKILL)  # die! die! die!
    except ProcessLookupError:
        pass  # OK, it's gone now

    elapsed = time.time() - start_time

    rc = process.poll()
    if rc < 0:
        status = f"Command killed by signal {-rc}, elapsed time: {elapsed}\n"
    else:
        status = f"Command return code: {rc}, elapsed time: {elapsed}\n"
    print(status)
    output += status

    with open(f'{cachefile}.elapsed', 'w') as outfile:
        outfile.write(str(elapsed))

    with open(f'{cachefile}.txt', 'w') as outfile:
        outfile.write(output)
    with open(f'{cachefile}.md5sum', 'w') as outfile:
        hashed = hashlib.md5()
        with open(filename, 'rb') as sourcefile:
            for chunk in iter(lambda: sourcefile.read(4096), b""):
                hashed.update(chunk)
        outfile.write(hashed.hexdigest())

    return True
