#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2014-2015. The SimGrid Team. All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

import re
import glob

types = [(
    'TCHAR', 'char', 'c'), ('TSTRING', 'const char*', 'cc'), ('TINT', 'int', 'i'), ('TLONG', 'long', 'l'), ('TUCHAR', 'unsigned char', 'uc'), ('TUSHORT', 'unsigned short', 'us'), ('TUINT', 'unsigned int', 'ui'), ('TULONG', 'unsigned long', 'ul'), ('TFLOAT', 'float', 'f'),
    ('TDOUBLE', 'double', 'd'), ('TDPTR', 'void*', 'dp'), ('TFPTR', 'FPtr', 'fp'), ('TCPTR', 'const void*', 'cp'), ('TSIZE', 'size_t', 'sz'), ('TSGSIZE', 'sg_size_t', 'sgsz'), ('TSGOFF', 'sg_offset_t', 'sgoff'), ('TVOID', 'void', ''), ('TDSPEC', 'void*', 'dp'), ('TFSPEC', 'FPtr', 'fp')]


class Arg(object):
    simcall_types = {k: v for _, k, v in types}

    def __init__(self, name, type, casted=None):
        self.name = name
        self.type = type
        self.casted = casted
        assert type in self.simcall_types, '%s not in (%s)' % (
            type, ', '.join(self.simcall_types.keys()))

    def field(self):
        return self.simcall_types[self.type]

    def rettype(self):
        return '%s' % self.casted if self.casted else self.type

    def cast(self):
        return '(%s)' % self.casted if self.casted else ''


class Simcall(object):
    simcalls_BODY = None
    simcalls_PRE = None

    def __init__(self, name, handler, res, args, call_kind):
        self.name = name
        self.res = res
        self.args = args
        self.need_handler = handler
        self.call_kind = call_kind

    def check(self):
        # libsmx.c  simcall_BODY_
        if self.simcalls_BODY is None:
            f = open('libsmx.cpp')
            self.simcalls_BODY = set(
                re.findall('simcall_BODY_(.*?)\(', f.read()))
            f.close()
        if self.name not in self.simcalls_BODY:
            print '# ERROR: No function calling simcall_BODY_%s' % self.name
            print '# Add something like this to libsmx.c:'
            print '%s simcall_%s(%s) {' % (self.res.rettype(), self.name, ', '.join('%s %s' % (arg.rettype(), arg.name) for arg in self.args))
            print '  return simcall_BODY_%s(%s);' % (self.name)
            print '}'
            return False

        # smx_*.c void simcall_HANDLER_host_on(smx_simcall_t simcall,
        # smx_host_t h)
        if self.simcalls_PRE is None:
            self.simcalls_PRE = set()
            for fn in glob.glob('smx_*') + glob.glob('../mc/*'):
                f = open(fn)
                self.simcalls_PRE |= set(
                    re.findall('simcall_HANDLER_(.*?)\(', f.read()))
                f.close()
        if self.need_handler:
            if (self.name not in self.simcalls_PRE):
                print '# ERROR: No function called simcall_HANDLER_%s' % self.name
                print '# Add something like this to the relevant C file (like smx_io.c if it\'s an IO call):'
                print '%s simcall_HANDLER_%s(smx_simcall_t simcall%s) {' % (self.res.rettype(), self.name, ''.join(', %s %s' % (arg.rettype(), arg.name)
                                                                                                                   for arg in self.args))
                print '  // Your code handling the simcall'
                print '}'
                return False
        else:
            if (self.name in self.simcalls_PRE):
                print '# ERROR: You have a function called simcall_HANDLER_%s, but that simcall is not using any handler' % self.name
                print '# Either change your simcall definition, or kill that function'
                return False
        return True

    def enum(self):
        return '  SIMCALL_%s,' % (self.name.upper())

    def string(self):
        return '  "SIMCALL_%s",' % self.name.upper()

    def accessors(self):
        res = []
        res.append('')
        # Arguments getter/setters
        for i in range(len(self.args)):
            arg = self.args[i]
            res.append('static inline %s simcall_%s__get__%s(smx_simcall_t simcall) {' % (
                arg.rettype(), self.name, arg.name))
            res.append(
                '  return %s simcall->args[%i].%s;' % (arg.cast(), i, arg.field()))
            res.append('}')
            res.append('static inline void simcall_%s__set__%s(smx_simcall_t simcall, %s arg) {' % (
                self.name, arg.name, arg.type))
            res.append('    simcall->args[%i].%s = arg;' % (i, arg.field()))
            res.append('}')

        # Return value getter/setters
        if self.res.type != 'void':
            res.append(
                'static inline %s simcall_%s__get__result(smx_simcall_t simcall){' % (self.res.rettype(), self.name))
            res.append('    return %s simcall->result.%s;' %
                       (self.res.cast(), self.res.field()))
            res.append('}')
            res.append(
                'static inline void simcall_%s__set__result(smx_simcall_t simcall, %s result){' % (self.name, self.res.type,))
            res.append('    simcall->result.%s = result;' % (self.res.field()))
            res.append('}')
        return '\n'.join(res)

    def case(self):
        res = []
        res.append('case SIMCALL_%s:' % (self.name.upper()))
        if self.need_handler:
            res.append(
                '      %ssimcall_HANDLER_%s(simcall %s);' % ('simcall->result.%s = ' % self.res.field() if self.call_kind == 'Func' else ' ',
                                                             self.name,
                                                             ''.join(', %s simcall->args[%d].%s' % (arg.cast(), i, arg.field())
                                                                     for i, arg in enumerate(self.args))))
        else:
            res.append(
                '      %sSIMIX_%s(%s);' % ('simcall->result.%s = ' % self.res.field() if self.call_kind == 'Func' else ' ',
                                           self.name,
                                           ','.join('%s simcall->args[%d].%s' % (arg.cast(), i, arg.field())
                                                    for i, arg in enumerate(self.args))))
        res.append('      %sbreak;  \n' %
                   ('SIMIX_simcall_answer(simcall);\n      ' if self.call_kind != 'Blck' else ' '))
        return '\n'.join(res)

    def body(self):
        res = ['  ']
        res.append(
            'inline static %s simcall_BODY_%s(%s) {' % (self.res.rettype(),
                                                        self.name,
                                                        ', '.join('%s %s' % (arg.rettype(), arg.name) for arg in self.args)))
        res.append('    smx_process_t self = SIMIX_process_self();')
        res.append('')
        res.append(
            '    /* Go to that function to follow the code flow through the simcall barrier */')
        if self.need_handler:
            res.append('    if (0) simcall_HANDLER_%s(%s);' % (self.name,
                                                               ', '.join(["&self->simcall"] + [arg.name for arg in self.args])))
        else:
            res.append('    if (0) SIMIX_%s(%s);' % (self.name,
                                                     ', '.join(arg.name for arg in self.args)))
        res.append(
            '    /* end of the guide intended to the poor programmer wanting to go from MSG to Surf */')
        res.append('')
        res.append('    self->simcall.call = SIMCALL_%s;' %
                   (self.name.upper()))
        res.append(
            '    memset(&self->simcall.result, 0, sizeof(self->simcall.result));')
        res.append(
            '    memset(self->simcall.args, 0, sizeof(self->simcall.args));')
        res.append('\n'.join('    self->simcall.args[%d].%s = (%s) %s;' % (i, arg.field(), arg.type, arg.name)
                             for i, arg in enumerate(self.args)))
        res.append('    if (self != simix_global->maestro_process) {')
        res.append(
            '      XBT_DEBUG("Yield process \'%s\' on simcall %s (%d)", self->name,')
        res.append(
            '                SIMIX_simcall_name(self->simcall.call), (int)self->simcall.call);')
        res.append('      SIMIX_process_yield(self);')
        res.append('    } else {')
        res.append('      SIMIX_simcall_handle(&self->simcall, 0);')
        res.append('    }    ')
        if self.res.type != 'void':
            res.append('    return (%s) self->simcall.result.%s;' %
                       (self.res.rettype(), self.res.field()))
        else:
            res.append('    ')
        res.append('  }')
        return '\n'.join(res)

    def handler_prototype(self):
        if self.need_handler:
            return "XBT_PRIVATE %s simcall_HANDLER_%s(smx_simcall_t simcall%s);" % (self.res.rettype() if self.call_kind == 'Func' else 'void',
                                                                                    self.name,
                                                                                    ''.join(', %s %s' % (arg.rettype(), arg.name)
                                                                                            for i, arg in enumerate(self.args)))
        else:
            return ""


def parse(fn):
    simcalls = []
    resdi = None
    simcalls_guarded = {}
    for line in open(fn).read().split('\n'):
        if line.startswith('##'):
            resdi = []
            simcalls_guarded[re.search(r'## *(.*)', line).group(1)] = resdi
        if line.startswith('#') or not line:
            continue
        match = re.match(
            r'(\S*?) *(\S*?) *(\S*?) *\((.*?)(?:, *(.*?))?\) *(.*)', line)
        assert match, line
        ans, handler, name, rest, resc, args = match.groups()
        assert (ans == 'Proc' or ans == 'Func' or ans == 'Blck'), "Invalid call type: '%s'. Faulty line:\n%s\n" % (
            ans, line)
        assert (handler == 'H' or handler == '-'), "Invalid need_handler indication: '%s'. Faulty line:\n%s\n" % (
            handler, line)
        sargs = []
        for n, t, c in re.findall(r'\((.*?), *(.*?)(?:, *(.*?))?\)', args):
            sargs.append(Arg(n, t, c))
        sim = Simcall(name, handler == 'H',
                      Arg('result', rest, resc), sargs, ans)
        if resdi is None:
            simcalls.append(sim)
        else:
            resdi.append(sim)
    return simcalls, simcalls_guarded


def header(name):
    fd = open(name, 'w')
    fd.write(
        '/**********************************************************************/\n')
    fd.write(
        '/* File generated by src/simix/simcalls.py from src/simix/simcalls.in */\n')
    fd.write(
        '/*                                                                    */\n')
    fd.write(
        '/*                    DO NOT EVER CHANGE THIS FILE                    */\n')
    fd.write(
        '/*                                                                    */\n')
    fd.write(
        '/* change simcalls specification in src/simix/simcalls.in             */\n')
    fd.write(
        '/**********************************************************************/\n\n')
    fd.write('/*\n')
    fd.write(
        ' * Note that the name comes from http://en.wikipedia.org/wiki/Popping\n')
    fd.write(
        ' * Indeed, the control flow is doing a strange dance in there.\n')
    fd.write(' *\n')
    fd.write(
        ' * That\'s not about http://en.wikipedia.org/wiki/Poop, despite the odor :)\n')
    fd.write(' */\n\n')
    return fd


def handle(fd, func, simcalls, guarded_simcalls):
    def nonempty(e): return e != ''
    fd.write(
        '\n'.join(filter(nonempty, (func(simcall) for simcall in simcalls))))

    for guard, list in guarded_simcalls.items():
        fd.write('\n#if %s\n' % (guard))
        fd.write('\n'.join(func(simcall) for simcall in list))
        fd.write('\n#endif\n')

if __name__ == '__main__':
    import sys
    simcalls, simcalls_dict = parse('simcalls.in')

    ok = True
    ok &= all(map(Simcall.check, simcalls))
    for k, v in simcalls_dict.items():
        ok &= all(map(Simcall.check, v))
    # FIXME: we should not hide it
    # if not ok:
    #  print ("Some checks fail!")
    #  sys.exit(1)

    #
    # smx_popping_accessors.c
    #
    fd = header('popping_accessors.h')
    handle(fd, Simcall.accessors, simcalls, simcalls_dict)
    fd.write(
        "\n\n/* The prototype of all simcall handlers, automatically generated for you */\n\n")
    handle(fd, Simcall.handler_prototype, simcalls, simcalls_dict)
    fd.close()

    #
    # smx_popping_enum.c
    #
    fd = header("popping_enum.h")
    fd.write('/**\n')
    fd.write(' * @brief All possible simcalls.\n')
    fd.write(' */\n')
    fd.write('typedef enum {\n')
    fd.write('  SIMCALL_NONE,\n')

    handle(fd, Simcall.enum, simcalls, simcalls_dict)

    fd.write('  NUM_SIMCALLS\n')
    fd.write('} e_smx_simcall_t;\n')
    fd.close()

    #
    # smx_popping_generated.cpp
    #

    fd = header("popping_generated.cpp")

    fd.write('#include <xbt/base.h>\n')
    fd.write('#include "smx_private.h"\n')
    fd.write('#if HAVE_MC\n')
    fd.write('#include "src/mc/mc_forward.hpp"\n')
    fd.write('#endif\n')
    fd.write('\n')
    fd.write('XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_popping);\n\n')

    fd.write(
        '/** @brief Simcalls\' names (generated from src/simix/simcalls.in) */\n')
    fd.write('const char* simcall_names[] = {\n')

    fd.write('   "SIMCALL_NONE",')
    handle(fd, Simcall.string, simcalls, simcalls_dict)

    fd.write('};\n\n')

    fd.write('/**\n')
    fd.write(
        ' * @brief (in kernel mode) unpack the simcall and activate the handler\n')
    fd.write(' * \n')
    fd.write(' * This function is generated from src/simix/simcalls.in\n')
    fd.write(' */\n')
    fd.write(
        'void SIMIX_simcall_handle(smx_simcall_t simcall, int value) {\n')
    fd.write(
        '  XBT_DEBUG("Handling simcall %p: %s", simcall, SIMIX_simcall_name(simcall->call));\n')
    fd.write('  SIMCALL_SET_MC_VALUE(simcall, value);\n')
    fd.write(
        '  if (simcall->issuer->context->iwannadie && simcall->call != SIMCALL_PROCESS_CLEANUP)\n')
    fd.write('    return;\n')
    fd.write('  switch (simcall->call) {\n')

    handle(fd, Simcall.case, simcalls, simcalls_dict)

    fd.write('    case NUM_SIMCALLS:\n')
    fd.write('      break;\n')
    fd.write('    case SIMCALL_NONE:\n')
    fd.write(
        '      THROWF(arg_error,0,"Asked to do the noop syscall on %s@%s",\n')
    fd.write('          SIMIX_process_get_name(simcall->issuer),\n')
    fd.write(
        '          sg_host_get_name(SIMIX_process_get_host(simcall->issuer))\n')
    fd.write('          );\n')
    fd.write('      break;\n')
    fd.write('\n')
    fd.write('  }\n')
    fd.write('}\n')

    fd.close()

    #
    # smx_popping_bodies.cpp
    #
    fd = header('popping_bodies.cpp')
    fd.write('#include "smx_private.h"\n')
    fd.write('#include "src/mc/mc_forward.hpp"\n')
    fd.write('#include "xbt/ex.h"\n')
    fd.write('#include <simgrid/simix.hpp>\n')
    handle(fd, Simcall.body, simcalls, simcalls_dict)
    fd.close()
