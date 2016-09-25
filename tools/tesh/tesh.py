#! @PYTHON_EXECUTABLE@
# -*- coding: utf-8 -*-
"""

tesh -- testing shell
========================

Copyright (c) 2012-2016. The SimGrid Team.
All rights reserved.

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

# TODO: If the output is sorted, we should report it to the users. Corresponding perl chunk
# print "WARNING: Both the observed output and expected output were sorted as requested.\n";
# print "WARNING: Output were only sorted using the $sort_prefix first chars.\n"
#    if ( $sort_prefix > 0 );
# print "WARNING: Use <! output sort 19> to sort by simulated date and process ID only.\n";
#    
# print "----8<---------------  Begin of unprocessed observed output (as it should appear in file):\n";
# map {print "> $_\n"} @{$cmd{'unsorted got'}};
# print "--------------->8----  End of the unprocessed observed output.\n";


"""


import sys, os
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


# Singleton metaclass that works in Python 2 & 3
# http://stackoverflow.com/questions/6760685/creating-a-singleton-in-python
class _Singleton(type):
    """ A metaclass that creates a Singleton base class when called. """
    _instances = {}
    def __call__(cls, *args, **kwargs):
        if cls not in cls._instances:
            cls._instances[cls] = super(_Singleton, cls).__call__(*args, **kwargs)
        return cls._instances[cls]
class Singleton(_Singleton('SingletonMeta', (object,), {})): pass

SIGNALS_TO_NAMES_DICT = dict((getattr(signal, n), n) \
    for n in dir(signal) if n.startswith('SIG') and '_' not in n )



#exit correctly
def exit(errcode):
    #If you do not flush some prints are skipped
    sys.stdout.flush()
    #os._exit exit even when executed within a thread
    os._exit(errcode)


def fatal_error(msg):
    print("[Tesh/CRITICAL] "+str(msg))
    exit(1)


#Set an environment variable.
# arg must be a string with the format "variable=value"
def setenv(arg):
    print("[Tesh/INFO] setenv "+arg)
    t = arg.split("=")
    os.environ[t[0]] = t[1]
    #os.putenv(t[0], t[1]) does not work
    #see http://stackoverflow.com/questions/17705419/python-os-environ-os-putenv-usr-bin-env


#http://stackoverflow.com/questions/30734967/how-to-expand-environment-variables-in-python-as-bash-does
def expandvars2(path):
    return re.sub(r'(?<!\\)\$[A-Za-z_][A-Za-z0-9_]*', '', os.path.expandvars(path))

# https://github.com/Cadair/jupyter_environment_kernels/issues/10
try:
    FileNotFoundError
except NameError:
    #py2
    FileNotFoundError = OSError


##############
#
# Classes
#
#



# read file line per line (and concat line that ends with "\")
class FileReader(Singleton):
    def __init__(self, filename):
        if filename is None:
            self.filename = "(stdin)"
            self.f = sys.stdin
        else:
            self.filename_raw = filename
            self.filename = os.path.basename(filename)
            self.f = open(self.filename_raw)
        
        self.linenumber = 0

    def linenumber(self):
        return self.linenumber
    
    def __repr__(self):
        return self.filename+":"+str(self.linenumber)
    
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


#keep the state of tesh (mostly configuration values)
class TeshState(Singleton):
    def __init__(self):
        self.threads = []
        self.args_suffix = ""
        self.ignore_regexps_common = []
        self.wrapper = None
    
    def add_thread(self, thread):
        self.threads.append(thread)
    
    def join_all_threads(self):
        for t in self.threads:
            t.acquire()
            t.release()

#Command line object
class Cmd(object):
    def __init__(self):
        self.input_pipe = []
        self.output_pipe_stdout = []
        self.output_pipe_stderr = []
        self.timeout = 5
        self.args = None
        self.linenumber = -1
        
        self.background = False
        self.cwd = None
        
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
            fatal_error("Unable to create file "+filename)
        file.write("\n".join(self.input_pipe))
        file.write("\n")
        file.close()

    def _cmd_cd(self, argline):
        args = shlex.split(argline)
        if len(args) != 2:
            fatal_error("Too many arguments to cd")
        try:
            os.chdir(args[1])
            print("[Tesh/INFO] change directory to "+args[1])
        except FileNotFoundError:
            print("Chdir to "+args[1]+" failed: No such file or directory")
            print("Test suite `"+FileReader().filename+"': NOK (system error)")
            exit(4)


    #Run the Cmd if possible.
    # Return False if nothing has been ran.
    def run_if_possible(self):
        if self.can_run():
            if self.background:
                #Python threads loose the cwd
                self.cwd = os.getcwd()
                lock = _thread.allocate_lock()
                lock.acquire()
                TeshState().add_thread(lock)
                _thread.start_new_thread( Cmd._run, (self, lock) )
            else:
                self._run()
            return True
        else:
            return False


    def _run(self, lock=None):
        #Python threads loose the cwd
        if self.cwd is not None:
            os.chdir(self.cwd)
            self.cwd = None
        
        #retrocompatibility: support ${aaa:=.} variable format
        def replace_perl_variables(m):
            vname = m.group(1)
            vdefault = m.group(2)
            if vname in os.environ:
                return "$"+vname
            else:
                return vdefault
        self.args = re.sub(r"\${(\w+):=([^}]*)}", replace_perl_variables, self.args)

        #replace bash environment variables ($THINGS) to their values
        self.args = expandvars2(self.args)
        
        if re.match("^mkfile ", self.args) is not None:
            self._cmd_mkfile(self.args)
            if lock is not None: lock.release()
            return
        
        if re.match("^cd ", self.args) is not None:
            self._cmd_cd(self.args)
            if lock is not None: lock.release()
            return
        
        if TeshState().wrapper is not None:
            self.timeout *= 20
            self.args = TeshState().wrapper + self.args
        elif re.match(".*smpirun.*", self.args) is not None:
            self.args = "sh " + self.args 

        self.args += TeshState().args_suffix
        
        print("["+FileReader().filename+":"+str(self.linenumber)+"] "+self.args)
                
        args = shlex.split(self.args)
        #print (args)
        try:
            proc = subprocess.Popen(args, bufsize=1, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True)
        except OSError as e:
            if e.errno == 8:
                e.strerror += "\nOSError: [Errno 8] Executed scripts should start with shebang line (like #!/bin/sh)"
            raise e

        try:
            (stdout_data, stderr_data) = proc.communicate("\n".join(self.input_pipe), self.timeout)
        except subprocess.TimeoutExpired:
            print("Test suite `"+FileReader().filename+"': NOK (<"+FileReader().filename+":"+str(self.linenumber)+"> timeout after "+str(self.timeout)+" sec)")
            exit(3)

        if self.output_display:
            print(stdout_data)

        #remove text colors
        ansi_escape = re.compile(r'\x1b[^m]*m')
        stdout_data = ansi_escape.sub('', stdout_data)
        
        #print ((stdout_data, stderr_data))
        
        if self.ignore_output:
            print("(ignoring the output of <"+FileReader().filename+":"+str(self.linenumber)+"> as requested)")
        else:
            stdouta = stdout_data.split("\n")
            while len(stdouta) > 0 and stdouta[-1] == "":
                del stdouta[-1]
            stdouta = self.remove_ignored_lines(stdouta)

            #the "sort" bash command is case unsensitive,
            # we mimic its behaviour
            if self.sort == 0:
                stdouta.sort(key=lambda x: x.lower())
                self.output_pipe_stdout.sort(key=lambda x: x.lower())
            elif self.sort > 0:
                stdouta.sort(key=lambda x: x[:self.sort].lower())
                self.output_pipe_stdout.sort(key=lambda x: x[:self.sort].lower())
            
            diff = list(difflib.unified_diff(self.output_pipe_stdout, stdouta,lineterm="",fromfile='expected', tofile='obtained'))
            if len(diff) > 0: 
                print("Output of <"+FileReader().filename+":"+str(self.linenumber)+"> mismatch:")
                for line in diff:
                    print(line)
                print("Test suite `"+FileReader().filename+"': NOK (<"+str(FileReader())+"> output mismatch)")
                if lock is not None: lock.release()
                exit(2)
        
        #print ((proc.returncode, self.expect_return))
        
        if proc.returncode != self.expect_return:
            if proc.returncode >= 0:
                print("Test suite `"+FileReader().filename+"': NOK (<"+str(FileReader())+"> returned code "+str(proc.returncode)+")")
                if lock is not None: lock.release()
                exit(2)
            else:
                print("Test suite `"+FileReader().filename+"': NOK (<"+str(FileReader())+"> got signal "+SIGNALS_TO_NAMES_DICT[-proc.returncode]+")")
                if lock is not None: lock.release()
                exit(-proc.returncode)
            
        if lock is not None: lock.release()
    
    
    
    def can_run(self):
        return self.args is not None




##############
#
# Main
#
#



if __name__ == '__main__':
    
    parser = argparse.ArgumentParser(description='tesh -- testing shell', add_help=True)
    group1 = parser.add_argument_group('Options')
    group1.add_argument('teshfile', nargs='?', help='Name of teshfile, stdin if omitted')
    group1.add_argument('--cd', metavar='some/directory', help='ask tesh to switch the working directory before launching the tests')
    group1.add_argument('--setenv', metavar='var=value', action='append', help='set a specific environment variable')
    group1.add_argument('--cfg', metavar='arg', help='add parameter --cfg=arg to each command line')
    group1.add_argument('--log', metavar='arg', help='add parameter --log=arg to each command line')
    group1.add_argument('--ignore-jenkins', action='store_true', help='ignore all cruft generated on SimGrid continous integration servers')
    group1.add_argument('--wrapper', metavar='arg', help='Run each command in the provided wrapper (eg valgrind)')

    try:
        options = parser.parse_args()
    except:
        exit(1)

    if options.cd is not None:
        os.chdir(options.cd)
    
    if options.ignore_jenkins:
        print("Ignore all cruft seen on SimGrid's continous integration servers")
        TeshState().ignore_regexps_common = [
           re.compile("^profiling:"),
           re.compile(".*WARNING: ASan doesn\'t fully support"),
           re.compile("Unable to clean temporary file C:.*")]
    
    if options.teshfile is None:
        f = FileReader(None)
        print("Test suite from stdin")
    else:
        f = FileReader(options.teshfile)
        print("Test suite '"+f.filename+"'")
    
    if options.setenv is not None:
        for e in options.setenv:
            setenv(e)
    
    if options.cfg is not None:
        TeshState().args_suffix += " --cfg="+options.cfg
    if options.log is not None:
        TeshState().args_suffix += " --log="+options.log

    if options.wrapper is not None:
        TeshState().wrapper = options.wrapper
    
    #cmd holds the current command line
    # tech commands will add some parameters to it
    # when ready, we execute it.
    cmd = Cmd()
    
    line = f.readfullline()
    while line is not None:
        #print(">>============="+line+"==<<")
        if len(line) == 0:
            #print ("END CMD block")
            if cmd.run_if_possible():
                cmd = Cmd()
        
        elif line[0] == "#":
            pass
        
        elif line[0:2] == "p ":
            print("["+str(FileReader())+"] "+line[2:])
        
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
            #get the signal integer value from the signal module
            if sig not in signal.__dict__:
                fatal_error("unrecognized signal '"+sig+"'")
            sig = int(signal.__dict__[sig])
            #popen return -signal when a process ends with a signal
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
        print("Test suite `"+f.filename+"' OK")
