#ifndef _XBT_AUTOMATON_H
#define _XBT_AUTOMATON_H

#include <stdlib.h>
#include <string.h>
#include <xbt/dynar.h>
#include <xbt/sysdep.h>
#include <xbt/graph.h>

SG_BEGIN_DECL()

typedef struct xbt_state {
  char* id;
  int type; /* -1 = init, 0 = inter, 1 = final */
  xbt_dynar_t in;
  xbt_dynar_t out;
} s_xbt_state;

typedef struct xbt_state* xbt_state_t;

typedef struct xbt_automaton {
  xbt_dynar_t propositional_symbols;
  xbt_dynar_t transitions;
  xbt_dynar_t states;
  xbt_state_t current_state;
} s_xbt_automaton;

typedef struct xbt_automaton* xbt_automaton_t;

typedef struct xbt_exp_label{
  enum{or=0, and=1, not=2, predicat=3, one=4} type;
  union{
    struct{
      struct xbt_exp_label* left_exp;
      struct xbt_exp_label* right_exp;
    }or_and;
    struct xbt_exp_label* exp_not;
    char* predicat;
  }u;
} s_xbt_exp_label;

typedef struct xbt_exp_label* xbt_exp_label_t;


typedef struct xbt_transition {
  xbt_state_t src;
  xbt_state_t dst;
  xbt_exp_label_t label;
} s_xbt_transition;

typedef struct xbt_transition* xbt_transition_t;


typedef struct xbt_propositional_symbol{
  char* pred;
  void* function;
} s_xbt_propositional_symbol;

typedef struct xbt_propositional_symbol* xbt_propositional_symbol_t;


XBT_PUBLIC(xbt_automaton_t) xbt_automaton_new(void);
XBT_PUBLIC(void) xbt_automaton_load(xbt_automaton_t automaton, const char *file);

XBT_PUBLIC(xbt_state_t) xbt_automaton_new_state(xbt_automaton_t a, int type, char* id);

XBT_PUBLIC(xbt_transition_t) xbt_automaton_new_transition(xbt_automaton_t a, xbt_state_t src, xbt_state_t dst, xbt_exp_label_t label);

XBT_PUBLIC(xbt_exp_label_t) xbt_automaton_new_label(int type, ...);

XBT_PUBLIC(xbt_dynar_t) xbt_automaton_get_states(xbt_automaton_t a);

XBT_PUBLIC(xbt_dynar_t) xbt_automaton_get_transitions(xbt_automaton_t a);

XBT_PUBLIC(xbt_transition_t) xbt_automaton_get_transition(xbt_automaton_t a, xbt_state_t src, xbt_state_t dst);

XBT_PUBLIC(void) xbt_automaton_free_automaton(xbt_automaton_t a, void_f_pvoid_t transition_free_function);

XBT_PUBLIC(void) xbt_automaton_free_state(xbt_automaton_t a, xbt_state_t s, void_f_pvoid_t transition_free_function);

XBT_PUBLIC(void) xbt_automaton_free_transition(xbt_automaton_t a, xbt_transition_t t, void_f_pvoid_t transition_free_function);

XBT_PUBLIC(xbt_state_t) xbt_automaton_transition_get_source(xbt_transition_t t);

XBT_PUBLIC(xbt_state_t) xbt_automaton_transition_get_destination(xbt_transition_t t);

XBT_PUBLIC(void) xbt_automaton_transition_set_source(xbt_transition_t t, xbt_state_t src);

XBT_PUBLIC(void) xbt_automaton_transition_set_destination(xbt_transition_t t, xbt_state_t dst);

XBT_PUBLIC(xbt_dynar_t) xbt_automaton_state_get_out_transitions(xbt_state_t s);

XBT_PUBLIC(xbt_dynar_t) xbt_automaton_state_get_in_transitions(xbt_state_t s);

XBT_PUBLIC(xbt_state_t) xbt_automaton_state_exists(xbt_automaton_t a, char *id); 

XBT_PUBLIC(void) xbt_automaton_display(xbt_automaton_t a);

XBT_PUBLIC(void) xbt_automaton_display_exp(xbt_exp_label_t l);

XBT_PUBLIC(xbt_propositional_symbol_t) xbt_new_propositional_symbol(xbt_automaton_t a, const char* id, void* fct);

XBT_PUBLIC(xbt_state_t) xbt_automaton_get_current_state(xbt_automaton_t a);

XBT_PUBLIC(int) automaton_state_compare(xbt_state_t s1, xbt_state_t s2);

XBT_PUBLIC(int) propositional_symbols_compare_value(xbt_dynar_t s1, xbt_dynar_t s2);

XBT_PUBLIC(int) automaton_transition_compare(const void *t1, const void *t2);

XBT_PUBLIC(int) automaton_label_transition_compare(xbt_exp_label_t l1, xbt_exp_label_t l2);


SG_END_DECL()

#endif
