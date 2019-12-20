/* automaton - representation of büchi automaton */

/* Copyright (c) 2011-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/automaton.h"
#include <stdio.h> /* printf */
#include <xbt/sysdep.h>

struct xbt_automaton_propositional_symbol{
  char* pred;
  /** Callback used to evaluate the value of the symbol */
  int (*callback)(void*);
  /** Additional data for the callback.
      Alternatively it can be used as a pointer to the data. */
  void* data;
  /** Optional callback used to free the data field */
  void (*free_function)(void*);
};

xbt_automaton_t xbt_automaton_new(void){
  xbt_automaton_t automaton = xbt_new0(struct xbt_automaton, 1);
  automaton->states = xbt_dynar_new(sizeof(xbt_automaton_state_t), xbt_automaton_state_free_voidp);
  automaton->transitions = xbt_dynar_new(sizeof(xbt_automaton_transition_t), xbt_automaton_transition_free_voidp);
  automaton->propositional_symbols = xbt_dynar_new(sizeof(xbt_automaton_propositional_symbol_t), xbt_automaton_propositional_symbol_free_voidp);
  return automaton;
}

xbt_automaton_state_t xbt_automaton_state_new(xbt_automaton_t a, int type, char* id){
  xbt_automaton_state_t state = xbt_new0(struct xbt_automaton_state, 1);
  state->type = type;
  state->id = xbt_strdup(id);
  state->in = xbt_dynar_new(sizeof(xbt_automaton_transition_t), xbt_automaton_transition_free_voidp);
  state->out = xbt_dynar_new(sizeof(xbt_automaton_transition_t), xbt_automaton_transition_free_voidp);
  xbt_dynar_push(a->states, &state);
  return state;
}

xbt_automaton_transition_t xbt_automaton_transition_new(xbt_automaton_t a, xbt_automaton_state_t src, xbt_automaton_state_t dst, xbt_automaton_exp_label_t label){
  xbt_automaton_transition_t transition = xbt_new0(struct xbt_automaton_transition, 1);
  if(src != NULL){
    xbt_dynar_push(src->out, &transition);
    transition->src = src;
  }
  if(dst != NULL){
    xbt_dynar_push(dst->in, &transition);
    transition->dst = dst;
  }
  transition->label = label;
  xbt_dynar_push(a->transitions, &transition);
  return transition;
}

xbt_automaton_exp_label_t xbt_automaton_exp_label_new_or(xbt_automaton_exp_label_t left,
                                                         xbt_automaton_exp_label_t right)
{
  xbt_automaton_exp_label_t label = xbt_new0(struct xbt_automaton_exp_label, 1);
  label->type                     = AUT_OR;
  label->u.or_and.left_exp        = left;
  label->u.or_and.right_exp       = right;
  return label;
}

xbt_automaton_exp_label_t xbt_automaton_exp_label_new_and(xbt_automaton_exp_label_t left,
                                                          xbt_automaton_exp_label_t right)
{
  xbt_automaton_exp_label_t label = xbt_new0(struct xbt_automaton_exp_label, 1);
  label->type                     = AUT_AND;
  label->u.or_and.left_exp        = left;
  label->u.or_and.right_exp       = right;
  return label;
}

xbt_automaton_exp_label_t xbt_automaton_exp_label_new_not(xbt_automaton_exp_label_t exp_not)
{
  xbt_automaton_exp_label_t label = xbt_new0(struct xbt_automaton_exp_label, 1);
  label->type                     = AUT_NOT;
  label->u.exp_not                = exp_not;
  return label;
}

xbt_automaton_exp_label_t xbt_automaton_exp_label_new_predicat(char* p)
{
  xbt_automaton_exp_label_t label = xbt_new0(struct xbt_automaton_exp_label, 1);
  label->type                     = AUT_PREDICAT;
  label->u.predicat               = xbt_strdup(p);
  return label;
}

xbt_automaton_exp_label_t xbt_automaton_exp_label_new_one(void)
{
  xbt_automaton_exp_label_t label = xbt_new0(struct xbt_automaton_exp_label, 1);
  label->type                     = AUT_ONE;
  return label;
}

xbt_dynar_t xbt_automaton_get_states(xbt_automaton_t a){
  return a->states;
}

xbt_dynar_t xbt_automaton_get_transitions(xbt_automaton_t a){
  return a->transitions;
}

xbt_automaton_transition_t xbt_automaton_get_transition(XBT_ATTRIB_UNUSED xbt_automaton_t a, xbt_automaton_state_t src,
                                                        xbt_automaton_state_t dst)
{
  xbt_automaton_transition_t transition;
  unsigned int cursor;
  xbt_dynar_foreach(src->out, cursor, transition){
    if((transition->src == src) && (transition->dst == dst))
      return transition;
  }
  return NULL;
}

xbt_automaton_state_t xbt_automaton_transition_get_source(xbt_automaton_transition_t t){
  return t->src;
}

xbt_automaton_state_t xbt_automaton_transition_get_destination(xbt_automaton_transition_t t){
  return t->dst;
}

void xbt_automaton_transition_set_source(xbt_automaton_transition_t t, xbt_automaton_state_t src){
  t->src = src;
  xbt_dynar_push(src->out,&t);
}

void xbt_automaton_transition_set_destination(xbt_automaton_transition_t t, xbt_automaton_state_t dst){
  t->dst = dst;
  xbt_dynar_push(dst->in,&t);
}

xbt_dynar_t xbt_automaton_state_get_out_transitions(xbt_automaton_state_t s){
  return s->out;
}

xbt_dynar_t xbt_automaton_state_get_in_transitions(xbt_automaton_state_t s){
  return s->in;
}

xbt_automaton_state_t xbt_automaton_state_exists(xbt_automaton_t a, char *id){
  xbt_automaton_state_t state = NULL;
  unsigned int cursor = 0;
  xbt_dynar_foreach(a->states, cursor, state){
   if(strcmp(state->id, id)==0)
     return state;
  }
  return NULL;
}

void xbt_automaton_display(xbt_automaton_t a){
  unsigned int cursor;
  xbt_automaton_state_t state = NULL;

  printf("\n\nCurrent state: %s\n", a->current_state->id);

  printf("\nStates' List: %lu\n\n", xbt_dynar_length(a->states));

  xbt_dynar_foreach(a->states, cursor, state)
    printf("ID: %s, type: %d\n", state->id, state->type);

  xbt_automaton_transition_t transition;
  printf("\nTransitions: %lu\n\n", xbt_dynar_length(a->transitions));

  xbt_dynar_foreach(a->transitions, cursor, transition){
    printf("label:");
    xbt_automaton_exp_label_display(transition->label);
    printf(", %s -> %s\n", transition->src->id, transition->dst->id);
  }
}

void xbt_automaton_exp_label_display(xbt_automaton_exp_label_t label){
  printf("(");
  switch(label->type){
    case 0:
      xbt_automaton_exp_label_display(label->u.or_and.left_exp);
      printf(" || ");
      xbt_automaton_exp_label_display(label->u.or_and.right_exp);
      break;
    case 1:
      xbt_automaton_exp_label_display(label->u.or_and.left_exp);
      printf(" && ");
      xbt_automaton_exp_label_display(label->u.or_and.right_exp);
      break;
    case 2:
      printf("!");
      xbt_automaton_exp_label_display(label->u.exp_not);
      break;
    case 3:
      printf("%s", label->u.predicat);
      break;
    case 4:
      printf("1");
      break;
    default:
      break;
  }
  printf(")");
}

xbt_automaton_state_t xbt_automaton_get_current_state(xbt_automaton_t a){
  return a->current_state;
}

static int call_simple_function(int function(void) )
{
  return function();
}

xbt_automaton_propositional_symbol_t xbt_automaton_propositional_symbol_new(xbt_automaton_t a, const char* id,
                                                                            int (*fct)(void))
{
  xbt_automaton_propositional_symbol_t prop_symb = xbt_new0(struct xbt_automaton_propositional_symbol, 1);
  prop_symb->pred = xbt_strdup(id);
  prop_symb->callback                            = ((int (*)(void *))&call_simple_function);
  prop_symb->data = (void*)&fct;
  prop_symb->free_function = NULL;
  xbt_dynar_push(a->propositional_symbols, &prop_symb);
  return prop_symb;
}

XBT_PUBLIC xbt_automaton_propositional_symbol_t xbt_automaton_propositional_symbol_new_pointer(xbt_automaton_t a,
                                                                                               const char* id,
                                                                                               int* value)
{
  xbt_automaton_propositional_symbol_t prop_symb = xbt_new0(struct xbt_automaton_propositional_symbol, 1);
  prop_symb->pred = xbt_strdup(id);
  prop_symb->callback = NULL;
  prop_symb->data = value;
  prop_symb->free_function = NULL;
  xbt_dynar_push(a->propositional_symbols, &prop_symb);
  return prop_symb;
}

XBT_PUBLIC xbt_automaton_propositional_symbol_t xbt_automaton_propositional_symbol_new_callback(
    xbt_automaton_t a, const char* id, xbt_automaton_propositional_symbol_callback_type callback, void* data,
    xbt_automaton_propositional_symbol_free_function_type free_function)
{
  xbt_automaton_propositional_symbol_t prop_symb = xbt_new0(struct xbt_automaton_propositional_symbol, 1);
  prop_symb->pred = xbt_strdup(id);
  prop_symb->callback = callback;
  prop_symb->data = data;
  prop_symb->free_function = free_function;
  xbt_dynar_push(a->propositional_symbols, &prop_symb);
  return prop_symb;
}

XBT_PUBLIC int xbt_automaton_propositional_symbol_evaluate(xbt_automaton_propositional_symbol_t symbol)
{
  if (symbol->callback)
    return (symbol->callback)(symbol->data);
  else
    return *(int*) symbol->data;
}

XBT_PUBLIC xbt_automaton_propositional_symbol_callback_type
xbt_automaton_propositional_symbol_get_callback(xbt_automaton_propositional_symbol_t symbol)
{
  return symbol->callback;
}

XBT_PUBLIC void* xbt_automaton_propositional_symbol_get_data(xbt_automaton_propositional_symbol_t symbol)
{
  return symbol->data;
}

XBT_PUBLIC const char* xbt_automaton_propositional_symbol_get_name(xbt_automaton_propositional_symbol_t symbol)
{
  return symbol->pred;
}

int xbt_automaton_state_compare(xbt_automaton_state_t s1, xbt_automaton_state_t s2){
  /* single id for each state, id and type sufficient for comparison*/
  return (strcmp(s1->id, s2->id) != 0) || (s1->type != s2->type);
}

int xbt_automaton_transition_compare(xbt_automaton_transition_t t1, xbt_automaton_transition_t t2)
{
  return xbt_automaton_state_compare(t1->src, t2->src) || xbt_automaton_state_compare(t1->dst, t2->dst) ||
         xbt_automaton_exp_label_compare(t1->label, t2->label);
}

int xbt_automaton_exp_label_compare(xbt_automaton_exp_label_t l1, xbt_automaton_exp_label_t l2){

  if(l1->type != l2->type)
    return 1;

  int res;
  switch(l1->type){
  case 0 : // OR
  case 1 : // AND
    res = xbt_automaton_exp_label_compare(l1->u.or_and.left_exp, l2->u.or_and.left_exp) ||
          xbt_automaton_exp_label_compare(l1->u.or_and.right_exp, l2->u.or_and.right_exp);
    break;
  case 2 : // NOT
    res = xbt_automaton_exp_label_compare(l1->u.exp_not, l2->u.exp_not);
    break;
  case 3 : // predicat
    res = strcmp(l1->u.predicat, l2->u.predicat) != 0;
    break;
  case 4 : // 1
    res = 0;
    break;
  default :
    res = -1;
    break;
  }
  return res;
}

int xbt_automaton_propositional_symbols_compare_value(xbt_dynar_t s1, xbt_dynar_t s2){
  unsigned int nb_elem = xbt_dynar_length(s1);

  for (unsigned int cursor = 0; cursor < nb_elem; cursor++) {
    int* iptr1 = xbt_dynar_get_ptr(s1, cursor);
    int* iptr2 = xbt_dynar_get_ptr(s2, cursor);
    if(*iptr1 != *iptr2)
      return 1;
  }

  return 0;
}

static void xbt_automaton_transition_free(xbt_automaton_transition_t t);
static void xbt_automaton_exp_label_free(xbt_automaton_exp_label_t e);
static void xbt_automaton_propositional_symbol_free(xbt_automaton_propositional_symbol_t ps);

void xbt_automaton_state_free(xbt_automaton_state_t s){
  if (s != NULL) {
    xbt_free(s->id);
    xbt_dynar_free(&(s->in));
    xbt_dynar_free(&(s->out));
    xbt_free(s);
  }
}

void xbt_automaton_state_free_voidp(void *s){
  xbt_automaton_state_free((xbt_automaton_state_t) * (void **) s);
}

static void xbt_automaton_transition_free(xbt_automaton_transition_t t){
  if(t){
    xbt_automaton_exp_label_free(t->label);
    xbt_free(t);
    t = NULL;
  }
}

void xbt_automaton_transition_free_voidp(void *t){
  xbt_automaton_transition_free((xbt_automaton_transition_t) * (void **) t);
}

static void xbt_automaton_exp_label_free(xbt_automaton_exp_label_t e){
  if(e){
    switch(e->type){
    case 0:
    case 1:
      xbt_automaton_exp_label_free(e->u.or_and.left_exp);
      xbt_automaton_exp_label_free(e->u.or_and.right_exp);
      break;
    case 2:
      xbt_automaton_exp_label_free(e->u.exp_not);
      break;
    case 3:
      xbt_free(e->u.predicat);
      break;
    default:
      break;
    }
    xbt_free(e);
    e = NULL;
  }
}

void xbt_automaton_exp_label_free_voidp(void *e){
  xbt_automaton_exp_label_free((xbt_automaton_exp_label_t) * (void **) e);
}

static void xbt_automaton_propositional_symbol_free(xbt_automaton_propositional_symbol_t ps){
  if(ps){
    xbt_free(ps->pred);
    xbt_free(ps);
    ps = NULL;
  }
}

void xbt_automaton_propositional_symbol_free_voidp(void *ps){
  xbt_automaton_propositional_symbol_t symbol = (xbt_automaton_propositional_symbol_t) * (void **) ps;
  if (symbol->free_function)
    symbol->free_function(symbol->data);
  xbt_automaton_propositional_symbol_free(symbol);
}

void xbt_automaton_free(xbt_automaton_t a){
  if(a){
    xbt_dynar_free(&(a->propositional_symbols));
    xbt_dynar_free(&(a->transitions));
    xbt_dynar_free(&(a->states));
    xbt_automaton_state_free(a->current_state);
    xbt_free(a);
    a = NULL;
  }
}
