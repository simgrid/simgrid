#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2014. The SimGrid Team. All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

import re, glob

types = [('TCHAR', 'char', 'c')
        ,('TSTRING', 'const char*', 'cc')
        ,('TINT', 'int', 'i')
        ,('TLONG', 'long', 'l')
        ,('TUCHAR', 'unsigned char', 'uc')
        ,('TUSHORT', 'unsigned short', 'us')
        ,('TUINT', 'unsigned int', 'ui')
        ,('TULONG', 'unsigned long', 'ul')
        ,('TFLOAT', 'float', 'f')
        ,('TDOUBLE', 'double', 'd')
        ,('TDPTR', 'void*', 'dp')
        ,('TFPTR', 'FPtr', 'fp')
        ,('TCPTR', 'const void*', 'cp')
        ,('TSIZE', 'size_t', 'sz')
        ,('TSGSIZE', 'sg_size_t', 'sgsz')
        ,('TSGOFF', 'sg_offset_t', 'sgoff')
        ,('TVOID', 'void', '')
        ,('TDSPEC', 'void*', 'dp')
        ,('TFSPEC', 'FPtr', 'fp')]

class Arg(object):
  simcall_types = {k:v for _,k,v in types}
  def __init__(self, name, type, casted=None):
    self.name = name 
    self.type = type
    self.casted = casted
    assert type in self.simcall_types, '%s not in (%s)'%(type, ', '.join(self.simcall_types.keys()))

  def field(self):
    return self.simcall_types[self.type]

  def ret(self):
    return '%s'%self.casted if self.casted else self.type

  def cast(self):
    return '(%s)'%self.casted if self.casted else '' 

class Simcall(object):
  simcalls_BODY = None
  simcalls_PRE = None
  def __init__(self, name, res, args, has_answer=True):
    self.name = name
    self.res = res
    self.args = args
    self.has_answer = has_answer

  def check(self):
    # smx_user.c  simcall_BODY_
    # smx_*.c void SIMIX_pre_host_on(smx_simcall_t simcall, smx_host_t h)
    self.check_body()
    self.check_pre()

  def check_body(self):
      if self.simcalls_BODY is None:
          f = open('smx_user.c')
          self.simcalls_BODY = set(re.findall('simcall_BODY_(.*?)\(', f.read()))
          f.close()
      if self.name not in self.simcalls_BODY:
          print '# ERROR: No function calling simcall_BODY_%s'%self.name
          print '# Add something like this to smx_user.c:'
          print '''%s simcall_%s(%s)
{
  return simcall_BODY_%s(%s);
}\n'''%(self.res.ret()
     ,self.name
     ,', '.join('%s %s'%(arg.ret(), arg.name)
                  for arg in self.args)
     ,self.name
     ,', '.join(arg.name for arg in self.args))
          return False
      return True

  def check_pre(self):
    if self.simcalls_PRE is None:
      self.simcalls_PRE = set()
      for fn in glob.glob('smx_*') + glob.glob('../mc/*'):
        f = open(fn)
        self.simcalls_PRE |= set(re.findall('SIMIX_pre_(.*?)\(', f.read()))
        f.close()
    if self.name not in self.simcalls_PRE:
      print '# ERROR: No function called SIMIX_pre_%s'%self.name
      print '# Add something like this to smx_.*.c:'
      print '''%s SIMIX_pre_%s(smx_simcall_t simcall%s)
{
  // Your code handling the simcall
}\n'''%(self.res.ret()
       ,self.name
       ,''.join(', %s %s'%(arg.ret(), arg.name)
                  for arg in self.args))
      return False
    return True

  def enum(self):
    return 'SIMCALL_%s,'%(self.name.upper())

  def string(self):
    return '[SIMCALL_%s] = "SIMCALL_%s",'%(self.name.upper(), self.name.upper())	

  def accessors(self):
    res = []
    for i in range(len(self.args)):
      res.append(self.arg_getter(i))
      res.append(self.arg_setter(i))
    if self.res.type != 'void':
        res.append('static inline %s simcall_%s__get__result(smx_simcall_t simcall){'%(self.res.ret(), self.name))
        res.append('    return %s simcall->result.%s;'%(self.res.cast(), self.res.field()))
        res.append('}')
        res.append('static inline void simcall_%s__set__result(smx_simcall_t simcall, %s result){'%(self.name, self.res.type,))
        res.append('    simcall->result.%s = result;'%(self.res.field()))
        res.append('}')
    return '\n'.join(res)

  def arg_getter(self, i):
    arg = self.args[i]
    return '''
static inline %s simcall_%s__get__%s(smx_simcall_t simcall){
  return %s simcall->args[%i].%s;
}'''%(arg.ret(), self.name, arg.name, arg.cast(), i, arg.field())

  def arg_setter(self, i):
    arg = self.args[i]
    return '''
static inline void simcall_%s__set__%s(smx_simcall_t simcall, %s arg){
    simcall->args[%i].%s = arg;
}'''%(self.name, arg.name, arg.type, i, arg.field())

  def case(self):
    return '''case SIMCALL_%s:
      %sSIMIX_pre_%s(simcall %s);
      %sbreak;  
'''%(self.name.upper(), 
     'simcall->result.%s = '%self.res.field() if self.res.type != 'void' and self.has_answer else ' ',
     self.name,
     ''.join(', %s simcall->args[%d].%s'%(arg.cast(), i, arg.field()) 
             for i, arg in enumerate(self.args)),
     'SIMIX_simcall_answer(simcall);\n      ' if self.has_answer else ' ')

  def body(self):
    return '''  
inline static %s simcall_BODY_%s(%s) {
    smx_process_t self = SIMIX_process_self();

    /* Go to that function to follow the code flow through the simcall barrier */
    if (0) SIMIX_pre_%s(%s);
    /* end of the guide intended to the poor programmer wanting to go from MSG to Surf */

    self->simcall.call = SIMCALL_%s;
    memset(&self->simcall.result, 0, sizeof(self->simcall.result));
    memset(self->simcall.args, 0, sizeof(self->simcall.args));
%s
    if (self != simix_global->maestro_process) {
      XBT_DEBUG("Yield process '%%s' on simcall %%s (%%d)", self->name,
                SIMIX_simcall_name(self->simcall.call), (int)self->simcall.call);
      SIMIX_process_yield(self);
    } else {
      SIMIX_simcall_enter(&self->simcall, 0);
    }    
    %s
  }'''%(self.res.ret()
       ,self.name
       ,', '.join('%s %s'%(arg.ret(), arg.name)
                  for arg in self.args)
       ,self.name
       ,', '.join(["&self->simcall"]+ [arg.name for arg in self.args])
       ,self.name.upper()
       ,'\n'.join('    self->simcall.args[%d].%s = (%s) %s;'%(i, arg.field(), arg.type, arg.name)
                  for i, arg in enumerate(self.args))
       ,'' if self.res.type == 'void' else 'return self->simcall.result.%s;'%self.res.field())

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
    match = re.match(r'(\S*?) *(\S*?) *\((.*?)(?:, *(.*?))?\) *(.*)', line)
    assert match, line
    name, ans, rest, resc, args = match.groups()
    sargs = []
    for n,t,c in re.findall(r'\((.*?), *(.*?)(?:, *(.*?))?\)', args):
      sargs.append(Arg(n,t,c))
    sim = Simcall(name, Arg('result', rest, resc), sargs, ans == 'True')
    if resdi is None:
      simcalls.append(sim)
    else:
      resdi.append(sim)
  return simcalls, simcalls_guarded

def header(fd):
    fd.write('/**********************************************************************/\n')
    fd.write('/* File generated by src/simix/simcalls.py from src/simix/simcalls.in */\n')
    fd.write('/*                                                                    */\n')
    fd.write('/*                    DO NOT EVER CHANGE THIS FILE                    */\n')
    fd.write('/*                                                                    */\n')
    fd.write('/* change simcalls specification in src/simix/simcalls.in             */\n')  
    fd.write('/**********************************************************************/\n\n')

def handle(fd,func, simcalls, guarded_simcalls):
    fd.write('\n'.join(func(simcall) for simcall in simcalls))
    for guard, list in guarded_simcalls.items():
        fd.write('\n#ifdef %s\n'%(guard))
        fd.write('\n'.join(func(simcall) for simcall in list))
        fd.write('\n#endif\n')

def write(fn, func, simcalls, scd,pre="",post=""):
    fd = open(fn, 'w')
    header(fd)
    fd.write(pre)
    handle(fd, func, simcalls, scd)
    fd.write(post)
    fd.close()

if __name__=='__main__':
  import sys
  simcalls, simcalls_dict = parse('simcalls.in')
  
  ok = True
  ok &= all(map(Simcall.check, simcalls))
  for k,v in simcalls_dict.items():
    ok &= all(map(Simcall.check, v))
  # FIXME: we should not hide it
  #if not ok:
  #  print ("Some checks fail!")
  #  sys.exit(1)

  write('smx_popping_accessors.h', Simcall.accessors, simcalls, simcalls_dict)
  
  
  fd = open("smx_popping_enum.h", 'w')
  header(fd)
  fd.write("""
/*
 * Note that the name comes from http://en.wikipedia.org/wiki/Popping 
 * Indeed, the control flow is doing a strange dance in there.
 *
 * That\'s not about http://en.wikipedia.org/wiki/Poop, despite the odor :)
 */

/**
 * @brief All possible simcalls.
 */
typedef enum {
SIMCALL_NONE,
  """)
  
  handle(fd, Simcall.enum, simcalls, simcalls_dict)
  
  fd.write("""
NUM_SIMCALLS
} e_smx_simcall_t;
  """)

  
  fd.close()

  
  fd = open("smx_popping_generated.c", 'w')
  header(fd)
  fd.write('/*\n')
  fd.write(' * Note that the name comes from http://en.wikipedia.org/wiki/Popping \n')
  fd.write(' * Indeed, the control flow is doing a strange dance in there.\n')
  fd.write(' *\n')
  fd.write(' * That\'s not about http://en.wikipedia.org/wiki/Poop, despite the odor :)\n')
  fd.write(' */\n\n')
  
  fd.write('#include "smx_private.h"\n');
  fd.write('#ifdef HAVE_MC\n');
  fd.write('#include "mc/mc_private.h"\n');
  fd.write('#endif\n');
  fd.write('\n');
  fd.write('XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_smurf);\n\n');
  
  fd.write('/** @brief Simcalls\' names (generated from src/simix/simcalls.in) */\n')
  fd.write('const char* simcall_names[] = {\n')

  handle(fd, Simcall.string, simcalls, simcalls_dict)

  fd.write('[SIMCALL_NONE] = "NONE"\n')
  fd.write('};\n\n')


  fd.write('/**\n');
  fd.write(' * @brief (in kernel mode) unpack the simcall and activate the handler\n');
  fd.write(' * \n')
  fd.write(' * This function is generated from src/simix/simcalls.in\n')
  fd.write(' */\n');
  fd.write('void SIMIX_simcall_enter(smx_simcall_t simcall, int value) {\n');
  fd.write('  XBT_DEBUG("Handling simcall %p: %s", simcall, SIMIX_simcall_name(simcall->call));\n');
  fd.write('  SIMCALL_SET_MC_VALUE(simcall, value);\n');
  fd.write('  if (simcall->issuer->context->iwannadie && simcall->call != SIMCALL_PROCESS_CLEANUP)\n');
  fd.write('    return;\n');
  fd.write('  switch (simcall->call) {\n');

  handle(fd, Simcall.case, simcalls, simcalls_dict)

  fd.write('    case NUM_SIMCALLS:\n');
  fd.write('      break;\n');
  fd.write('    case SIMCALL_NONE:\n');
  fd.write('      THROWF(arg_error,0,"Asked to do the noop syscall on %s@%s",\n');
  fd.write('          SIMIX_process_get_name(simcall->issuer),\n');
  fd.write('          SIMIX_host_get_name(SIMIX_process_get_host(simcall->issuer))\n');
  fd.write('          );\n');
  fd.write('      break;\n');
  fd.write('\n');
  fd.write('  }\n');
  fd.write('}\n');
  
  fd.close()
  
  write('smx_popping_bodies.c', Simcall.body, simcalls, simcalls_dict)
