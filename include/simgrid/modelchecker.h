/* simgrid/modelchecker.h - Formal Verification made possible in SimGrid    */

/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** \file modelchecker.h
 *
 *  This is the API used by the user simulated program to communicate
 *  with the MC.
 */

#ifndef SIMGRID_MODELCHECKER_H
#define SIMGRID_MODELCHECKER_H

#include <stdbool.h>

#include <simgrid_config.h> /* HAVE_MC ? */

#include <xbt/base.h>

SG_BEGIN_DECL()

XBT_PUBLIC(int) MC_random(int min, int max);

#if HAVE_MC

/* Internal variable used to check if we're running under the MC
 *
 * Please don't use directly: you should use MC_is_active. */
extern XBT_PUBLIC(int) _sg_do_model_check;
extern XBT_PUBLIC(int) _sg_mc_visited;

#define MC_is_active()                  _sg_do_model_check
#define MC_visited_reduction()          _sg_mc_visited

/** Assertion for the model-checker
 *
 *  This function is used to define safety properties to verify.
 */
XBT_PUBLIC(void) MC_assert(int);

XBT_PUBLIC(void) MC_automaton_new_propositional_symbol(const char* id, int(*fct)(void));
XBT_PUBLIC(void) MC_automaton_new_propositional_symbol_pointer(const char *id, int* value);

XBT_PUBLIC(void) MC_cut(void);
XBT_PUBLIC(void) MC_ignore(void *addr, size_t size);

#else

#define MC_is_active()                  0
#define MC_visited_reduction()          0

#define MC_assert(a)                    xbt_assert(a)
#define MC_automaton_new_propositional_symbol(a, b) ((void)0)
#define MC_automaton_new_propositional_symbol_pointer(a, b) ((void)0)
#define MC_cut()                        ((void)0)
#define MC_ignore(a, b)                 ((void)0)

#endif

SG_END_DECL()

#endif /* SIMGRID_MODELCHECKER_H */
