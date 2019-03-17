#! @PYTHON_EXECUTABLE@
# -*- coding: utf-8 -*-
"""

tesh -- testing shell
========================

Copyright (c) 2012-2019. The SimGrid Team. All rights reserved.

This program is free software; you can redistribute it and/or modify it
under the terms of the license (GNU LGPL) which comes with this package.


#TODO: child of child of child that printfs. Does it work?
#TODO: a child dies after its parent. What happen?

#TODO: regular expression in output
#ex: >> Time taken: [0-9]+s
#TODO: linked regular expression in output
#ex:
# >> Bytes sent: ([0-9]+)
# >> Bytes recv: \1
# then, even better:
# ! expect (\1 > 500)

"""


import sys
import os
import shlex
import re
import difflib
import signal
import argparse

if sys.version_info[0] == 3:
    import subprocess
    import _thread
else:
    raise "This program is expected to run with Python3 only"

##############
#
# Utilities
#
#


def isWindows():
    return sys.platform.startswith('win')

# Singleton metaclass that works in Python 2 & 3
# http://stackoverflow.com/questions/6760685/creating-a-singleton-in-python


class _Singleton(type):
    """ A metaclass that creates a Singleton base class when called. """
    _instances = {}

    def __call__(cls, *args, **kwargs):
        if cls not in cls._instances:
            cls._instances[cls] = super(_Singleton, cls).__call__(*args, **kwargs)
        return cls._instances[cls]


class Singleton(_Singleton('SingletonMeta', (object,), {})):
    pass


SIGNALS_TO_NAMES_DICT = dict((getattr(signal, n), n)
                             for n in dir(signal) if n.startswith('SIG') and '_' not in n)


# exit correctly
def tesh_exit(errcode):
    # If you do not flush some prints are skipped
    sys.stdout.flush()
    # os._exit exit even when executed within a thread
    os._exit(errcode)


def fatal_error(msg):
    print("[Tesh/CRITICAL] " + str(msg))
    tesh_exit(1)


# Set an environment variable.
# arg must be a string with the format "variable=value"
def setenv(arg):
    print("[Tesh/INFO] setenv " + arg)
    t = arg.split("=", 1)
    os.environ[t[0]] = t[1]
    # os.putenv(t[0], t[1]) does not work
    # see http://stackoverflow.com/questions/17705419/python-os-environ-os-putenv-usr-bin-env


# http://stackoverflow.com/questions/30734967/how-to-expand-environment-variables-in-python-as-bash-does
def expandvars2(path):
    return re.sub(r'(?<!\\)\$[A-Za-z_][A-Za-z0-9_]*', '', os.path.expandvars(path))


# https://github.com/Cadair/jupyter_environment_kernels/issues/10
try:
    FileNotFoundError
except NameError:
    # py2
    FileNotFoundError = OSError

##############
#
# Cleanup on signal
#
#

# Global variable. Stores which process group should be killed (or None otherwise)
pgtokill = None


def kill_process_group(pgid):
    if pgid is None:  # Nobody to kill. We don't know who to kill on windows, or we don't have anyone to kill on signal handler
        return

    # print("Kill process group {}".format(pgid))
    try:
        os.killpg(pgid, signal.SIGTERM)
    except OSError:
        # os.killpg failed. OK. Some subprocesses may still be running.
        pass


def signal_handler(signal, frame):
    print("Caught signal {}".format(SIGNALS_TO_NAMES_DICT[signal]))
    if pgtokill is not None:
        kill_process_group(pgtokill)
    tesh_exit(5)


##############
#
# Classes
#
#


# read file line per line (and concat line that ends with "\")
class FileReader(Singleton):
    def __init__(self, filename=None):
        if filename is None:
            self.filename = "(stdin)"
            self.f = sys.stdin
        else:
            self.filename_raw = filename
            self.filename = os.path.basename(filename)
            self.abspath = os.path.abspath(filename)
            self.f = open(self.filename_raw)

        self.linenumber = 0

    def __repr__(self):
        return self.filename + ":" + str(self.linenumber)

    def readfullline(self):
        try:
            line = next(self.f)
            self.linenumber += 1
        except StopIteration:
            return None
        if line[-1] == "\n":
            txt = line[0:-1]
        else:
            txt = line
        while len(line) > 1 and line[-2] == "\\":
            txt = txt[0:-1]
            line = next(self.f)
            self.linenumber += 1
            txt += line[0:-1]
        return txt


# keep the state of tesh (mostly configuration values)
class TeshState(Singleton):
    def __init__(self):
        self.threads = []
        self.args_suffix = ""
        self.ignore_regexps_common = []
        self.jenkins = False  # not a Jenkins run by default
        self.timeout = 10  # default value: 10 sec
        self.wrapper = None
        self.keep = False

    def add_thread(self, thread):
        self.threads.append(thread)

    def join_all_threads(self):
        for t in self.threads:
            t.acquire()
            t.release()

# Command line object


class Cmd(object):
    def __init__(self):
        self.input_pipe = []
        self.output_pipe_stdout = []
        self.output_pipe_stderr = []
        self.timeout = TeshState().timeout
        self.args = None
        self.linenumber = -1

        self.background = False
        # Python threads loose the cwd
        self.cwd = os.getcwd()

        self.ignore_output = False
        self.expect_return = 0

        self.output_display = False

        self.sort = -1

        self.ignore_regexps = TeshState().ignore_regexps_common

    def add_input_pipe(self, l):
        self.input_pipe.append(l)

    def add_output_pipe_stdout(self, l):
        self.output_pipe_stdout.append(l)

    def add_output_pipe_stderr(self, l):
        self.output_pipe_stderr.append(l)

    def set_cmd(self, args, linenumber):
        self.args = args
        self.linenumber = linenumber

    def add_ignore(self, txt):
        self.ignore_regexps.append(re.compile(txt))

    def remove_ignored_lines(self, lines):
        for ign in self.ignore_regexps:
            lines = [l for l in lines if not ign.match(l)]
        return lines

    def _cmd_mkfile(self, argline):
        filename = argline[len("mkfile "):]
        file = open(filename, "w")
        if file is None:
            fatal_error("Unable to create file " + filename)
        file.write("\n".join(self.input_pipe))
        file.write("\n")
        file.close()

    def _cmd_cd(self, argline):
        args = shlex.split(argline)
        if len(args) != 2:
            fatal_error("Too many arguments to cd")
        try:
            os.chdir(args[1])
            print("[Tesh/INFO] change directory to " + args[1])
        except FileNotFoundError:
            print("Chdir to " + args[1] + " failed: No such file or directory")
            print("Test suite `" + FileReader().filename + "': NOK (system error)")
            tesh_exit(4)

    # Run the Cmd if possible.
    # Return False if nothing has been ran.

    def run_if_possible(self):
        if self.can_run():
            if self.background:
                lock = _thread.allocate_lock()
                lock.acquire()
                TeshState().add_thread(lock)
                _thread.start_new_thread(Cmd._run, (self, lock))
            else:
                self._run()
            return True
        else:
            return False

    def _run(self, lock=None):
        # Python threads loose the cwd
        os.chdir(self.cwd)

        # retrocompatibility: support ${aaa:=.} variable format
        def replace_perl_variables(m):
            vname = m.group(1)
            vdefault = m.group(2)
            if vname in os.environ:
                return "$" + vname
            else:
                return vdefault
        self.args = re.sub(r"\${(\w+):=([^}]*)}", replace_perl_variables, self.args)

        # replace bash environment variables ($THINGS) to their values
        self.args = expandvars2(self.args)

        if re.match("^mkfile ", self.args) is not None:
            self._cmd_mkfile(self.args)
            if lock is not None:
                lock.release()
            return

        if re.match("^cd ", self.args) is not None:
            self._cmd_cd(self.args)
            if lock is not None:
                lock.release()
            return

        if TeshState().wrapper is not None:
            self.timeout *= 20
            self.args = TeshState().wrapper + self.args
        elif re.match(".*smpirun.*", self.args) is not None:
            self.args = "sh " + self.args
        if TeshState().jenkins and self.timeout is not None:
            self.timeout *= 10

        self.args += TeshState().args_suffix

        print("[" + FileReader().filename + ":" + str(self.linenumber) + "] " + self.args)

        args = shlex.split(self.args)
        #print (args)

        global pgtokill

        try:
            proc = subprocess.Popen(
                args,
                bufsize=1,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                universal_newlines=True,
                start_new_session=True)
            try:
                if not isWindows():
                    pgtokill = os.getpgid(proc.pid)
            except OSError:
                # os.getpgid failed. OK. No cleanup.
                pass
        except PermissionError:
            print("[" + FileReader().filename + ":" + str(self.linenumber) +
                  "] Cannot start '" + args[0] + "': The binary is not executable.")
            print("[" + FileReader().filename + ":" + str(self.linenumber) + "] Current dir: " + os.getcwd())
            tesh_exit(3)
        except NotADirectoryError:
            print("[" + FileReader().filename + ":" + str(self.linenumber) + "] Cannot start '" +
                  args[0] + "': The path to binary does not exist.")
            print("[" + FileReader().filename + ":" + str(self.linenumber) + "] Current dir: " + os.getcwd())
            tesh_exit(3)
        except FileNotFoundError:
            print("[" + FileReader().filename + ":" + str(self.linenumber) +
                  "] Cannot start '" + args[0] + "': File not found")
            tesh_exit(3)
        except OSError as osE:
            if osE.errno == 8:
                osE.strerror += "\nOSError: [Errno 8] Executed scripts should start with shebang line (like #!/usr/bin/env sh)"
            raise osE

        cmdName = FileReader().filename + ":" + str(self.linenumber)
        try:
            (stdout_data, stderr_data) = proc.communicate("\n".join(self.input_pipe), self.timeout)
            pgtokill = None
        except subprocess.TimeoutExpired:
            print("Test suite `" + FileReader().filename + "': NOK (<" +
                  cmdName + "> timeout after " + str(self.timeout) + " sec)")
            kill_process_group(pgtokill)
            tesh_exit(3)

        if self.output_display:
            print(stdout_data)

        # remove text colors
        ansi_escape = re.compile(r'\x1b[^m]*m')
        stdout_data = ansi_escape.sub('', stdout_data)

        #print ((stdout_data, stderr_data))

        if self.ignore_output:
            print("(ignoring the output of <" + cmdName + "> as requested)")
        else:
            stdouta = stdout_data.split("\n")
            while len(stdouta) > 0 and stdouta[-1] == "":
                del stdouta[-1]
            stdouta = self.remove_ignored_lines(stdouta)
            stdcpy = stdouta[:]

            # Mimic the "sort" bash command, which is case unsensitive.
            if self.sort == 0:
                stdouta.sort(key=lambda x: x.lower())
                self.output_pipe_stdout.sort(key=lambda x: x.lower())
            elif self.sort > 0:
                stdouta.sort(key=lambda x: x[:self.sort].lower())
                self.output_pipe_stdout.sort(key=lambda x: x[:self.sort].lower())

            diff = list(
                difflib.unified_diff(
                    self.output_pipe_stdout,
                    stdouta,
                    lineterm="",
                    fromfile='expected',
                    tofile='obtained'))
            if len(diff) > 0:
                print("Output of <" + cmdName + "> mismatch:")
                if self.sort >= 0:  # If sorted, truncate the diff output and show the unsorted version
                    difflen = 0
                    for line in diff:
                        if difflen < 50:
                            print(line)
                        difflen += 1
                    if difflen > 50:
                        print("(diff truncated after 50 lines)")
                    print("Unsorted observed output:\n")
                    for line in stdcpy:
                        print(line)
                else:  # If not sorted, just display the diff
                    for line in diff:
                        print(line)

                print("Test suite `" + FileReader().filename + "': NOK (<" + cmdName + "> output mismatch)")
                if lock is not None:
                    lock.release()
                if TeshState().keep:
                    f = open('obtained', 'w')
                    obtained = stdout_data.split("\n")
                    while len(obtained) > 0 and obtained[-1] == "":
                        del obtained[-1]
                    obtained = self.remove_ignored_lines(obtained)
                    for line in obtained:
                        f.write("> " + line + "\n")
                    f.close()
                    print("Obtained output kept as requested: " + os.path.abspath("obtained"))
                tesh_exit(2)

        #print ((proc.returncode, self.expect_return))

        if proc.returncode != self.expect_return:
            if proc.returncode >= 0:
                print("Test suite `" + FileReader().filename + "': NOK (<" +
                      cmdName + "> returned code " + str(proc.returncode) + ")")
                if lock is not None:
                    lock.release()
                tesh_exit(2)
            else:
                print("Test suite `" + FileReader().filename + "': NOK (<" + cmdName +
                      "> got signal " + SIGNALS_TO_NAMES_DICT[-proc.returncode] + ")")
                if lock is not None:
                    lock.release()
                tesh_exit(-proc.returncode)

        if lock is not None:
            lock.release()

    def can_run(self):
        return self.args is not None


##############
#
# Main
#
#


if __name__ == '__main__':
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    parser = argparse.ArgumentParser(description='tesh -- testing shell', add_help=True)
    group1 = parser.add_argument_group('Options')
    group1.add_argument('teshfile', nargs='?', help='Name of teshfile, stdin if omitted')
    group1.add_argument(
        '--cd',
        metavar='some/directory',
        help='ask tesh to switch the working directory before launching the tests')
    group1.add_argument('--setenv', metavar='var=value', action='append', help='set a specific environment variable')
    group1.add_argument('--cfg', metavar='arg', action='append', help='add parameter --cfg=arg to each command line')
    group1.add_argument('--log', metavar='arg', action='append', help='add parameter --log=arg to each command line')
    group1.add_argument(
        '--ignore-jenkins',
        action='store_true',
        help='ignore all cruft generated on SimGrid continous integration servers')
    group1.add_argument('--wrapper', metavar='arg', help='Run each command in the provided wrapper (eg valgrind)')
    group1.add_argument(
        '--keep',
        action='store_true',
        help='Keep the obtained output when it does not match the expected one')

    try:
        options = parser.parse_args()
    except SystemExit:
        tesh_exit(1)

    if options.cd is not None:
        print("[Tesh/INFO] change directory to " + options.cd)
        os.chdir(options.cd)

    if options.ignore_jenkins:
        print("Ignore all cruft seen on SimGrid's continous integration servers")
        # Note: regexps should match at the beginning of lines
        TeshState().ignore_regexps_common = [
            re.compile(r"profiling:"),
            re.compile(r"Unable to clean temporary file C:"),
            re.compile(r".*Configuration change: Set 'contexts/"),
            re.compile(r"Picked up JAVA_TOOL_OPTIONS: "),
            re.compile(r"Picked up _JAVA_OPTIONS: "),
            re.compile(r"==[0-9]+== ?WARNING: ASan doesn't fully support"),
            re.compile(r"==[0-9]+== ?WARNING: ASan is ignoring requested __asan_handle_no_return: stack top:"),
            re.compile(r"False positive error reports may follow"),
            re.compile(r"For details see http://code.google.com/p/address-sanitizer/issues/detail\?id=189"),
            re.compile(r"For details see https://github.com/google/sanitizers/issues/189"),
            re.compile(r"Python runtime initialized with LC_CTYPE=C .*"),
            # Seen on CircleCI
            re.compile(r"cmake: /usr/local/lib/libcurl\.so\.4: no version information available \(required by cmake\)"),
            re.compile(r".*mmap broken on FreeBSD, but dlopen\+thread broken too. Switching to dlopen\+raw contexts\."),
            re.compile(r".*dlopen\+thread broken on Apple and BSD\. Switching to raw contexts\."),
        ]
        TeshState().jenkins = True  # This is a Jenkins build

    if options.teshfile is None:
        f = FileReader(None)
        print("Test suite from stdin")
    else:
        if not os.path.isfile(options.teshfile):
            print("Cannot open teshfile '" + options.teshfile + "': File not found")
            tesh_exit(3)
        f = FileReader(options.teshfile)
        print("Test suite '" + f.abspath + "'")

    if options.setenv is not None:
        for e in options.setenv:
            setenv(e)

    if options.cfg is not None:
        for c in options.cfg:
            TeshState().args_suffix += " --cfg=" + c
    if options.log is not None:
        for l in options.log:
            TeshState().args_suffix += " --log=" + l

    if options.wrapper is not None:
        TeshState().wrapper = options.wrapper

    if options.keep:
        TeshState().keep = True

    # cmd holds the current command line
    # tech commands will add some parameters to it
    # when ready, we execute it.
    cmd = Cmd()

    line = f.readfullline()
    while line is not None:
        # print(">>============="+line+"==<<")
        if len(line) == 0:
            #print ("END CMD block")
            if cmd.run_if_possible():
                cmd = Cmd()

        elif line[0] == "#":
            pass

        elif line[0:2] == "p ":
            print("[" + str(FileReader()) + "] " + line[2:])

        elif line[0:2] == "< ":
            cmd.add_input_pipe(line[2:])
        elif line[0:1] == "<":
            cmd.add_input_pipe(line[1:])

        elif line[0:2] == "> ":
            cmd.add_output_pipe_stdout(line[2:])
        elif line[0:1] == ">":
            cmd.add_output_pipe_stdout(line[1:])

        elif line[0:2] == "$ ":
            if cmd.run_if_possible():
                cmd = Cmd()
            cmd.set_cmd(line[2:], f.linenumber)

        elif line[0:2] == "& ":
            if cmd.run_if_possible():
                cmd = Cmd()
            cmd.set_cmd(line[2:], f.linenumber)
            cmd.background = True

        elif line[0:15] == "! output ignore":
            cmd.ignore_output = True
            #print("cmd.ignore_output = True")
        elif line[0:16] == "! output display":
            cmd.output_display = True
            cmd.ignore_output = True
        elif line[0:15] == "! expect return":
            cmd.expect_return = int(line[16:])
            #print("expect return "+str(int(line[16:])))
        elif line[0:15] == "! expect signal":
            sig = line[16:]
            # get the signal integer value from the signal module
            if sig not in signal.__dict__:
                fatal_error("unrecognized signal '" + sig + "'")
            sig = int(signal.__dict__[sig])
            # popen return -signal when a process ends with a signal
            cmd.expect_return = -sig
        elif line[0:len("! timeout ")] == "! timeout ":
            if "no" in line[len("! timeout "):]:
                cmd.timeout = None
            else:
                cmd.timeout = int(line[len("! timeout "):])

        elif line[0:len("! output sort")] == "! output sort":
            if len(line) >= len("! output sort "):
                sort = int(line[len("! output sort "):])
            else:
                sort = 0
            cmd.sort = sort
        elif line[0:len("! setenv ")] == "! setenv ":
            setenv(line[len("! setenv "):])

        elif line[0:len("! ignore ")] == "! ignore ":
            cmd.add_ignore(line[len("! ignore "):])

        else:
            fatal_error("UNRECOGNIZED OPTION")

        line = f.readfullline()

    cmd.run_if_possible()

    TeshState().join_all_threads()

    if f.filename == "(stdin)":
        print("Test suite from stdin OK")
    else:
        print("Test suite `" + f.filename + "' OK")
