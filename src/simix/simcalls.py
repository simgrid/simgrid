#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2014. The SimGrid Team.
# All rights reserved.

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
  
  def result_getter_setter(self):
    return '%s\n%s'%(self.result_getter(), self.result_setter())
  
  def result_getter(self):
    return '' if self.res.type == 'void' else '''static inline %s simcall_%s__get__result(smx_simcall_t simcall){
  return %s simcall->result.%s;
}'''%(self.res.ret(), self.name, self.res.cast(), self.res.field())

  def result_setter(self):
    return '' if self.res.type == 'void' else '''static inline void simcall_%s__set__result(smx_simcall_t simcall, %s result){
    simcall->result.%s = result;
}'''%(self.name, self.res.type, self.res.field())

  def args_getter_setter(self):
    res = []
    for i in range(len(self.args)):
      res.append(self.arg_getter(i))
      res.append(self.arg_setter(i))
    return '\n'.join(res)

  def arg_getter(self, i):
    arg = self.args[i]
    return '''static inline %s simcall_%s__get__%s(smx_simcall_t simcall){
  return %s simcall->args[%i].%s;
}'''%(arg.ret(), self.name, arg.name, arg.cast(), i, arg.field())

  def arg_setter(self, i):
    arg = self.args[i]
    return '''static inline void simcall_%s__set__%s(smx_simcall_t simcall, %s arg){
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
    return '''  inline static %s simcall_BODY_%s(%s) {
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
      SIMIX_simcall_pre(&self->simcall, 0);
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
  res = []
  resdi = None
  resd = {}
  for line in open(fn).read().split('\n'):
    if line.startswith('##'):
      resdi = []
      resd[re.search(r'## *(.*)', line).group(1)] = resdi
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
      res.append(sim)
    else:
      resdi.append(sim)
  return res, resd

def write(fn, func, scs, scd,pre="",post=""):
  f = open(fn, 'w')
  f.write('/*********************************************\n')
  f.write(' * File Generated by src/simix/simcalls.py   *\n')
  f.write(' *                from src/simix/simcalls.in *\n')
  f.write(' * Do not modify this file, add new simcalls *\n')
  f.write(' * in src/simix/simcalls.in                  *\n')  
  f.write(' *********************************************/\n\n')
  f.write(pre)
  f.write('\n'.join(func(sc) for sc in scs))
  for k, v in scd.items():
    f.write('\n#ifdef %s\n%s\n#endif\n'%(k, '\n'.join(func(sc) for sc in v)))
  f.write(post)
  f.close()

if __name__=='__main__':
  import sys
  simcalls, simcalls_dict = parse('simcalls.in')
  
  ok = True
  ok &= all(map(Simcall.check, simcalls))
  for k,v in simcalls_dict.items():
    ok &= all(map(Simcall.check, v))
  #if not ok:
  #  sys.exit(1)

  write('simcalls_generated_enum.h', Simcall.enum, simcalls, simcalls_dict,"""
/**
 * @brief All possible simcalls.
 */
typedef enum {
SIMCALL_NONE,
  ""","""
SIMCALL_NEW_API_INIT,
NUM_SIMCALLS
} e_smx_simcall_t;
  """)
  
  write('simcalls_generated_string.c', Simcall.string, simcalls, simcalls_dict)
  write('simcalls_generated_res_getter_setter.h', Simcall.result_getter_setter, simcalls, simcalls_dict)
  write('simcalls_generated_args_getter_setter.h', Simcall.args_getter_setter, simcalls, simcalls_dict)
  
  
  write('simcalls_generated_case.c', Simcall.case, simcalls, simcalls_dict,"""
#include "smx_private.h"
#ifdef HAVE_MC
#include "mc/mc_private.h"
#endif

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_smurf);

void SIMIX_simcall_pre(smx_simcall_t simcall, int value)
{
  XBT_DEBUG("Handling simcall %p: %s", simcall, SIMIX_simcall_name(simcall->call));
  SIMCALL_SET_MC_VALUE(simcall, value);
  if (simcall->issuer->context->iwannadie && simcall->call != SIMCALL_PROCESS_CLEANUP)
    return;
  switch (simcall->call) {
  ""","""
    case NUM_SIMCALLS:
      break;
    case SIMCALL_NONE:
      THROWF(arg_error,0,"Asked to do the noop syscall on %s@%s",
          SIMIX_process_get_name(simcall->issuer),
          SIMIX_host_get_name(SIMIX_process_get_host(simcall->issuer))
          );
      break;

    /* ****************************************************************************************** */
    /* TUTORIAL: New API                                                                        */
    /* ****************************************************************************************** */
    case SIMCALL_NEW_API_INIT:
      SIMIX_pre_new_api_fct(simcall);
      break;
  }
}
  """)
  
  
  write('simcalls_generated_body.c', Simcall.body, simcalls, simcalls_dict)
