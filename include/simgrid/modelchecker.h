/* simgrid/modelchecker.h - Formal Verification made possible in SimGrid    */

/* Copyright (c) 2008-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdbool.h>

#include <simgrid_config.h> /* HAVE_MC ? */
#include <xbt.h>
#include "xbt/automaton.h"

#ifndef SIMGRID_MODELCHECKER_H
#define SIMGRID_MODELCHECKER_H

SG_BEGIN_DECL()

XBT_PUBLIC(int) MC_random(int min, int max);

#ifdef HAVE_MC

extern int _sg_do_model_check; /* please don't use directly: we inline MC_is_active, but that's what you should use */
extern int _sg_mc_visited;

#define MC_is_active()                  _sg_do_model_check
#define MC_visited_reduction()          _sg_mc_visited

XBT_PUBLIC(void) MC_assert(int);
XBT_PUBLIC(void) MC_automaton_new_propositional_symbol(const char* id, int(*fct)(void));
XBT_PUBLIC(void) MC_automaton_new_propositional_symbol_pointer(const char *id, int* value);
XBT_PUBLIC(void) MC_automaton_new_propositional_symbol_callback(const char* id,
  xbt_automaton_propositional_symbol_callback_type callback,
  void* data, xbt_automaton_propositional_symbol_free_function_type free_function);
XBT_PUBLIC(void *) MC_snapshot(void);
XBT_PUBLIC(int) MC_compare_snapshots(void *s1, void *s2);
XBT_PUBLIC(void) MC_cut(void);
XBT_PUBLIC(void) MC_ignore(void *addr, size_t size);

#else

#define MC_is_active()                  0
#define MC_visited_reduction()          0

#define MC_assert(a)                    xbt_assert(a)
#define MC_automaton_new_propositional_symbol(a, b) ((void)0)
#define MC_automaton_new_propositional_symbol_pointer(a, b) ((void)0)
#define MC_automaton_new_propositional_symbol_callback(id,callback,data,free_function) \
  if(free_function) free_function(data);
#define MC_snapshot()                   ((void*)0)
#define MC_compare_snapshots(a, b)      0
#define MC_cut()                        ((void)0)
#define MC_ignore(a, b)                 ((void)0)

#endif

SG_END_DECL()

#endif /* SIMGRID_MODELCHECKER_H */
