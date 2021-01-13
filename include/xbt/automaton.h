/* Copyright (c) 2011-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_AUTOMATON_H
#define XBT_AUTOMATON_H

#include <xbt/dynar.h>

SG_BEGIN_DECL

typedef struct xbt_automaton_state {
  char* id;
  int type; /* -1 = init, 0 = inter, 1 = final */
  xbt_dynar_t in;
  xbt_dynar_t out;
} s_xbt_automaton_state;

typedef struct xbt_automaton_state* xbt_automaton_state_t;
typedef const struct xbt_automaton_state* const_xbt_automaton_state_t;

typedef struct xbt_automaton {
  xbt_dynar_t propositional_symbols;
  xbt_dynar_t transitions;
  xbt_dynar_t states;
  xbt_automaton_state_t current_state;
} s_xbt_automaton;

typedef struct xbt_automaton* xbt_automaton_t;
typedef const struct xbt_automaton* const_xbt_automaton_t;

typedef struct xbt_automaton_exp_label{
  enum{AUT_OR=0, AUT_AND=1, AUT_NOT=2, AUT_PREDICAT=3, AUT_ONE=4} type;
  union{
    struct{
      struct xbt_automaton_exp_label* left_exp;
      struct xbt_automaton_exp_label* right_exp;
    }or_and;
    struct xbt_automaton_exp_label* exp_not;
    char* predicat;
  }u;
} s_xbt_automaton_exp_label;

typedef struct xbt_automaton_exp_label* xbt_automaton_exp_label_t;
typedef const struct xbt_automaton_exp_label* const_xbt_automaton_exp_label_t;

typedef struct xbt_automaton_transition {
  xbt_automaton_state_t src;
  xbt_automaton_state_t dst;
  xbt_automaton_exp_label_t label;
} s_xbt_automaton_transition;

typedef struct xbt_automaton_transition* xbt_automaton_transition_t;
typedef const struct xbt_automaton_transition* const_xbt_automaton_transition_t;

typedef struct xbt_automaton_propositional_symbol* xbt_automaton_propositional_symbol_t;
typedef const struct xbt_automaton_propositional_symbol* const_xbt_automaton_propositional_symbol_t;

typedef int (*xbt_automaton_propositional_symbol_callback_type)(void*);
typedef void (*xbt_automaton_propositional_symbol_free_function_type)(void*);

XBT_PUBLIC xbt_automaton_t xbt_automaton_new(void);
XBT_PUBLIC void xbt_automaton_load(xbt_automaton_t automaton, const char* file);
XBT_PUBLIC xbt_automaton_state_t xbt_automaton_state_new(const_xbt_automaton_t a, int type, const char* id);
XBT_PUBLIC xbt_automaton_transition_t xbt_automaton_transition_new(const_xbt_automaton_t a, xbt_automaton_state_t src,
                                                                   xbt_automaton_state_t dst,
                                                                   xbt_automaton_exp_label_t label);
XBT_PUBLIC xbt_automaton_exp_label_t xbt_automaton_exp_label_new_or(xbt_automaton_exp_label_t left,
                                                                    xbt_automaton_exp_label_t right);
XBT_PUBLIC xbt_automaton_exp_label_t xbt_automaton_exp_label_new_and(xbt_automaton_exp_label_t left,
                                                                     xbt_automaton_exp_label_t right);
XBT_PUBLIC xbt_automaton_exp_label_t xbt_automaton_exp_label_new_not(xbt_automaton_exp_label_t exp_not);
XBT_PUBLIC xbt_automaton_exp_label_t xbt_automaton_exp_label_new_predicat(const char* p);
XBT_PUBLIC xbt_automaton_exp_label_t xbt_automaton_exp_label_new_one(void);
XBT_PUBLIC xbt_dynar_t xbt_automaton_get_states(const_xbt_automaton_t a);
XBT_PUBLIC xbt_dynar_t xbt_automaton_get_transitions(const_xbt_automaton_t a);
XBT_PUBLIC xbt_automaton_transition_t xbt_automaton_get_transition(const_xbt_automaton_t a,
                                                                   const_xbt_automaton_state_t src,
                                                                   const_xbt_automaton_state_t dst);
XBT_PUBLIC xbt_automaton_state_t xbt_automaton_transition_get_source(const_xbt_automaton_transition_t t);
XBT_PUBLIC xbt_automaton_state_t xbt_automaton_transition_get_destination(const_xbt_automaton_transition_t t);
XBT_PUBLIC void xbt_automaton_transition_set_source(xbt_automaton_transition_t t, xbt_automaton_state_t src);
XBT_PUBLIC void xbt_automaton_transition_set_destination(xbt_automaton_transition_t t, xbt_automaton_state_t dst);
XBT_PUBLIC xbt_dynar_t xbt_automaton_state_get_out_transitions(const_xbt_automaton_state_t s);
XBT_PUBLIC xbt_dynar_t xbt_automaton_state_get_in_transitions(const_xbt_automaton_state_t s);
XBT_PUBLIC xbt_automaton_state_t xbt_automaton_state_exists(const_xbt_automaton_t a, const char* id);
XBT_PUBLIC void xbt_automaton_display(const_xbt_automaton_t a);
XBT_PUBLIC void xbt_automaton_exp_label_display(const_xbt_automaton_exp_label_t l);

// xbt_automaton_propositional_symbol constructors:
XBT_PUBLIC xbt_automaton_propositional_symbol_t xbt_automaton_propositional_symbol_new(const_xbt_automaton_t a,
                                                                                       const char* id,
                                                                                       int (*fct)(void));
XBT_PUBLIC xbt_automaton_propositional_symbol_t xbt_automaton_propositional_symbol_new_pointer(const_xbt_automaton_t a,
                                                                                               const char* id,
                                                                                               int* value);
XBT_PUBLIC xbt_automaton_propositional_symbol_t xbt_automaton_propositional_symbol_new_callback(
    const_xbt_automaton_t a, const char* id, xbt_automaton_propositional_symbol_callback_type callback, void* data,
    xbt_automaton_propositional_symbol_free_function_type free_function);

// xbt_automaton_propositional_symbol accessors:
XBT_PUBLIC xbt_automaton_propositional_symbol_callback_type
xbt_automaton_propositional_symbol_get_callback(const_xbt_automaton_propositional_symbol_t symbol);
XBT_PUBLIC void* xbt_automaton_propositional_symbol_get_data(const_xbt_automaton_propositional_symbol_t symbol);
XBT_PUBLIC const char* xbt_automaton_propositional_symbol_get_name(const_xbt_automaton_propositional_symbol_t symbol);

// xbt_automaton_propositional_symbol methods!
XBT_PUBLIC int xbt_automaton_propositional_symbol_evaluate(const_xbt_automaton_propositional_symbol_t symbol);

XBT_PUBLIC xbt_automaton_state_t xbt_automaton_get_current_state(const_xbt_automaton_t a);
XBT_PUBLIC int xbt_automaton_state_compare(const_xbt_automaton_state_t s1, const_xbt_automaton_state_t s2);
XBT_PUBLIC int xbt_automaton_propositional_symbols_compare_value(const_xbt_dynar_t s1, const_xbt_dynar_t s2);
XBT_PUBLIC int xbt_automaton_transition_compare(const_xbt_automaton_transition_t t1,
                                                const_xbt_automaton_transition_t t2);
XBT_PUBLIC int xbt_automaton_exp_label_compare(const_xbt_automaton_exp_label_t l1, const_xbt_automaton_exp_label_t l2);
XBT_PUBLIC void xbt_automaton_state_free_voidp(void* s);
XBT_PUBLIC void xbt_automaton_state_free(xbt_automaton_state_t s);
XBT_PUBLIC void xbt_automaton_transition_free_voidp(void* t);
XBT_PUBLIC void xbt_automaton_exp_label_free_voidp(void* e);
XBT_PUBLIC void xbt_automaton_propositional_symbol_free_voidp(void* ps);
XBT_PUBLIC void xbt_automaton_free(xbt_automaton_t a);

SG_END_DECL

#endif
