#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2014-2015. The SimGrid Team. All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

import re
import glob

types = [
    ('char', 'c'),
    ('const char*', 'cc'),
    ('int', 'i'),
    ('long', 'l'),
    ('unsigned char', 'uc'),
    ('unsigned short', 'us'),
    ('unsigned int', 'ui'),
    ('unsigned long', 'ul'),
    ('float', 'f'),
    ('double', 'd'),
    ('void*', 'dp'),
    ('FPtr', 'fp'),
    ('const void*', 'cp'),
    ('size_t', 'sz'),
    ('sg_size_t', 'sgsz'),
    ('sg_offset_t', 'sgoff'),
    ('void', ''),
    ('void*', 'dp'),
    ('FPtr', 'fp'),
    ]


class Arg(object):
    simcall_types = {k: v for k, v in types}

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
                '  return simgrid::simix::unmarshal<%s>(simcall->args[%i]);' % (arg.rettype(), i))
            res.append('}')
            res.append('static inline void simcall_%s__set__%s(smx_simcall_t simcall, %s arg) {' % (
                self.name, arg.name, arg.rettype()))
            res.append('    simgrid::simix::marshal<%s>(simcall->args[%i], arg);' % (arg.rettype(), i))
            res.append('}')

        # Return value getter/setters
        if self.res.type != 'void':
            res.append(
                'static inline %s simcall_%s__get__result(smx_simcall_t simcall){' % (self.res.rettype(), self.name))
            res.append('    return simgrid::simix::unmarshal<%s>(simcall->result);' % self.res.rettype())
            res.append('}')
            res.append(
                'static inline void simcall_%s__set__result(smx_simcall_t simcall, %s result){' % (self.name, self.res.rettype()))
            res.append('    simgrid::simix::marshal<%s>(simcall->result, result);' % (self.res.rettype()))
            res.append('}')
        return '\n'.join(res)

    def case(self):
        res = []
        args = [ ("simgrid::simix::unmarshal<%s>(simcall->args[%d])" % (arg.rettype(), i))
            for i, arg in enumerate(self.args) ]
        res.append('case SIMCALL_%s:' % (self.name.upper()))
        if self.need_handler:
            call = "simcall_HANDLER_%s(simcall%s%s)" % (self.name,
                ", " if len(args) > 0 else "",
                ', '.join(args))
        else:
            call = "SIMIX_%s(%s)" % (self.name, ', '.join(args))
        if self.call_kind == 'Func':
            res.append("      simgrid::simix::marshal<%s>(simcall->result, %s);" % (self.res.rettype(), call))
        else:
            res.append("      " + call + ";");
        res.append('      %sbreak;  \n' %
                   ('SIMIX_simcall_answer(simcall);\n      ' if self.call_kind != 'Blck' else ' '))
        return '\n'.join(res)

    def body(self):
        res = ['  ']
        res.append(
            'inline static %s simcall_BODY_%s(%s) {' % (self.res.rettype(),
                                                        self.name,
                                                        ', '.join('%s %s' % (arg.rettype(), arg.name) for arg in self.args)))
        res.append(
            '    /* Go to that function to follow the code flow through the simcall barrier */')
        if self.need_handler:
            res.append('    if (0) simcall_HANDLER_%s(%s);' % (self.name,
                                                               ', '.join(["&SIMIX_process_self()->simcall"] + [arg.name for arg in self.args])))
        else:
            res.append('    if (0) SIMIX_%s(%s);' % (self.name,
                                                     ', '.join(arg.name for arg in self.args)))
        res.append('    return simcall<%s%s>(SIMCALL_%s%s);' % (
            self.res.rettype(),
            "".join([ ", " + arg.rettype() for i, arg in enumerate(self.args) ]),
            self.name.upper(),
            "".join([ ", " + arg.name for i, arg in enumerate(self.args) ])
            ));
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
    fd.write('#include "src/simix/popping_private.h"');
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

    fd.write('/** @private\n')
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
    fd.write("/** @cond */ // Please Doxygen, don't look at this\n")
    fd.write('''
template<class R, class... T>
inline static R simcall(e_smx_simcall_t call, T const&... t)
{
  smx_process_t self = SIMIX_process_self();
  simgrid::simix::marshal(&self->simcall, call, t...);
  if (self != simix_global->maestro_process) {
    XBT_DEBUG("Yield process '%s' on simcall %s (%d)", self->name.c_str(),
              SIMIX_simcall_name(self->simcall.call), (int)self->simcall.call);
    SIMIX_process_yield(self);
  } else {
    SIMIX_simcall_handle(&self->simcall, 0);
  }
  return simgrid::simix::unmarshal<R>(self->simcall.result);
}
''')
    handle(fd, Simcall.body, simcalls, simcalls_dict)
    fd.write("/** @endcond */\n");
    fd.close()
