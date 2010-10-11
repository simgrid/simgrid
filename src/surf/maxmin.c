/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/mallocator.h"
#include "maxmin_private.h"
#include <stdlib.h>
#include <stdio.h>              /* sprintf */
#include <math.h>
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_maxmin, surf,
                                "Logging specific to SURF (maxmin)");

static void *lmm_variable_mallocator_new_f(void);
static void lmm_variable_mallocator_free_f(void *var);
static void lmm_variable_mallocator_reset_f(void *var);
static void lmm_update_modified_set(lmm_system_t sys,
                                    lmm_constraint_t cnst);
static void lmm_remove_all_modified_set(lmm_system_t sys);
int sg_maxmin_selective_update = 1;
static int Global_debug_id = 1;
static int Global_const_debug_id = 1;
lmm_system_t lmm_system_new(void)
{
  lmm_system_t l = NULL;
  s_lmm_variable_t var;
  s_lmm_constraint_t cnst;

  l = xbt_new0(s_lmm_system_t, 1);

  l->modified = 0;
  l->selective_update_active = sg_maxmin_selective_update;

  DEBUG1("Setting selective_update_active flag to %d\n",
         l->selective_update_active);

  xbt_swag_init(&(l->variable_set),
                xbt_swag_offset(var, variable_set_hookup));
  xbt_swag_init(&(l->constraint_set),
                xbt_swag_offset(cnst, constraint_set_hookup));

  xbt_swag_init(&(l->active_constraint_set),
                xbt_swag_offset(cnst, active_constraint_set_hookup));

  xbt_swag_init(&(l->modified_constraint_set),
                xbt_swag_offset(cnst, modified_constraint_set_hookup));
  xbt_swag_init(&(l->saturated_variable_set),
                xbt_swag_offset(var, saturated_variable_set_hookup));
  xbt_swag_init(&(l->saturated_constraint_set),
                xbt_swag_offset(cnst, saturated_constraint_set_hookup));

  l->variable_mallocator = xbt_mallocator_new(64,
                                              lmm_variable_mallocator_new_f,
                                              lmm_variable_mallocator_free_f,
                                              lmm_variable_mallocator_reset_f);

  return l;
}

void lmm_system_free(lmm_system_t sys)
{
  lmm_variable_t var = NULL;
  lmm_constraint_t cnst = NULL;

  while ((var = extract_variable(sys))) {
    WARN2
        ("Variable %p (%d) still in LMM system when freing it: this may be a bug",
         var, var->id_int);
    lmm_var_free(sys, var);
  }

  while ((cnst = extract_constraint(sys)))
    lmm_cnst_free(sys, cnst);

  xbt_mallocator_free(sys->variable_mallocator);
  free(sys);
}

XBT_INLINE void lmm_variable_disable(lmm_system_t sys, lmm_variable_t var)
{
  int i;
  lmm_element_t elem = NULL;

  XBT_IN2("(sys=%p, var=%p)", sys, var);
  sys->modified = 1;

  for (i = 0; i < var->cnsts_number; i++) {
    elem = &var->cnsts[i];
    xbt_swag_remove(elem, &(elem->constraint->element_set));
    xbt_swag_remove(elem, &(elem->constraint->active_element_set));
    if (!xbt_swag_size(&(elem->constraint->element_set)))
      make_constraint_inactive(sys, elem->constraint);
    else
      lmm_update_modified_set(sys, elem->constraint);
  }
  var->cnsts_number = 0;
  XBT_OUT;
}

static void lmm_var_free(lmm_system_t sys, lmm_variable_t var)
{

  lmm_variable_disable(sys, var);
  free(var->cnsts);
  xbt_mallocator_release(sys->variable_mallocator, var);
}

static XBT_INLINE void lmm_cnst_free(lmm_system_t sys,
                                     lmm_constraint_t cnst)
{
/*   xbt_assert0(xbt_swag_size(&(cnst->element_set)), */
/* 	      "This list should be empty!"); */
  remove_active_constraint(sys, cnst);
  free(cnst);
}

lmm_constraint_t lmm_constraint_new(lmm_system_t sys, void *id,
                                    double bound_value)
{
  lmm_constraint_t cnst = NULL;
  s_lmm_element_t elem;

  cnst = xbt_new0(s_lmm_constraint_t, 1);
  cnst->id = id;
  cnst->id_int = Global_const_debug_id++;
  xbt_swag_init(&(cnst->element_set),
                xbt_swag_offset(elem, element_set_hookup));
  xbt_swag_init(&(cnst->active_element_set),
                xbt_swag_offset(elem, active_element_set_hookup));

  cnst->bound = bound_value;
  cnst->usage = 0;
  cnst->shared = 1;
  insert_constraint(sys, cnst);

  return cnst;
}

XBT_INLINE void lmm_constraint_shared(lmm_constraint_t cnst)
{
  cnst->shared = 0;
}

XBT_INLINE int lmm_constraint_is_shared(lmm_constraint_t cnst)
{
  return (cnst->shared);
}

XBT_INLINE void lmm_constraint_free(lmm_system_t sys,
                                    lmm_constraint_t cnst)
{
  remove_constraint(sys, cnst);
  lmm_cnst_free(sys, cnst);
}

static void *lmm_variable_mallocator_new_f(void)
{
  return xbt_new(s_lmm_variable_t, 1);
}

static void lmm_variable_mallocator_free_f(void *var)
{
  xbt_free(var);
}

static void lmm_variable_mallocator_reset_f(void *var)
{
  /* memset to zero like calloc */
  memset(var, 0, sizeof(s_lmm_variable_t));
}

lmm_variable_t lmm_variable_new(lmm_system_t sys, void *id,
                                double weight,
                                double bound, int number_of_constraints)
{
  lmm_variable_t var = NULL;
  int i;

  XBT_IN5("(sys=%p, id=%p, weight=%f, bound=%f, num_cons =%d)",
          sys, id, weight, bound, number_of_constraints);

  var = xbt_mallocator_get(sys->variable_mallocator);
  var->id = id;
  var->id_int = Global_debug_id++;
  var->cnsts = xbt_new0(s_lmm_element_t, number_of_constraints);
  for (i = 0; i < number_of_constraints; i++) {
    /* Should be useless because of the 
       calloc but it seems to help valgrind... */
    var->cnsts[i].element_set_hookup.next = NULL;
    var->cnsts[i].element_set_hookup.prev = NULL;
    var->cnsts[i].active_element_set_hookup.next = NULL;
    var->cnsts[i].active_element_set_hookup.prev = NULL;
    var->cnsts[i].constraint = NULL;
    var->cnsts[i].variable = NULL;
    var->cnsts[i].value = 0.0;
  }
  var->cnsts_size = number_of_constraints;
  var->cnsts_number = 0;        /* Should be useless because of the 
                                   calloc but it seems to help valgrind... */
  var->weight = weight;
  var->bound = bound;
  var->value = 0.0;


  var->func_f = func_f_def;
  var->func_fp = func_fp_def;
  var->func_fpi = func_fpi_def;

  if (weight)
    xbt_swag_insert_at_head(var, &(sys->variable_set));
  else
    xbt_swag_insert_at_tail(var, &(sys->variable_set));
  XBT_OUT;
  return var;
}

void lmm_variable_free(lmm_system_t sys, lmm_variable_t var)
{
  remove_variable(sys, var);
  lmm_var_free(sys, var);
}

XBT_INLINE double lmm_variable_getvalue(lmm_variable_t var)
{
  return (var->value);
}

XBT_INLINE double lmm_variable_getbound(lmm_variable_t var)
{
  return (var->bound);
}

void lmm_expand(lmm_system_t sys, lmm_constraint_t cnst,
                lmm_variable_t var, double value)
{
  lmm_element_t elem = NULL;

  sys->modified = 1;

  xbt_assert0(var->cnsts_number < var->cnsts_size, "Too much constraints");

  elem = &(var->cnsts[var->cnsts_number++]);

  elem->value = value;
  elem->constraint = cnst;
  elem->variable = var;

  if (var->weight)
    xbt_swag_insert_at_head(elem, &(elem->constraint->element_set));
  else
    xbt_swag_insert_at_tail(elem, &(elem->constraint->element_set));

  make_constraint_active(sys, cnst);
  lmm_update_modified_set(sys, cnst);
}

void lmm_expand_add(lmm_system_t sys, lmm_constraint_t cnst,
                    lmm_variable_t var, double value)
{
  int i;
  sys->modified = 1;

  for (i = 0; i < var->cnsts_number; i++)
    if (var->cnsts[i].constraint == cnst)
      break;

  if (i < var->cnsts_number) {
    if (cnst->shared)
      var->cnsts[i].value += value;
    else
      var->cnsts[i].value = MAX(var->cnsts[i].value, value);
    lmm_update_modified_set(sys, cnst);
  } else
    lmm_expand(sys, cnst, var, value);
}

void lmm_elem_set_value(lmm_system_t sys, lmm_constraint_t cnst,
                        lmm_variable_t var, double value)
{
  int i;

  for (i = 0; i < var->cnsts_number; i++)
    if (var->cnsts[i].constraint == cnst)
      break;

  if (i < var->cnsts_number) {
    var->cnsts[i].value = value;
    sys->modified = 1;
    lmm_update_modified_set(sys, cnst);
  } else
    DIE_IMPOSSIBLE;
}

XBT_INLINE lmm_constraint_t lmm_get_cnst_from_var(lmm_system_t sys,
                                                  lmm_variable_t var,
                                                  int num)
{
  if (num < var->cnsts_number)
    return (var->cnsts[num].constraint);
  else
    return NULL;
}

XBT_INLINE int lmm_get_number_of_cnst_from_var(lmm_system_t sys,
                                               lmm_variable_t var)
{
  return (var->cnsts_number);
}

lmm_variable_t lmm_get_var_from_cnst(lmm_system_t sys,
                                     lmm_constraint_t cnst,
                                     lmm_element_t * elem)
{
  if (!(*elem))
    *elem = xbt_swag_getFirst(&(cnst->element_set));
  else
    *elem = xbt_swag_getNext(*elem, cnst->element_set.offset);
  if (*elem)
    return (*elem)->variable;
  else
    return NULL;
}

XBT_INLINE void *lmm_constraint_id(lmm_constraint_t cnst)
{
  return cnst->id;
}

XBT_INLINE void *lmm_variable_id(lmm_variable_t var)
{
  return var->id;
}

static XBT_INLINE void saturated_constraint_set_update(lmm_system_t sys,
                                                       lmm_constraint_t
                                                       cnst,
                                                       double *min_usage)
{
  lmm_constraint_t useless_cnst = NULL;

  XBT_IN3("sys=%p, cnst=%p, min_usage=%f", sys, cnst, *min_usage);
  if (cnst->usage <= 0) {
    XBT_OUT;
    return;
  }
  if (cnst->remaining <= 0) {
    XBT_OUT;
    return;
  }
  if ((*min_usage < 0) || (*min_usage > cnst->remaining / cnst->usage)) {
    *min_usage = cnst->remaining / cnst->usage;
    LOG3(xbt_log_priority_trace,
         "min_usage=%f (cnst->remaining=%f, cnst->usage=%f)", *min_usage,
         cnst->remaining, cnst->usage);
    while ((useless_cnst =
            xbt_swag_getFirst(&(sys->saturated_constraint_set))))
      xbt_swag_remove(useless_cnst, &(sys->saturated_constraint_set));

    xbt_swag_insert(cnst, &(sys->saturated_constraint_set));
  } else if (*min_usage == cnst->remaining / cnst->usage) {
    xbt_swag_insert(cnst, &(sys->saturated_constraint_set));
  }
  XBT_OUT;
}

static XBT_INLINE void saturated_variable_set_update(lmm_system_t sys)
{
  lmm_constraint_t cnst = NULL;
  xbt_swag_t cnst_list = NULL;
  lmm_element_t elem = NULL;
  xbt_swag_t elem_list = NULL;

  cnst_list = &(sys->saturated_constraint_set);
  while ((cnst = xbt_swag_getFirst(cnst_list))) {
    elem_list = &(cnst->active_element_set);
    xbt_swag_foreach(elem, elem_list) {
      if (elem->variable->weight <= 0)
        break;
      if ((elem->value > 0))
        xbt_swag_insert(elem->variable, &(sys->saturated_variable_set));
    }
    xbt_swag_remove(cnst, cnst_list);
  }
}

void lmm_print(lmm_system_t sys)
{
  lmm_constraint_t cnst = NULL;
  lmm_element_t elem = NULL;
  lmm_variable_t var = NULL;
  xbt_swag_t cnst_list = NULL;
  xbt_swag_t var_list = NULL;
  xbt_swag_t elem_list = NULL;
  char print_buf[1024];
  char *trace_buf = xbt_malloc0(sizeof(char));
  double sum = 0.0;

  /* Printing Objective */
  var_list = &(sys->variable_set);
  sprintf(print_buf, "MAX-MIN ( ");
  trace_buf =
      xbt_realloc(trace_buf, strlen(trace_buf) + strlen(print_buf) + 1);
  strcat(trace_buf, print_buf);
  xbt_swag_foreach(var, var_list) {
    sprintf(print_buf, "'%d'(%f) ", var->id_int, var->weight);
    trace_buf =
        xbt_realloc(trace_buf, strlen(trace_buf) + strlen(print_buf) + 1);
    strcat(trace_buf, print_buf);
  }
  sprintf(print_buf, ")");
  trace_buf =
      xbt_realloc(trace_buf, strlen(trace_buf) + strlen(print_buf) + 1);
  strcat(trace_buf, print_buf);
  fprintf(stderr, "%s", trace_buf);
  //DEBUG1("%20s", trace_buf); FIXME
  trace_buf[0] = '\000';

  DEBUG0("Constraints");
  /* Printing Constraints */
  cnst_list = &(sys->active_constraint_set);
  xbt_swag_foreach(cnst, cnst_list) {
    sum = 0.0;
    elem_list = &(cnst->element_set);
    sprintf(print_buf, "\t");
    trace_buf =
        xbt_realloc(trace_buf, strlen(trace_buf) + strlen(print_buf) + 1);
    strcat(trace_buf, print_buf);
    xbt_swag_foreach(elem, elem_list) {
      sprintf(print_buf, "%f.'%d'(%f) + ", elem->value,
              elem->variable->id_int, elem->variable->value);
      trace_buf =
          xbt_realloc(trace_buf,
                      strlen(trace_buf) + strlen(print_buf) + 1);
      strcat(trace_buf, print_buf);
      sum += elem->value * elem->variable->value;
    }
    sprintf(print_buf, "0 <= %f ('%d')", cnst->bound, cnst->id_int);
    trace_buf =
        xbt_realloc(trace_buf, strlen(trace_buf) + strlen(print_buf) + 1);
    strcat(trace_buf, print_buf);

    if (!cnst->shared) {
      sprintf(print_buf, " [MAX-Constraint]");
      trace_buf =
          xbt_realloc(trace_buf,
                      strlen(trace_buf) + strlen(print_buf) + 1);
      strcat(trace_buf, print_buf);
    }
    //   DEBUG1("%s", trace_buf);
    fprintf(stderr, "%s", trace_buf);
    trace_buf[0] = '\000';
    xbt_assert3(!double_positive(sum - cnst->bound),
                "Incorrect value (%f is not smaller than %f): %g",
                sum, cnst->bound, sum - cnst->bound);
  }

  DEBUG0("Variables");
  /* Printing Result */
  xbt_swag_foreach(var, var_list) {
    if (var->bound > 0) {
      DEBUG4("'%d'(%f) : %f (<=%f)", var->id_int, var->weight, var->value,
             var->bound);
      xbt_assert2(!double_positive(var->value - var->bound),
                  "Incorrect value (%f is not smaller than %f",
                  var->value, var->bound);
    } else {
      DEBUG3("'%d'(%f) : %f", var->id_int, var->weight, var->value);
    }
  }

  free(trace_buf);
}

void lmm_solve(lmm_system_t sys)
{
  lmm_variable_t var = NULL;
  lmm_constraint_t cnst = NULL;
  lmm_element_t elem = NULL;
  xbt_swag_t cnst_list = NULL;
  xbt_swag_t var_list = NULL;
  xbt_swag_t elem_list = NULL;
  double min_usage = -1;
  double min_bound = -1;

  if (!(sys->modified))
    return;

  /*
   * Compute Usage and store the variables that reach the maximum.
   */
  cnst_list =
      sys->
      selective_update_active ? &(sys->modified_constraint_set) :
      &(sys->active_constraint_set);

  DEBUG1("Active constraints : %d", xbt_swag_size(cnst_list));
  /* Init: Only modified code portions */
  xbt_swag_foreach(cnst, cnst_list) {
    elem_list = &(cnst->element_set);
    //DEBUG1("Variable set : %d", xbt_swag_size(elem_list));
    xbt_swag_foreach(elem, elem_list) {
      var = elem->variable;
      if (var->weight <= 0.0)
        break;
      var->value = 0.0;
    }
  }

  DEBUG1("Active constraints : %d", xbt_swag_size(cnst_list));
  xbt_swag_foreach(cnst, cnst_list) {
    /* INIT */
    cnst->remaining = cnst->bound;
    if (cnst->remaining == 0)
      continue;
    cnst->usage = 0;
    elem_list = &(cnst->element_set);
    xbt_swag_foreach(elem, elem_list) {
      /* 0-weighted elements (ie, sleep actions) are at the end of the swag and we don't want to consider them */
      if (elem->variable->weight <= 0)
        break;
      if ((elem->value > 0)) {
        if (cnst->shared)
          cnst->usage += elem->value / elem->variable->weight;
        else if (cnst->usage < elem->value / elem->variable->weight)
          cnst->usage = elem->value / elem->variable->weight;

        make_elem_active(elem);
      }
    }
    DEBUG2("Constraint Usage %d : %f", cnst->id_int, cnst->usage);
    /* Saturated constraints update */
    saturated_constraint_set_update(sys, cnst, &min_usage);
  }
  saturated_variable_set_update(sys);

  /* Saturated variables update */

  do {
    /* Fix the variables that have to be */
    var_list = &(sys->saturated_variable_set);

    xbt_swag_foreach(var, var_list) {
      if (var->weight <= 0.0)
        DIE_IMPOSSIBLE;
      /* First check if some of these variables have reach their upper
         bound and update min_usage accordingly. */
      DEBUG5
          ("var=%d, var->bound=%f, var->weight=%f, min_usage=%f, var->bound*var->weight=%f",
           var->id_int, var->bound, var->weight, min_usage,
           var->bound * var->weight);
      if ((var->bound > 0) && (var->bound * var->weight < min_usage)) {
        if (min_bound < 0)
          min_bound = var->bound;
        else
          min_bound = MIN(min_bound, var->bound);
        DEBUG1("Updated min_bound=%f", min_bound);
      }
    }


    while ((var = xbt_swag_getFirst(var_list))) {
      int i;

      if (min_bound < 0) {
        var->value = min_usage / var->weight;
      } else {
        if (min_bound == var->bound)
          var->value = var->bound;
        else {
          xbt_swag_remove(var, var_list);
          continue;
        }
      }
      DEBUG5("Min usage: %f, Var(%d)->weight: %f, Var(%d)->value: %f ",
             min_usage, var->id_int, var->weight, var->id_int, var->value);


      /* Update usage */

      for (i = 0; i < var->cnsts_number; i++) {
        elem = &var->cnsts[i];
        cnst = elem->constraint;
        if (cnst->shared) {
          double_update(&(cnst->remaining), elem->value * var->value);
          double_update(&(cnst->usage), elem->value / var->weight);
          make_elem_inactive(elem);
        } else {                /* FIXME one day: We recompute usage.... :( */
          cnst->usage = 0.0;
          make_elem_inactive(elem);
          elem_list = &(cnst->element_set);
          xbt_swag_foreach(elem, elem_list) {
            if (elem->variable->weight <= 0)
              break;
            if (elem->variable->value > 0)
              break;
            if ((elem->value > 0)) {
              cnst->usage =
                  MAX(cnst->usage, elem->value / elem->variable->weight);
              DEBUG2("Constraint Usage %d : %f", cnst->id_int,
                     cnst->usage);
              make_elem_active(elem);
            }
          }
        }
      }
      xbt_swag_remove(var, var_list);
    }

    /* Find out which variables reach the maximum */
    cnst_list =
        sys->selective_update_active ? &(sys->modified_constraint_set) :
        &(sys->active_constraint_set);
    min_usage = -1;
    min_bound = -1;
    xbt_swag_foreach(cnst, cnst_list) {
      saturated_constraint_set_update(sys, cnst, &min_usage);
    }
    saturated_variable_set_update(sys);

  } while (xbt_swag_size(&(sys->saturated_variable_set)));

  sys->modified = 0;
  if (sys->selective_update_active)
    lmm_remove_all_modified_set(sys);

  if (XBT_LOG_ISENABLED(surf_maxmin, xbt_log_priority_debug)) {
    lmm_print(sys);
  }
}

/* Not a O(1) function */

void lmm_update(lmm_system_t sys, lmm_constraint_t cnst,
                lmm_variable_t var, double value)
{
  int i;

  for (i = 0; i < var->cnsts_number; i++)
    if (var->cnsts[i].constraint == cnst) {
      var->cnsts[i].value = value;
      sys->modified = 1;
      lmm_update_modified_set(sys, cnst);
      return;
    }
}

/** \brief Attribute the value bound to var->bound.
 * 
 *  \param sys the lmm_system_t
 *  \param var the lmm_variable_t
 *  \param bound the new bound to associate with var
 * 
 *  Makes var->bound equal to bound. Whenever this function is called 
 *  a change is  signed in the system. To
 *  avoid false system changing detection it is a good idea to test 
 *  (bound != 0) before calling it.
 *
 */
void lmm_update_variable_bound(lmm_system_t sys, lmm_variable_t var,
                               double bound)
{
  int i;

  sys->modified = 1;
  var->bound = bound;

  for (i = 0; i < var->cnsts_number; i++)
    lmm_update_modified_set(sys, var->cnsts[i].constraint);

}


void lmm_update_variable_weight(lmm_system_t sys, lmm_variable_t var,
                                double weight)
{
  int i;
  lmm_element_t elem;

  if (weight == var->weight)
    return;
  XBT_IN3("(sys=%p, var=%p, weight=%f)", sys, var, weight);
  sys->modified = 1;
  var->weight = weight;
  xbt_swag_remove(var, &(sys->variable_set));
  if (weight)
    xbt_swag_insert_at_head(var, &(sys->variable_set));
  else
    xbt_swag_insert_at_tail(var, &(sys->variable_set));

  for (i = 0; i < var->cnsts_number; i++) {
    elem = &var->cnsts[i];
    xbt_swag_remove(elem, &(elem->constraint->element_set));
    if (weight)
      xbt_swag_insert_at_head(elem, &(elem->constraint->element_set));
    else
      xbt_swag_insert_at_tail(elem, &(elem->constraint->element_set));

    lmm_update_modified_set(sys, elem->constraint);
  }
  if (!weight)
    var->value = 0.0;

  XBT_OUT;
}

XBT_INLINE double lmm_get_variable_weight(lmm_variable_t var)
{
  return var->weight;
}

XBT_INLINE void lmm_update_constraint_bound(lmm_system_t sys,
                                            lmm_constraint_t cnst,
                                            double bound)
{
  sys->modified = 1;
  lmm_update_modified_set(sys, cnst);
  cnst->bound = bound;
}

XBT_INLINE int lmm_constraint_used(lmm_system_t sys, lmm_constraint_t cnst)
{
  return xbt_swag_belongs(cnst, &(sys->active_constraint_set));
}

XBT_INLINE lmm_constraint_t lmm_get_first_active_constraint(lmm_system_t
                                                            sys)
{
  return xbt_swag_getFirst(&(sys->active_constraint_set));
}

XBT_INLINE lmm_constraint_t lmm_get_next_active_constraint(lmm_system_t
                                                           sys,
                                                           lmm_constraint_t
                                                           cnst)
{
  return xbt_swag_getNext(cnst, (sys->active_constraint_set).offset);
}

#ifdef HAVE_LATENCY_BOUND_TRACKING
XBT_INLINE int lmm_is_variable_limited_by_latency(lmm_variable_t var)
{
  return (double_equals(var->bound, var->value));
}
#endif


/** \brief Update the constraint set propagating recursively to
 *  other constraints so the system should not be entirely computed.
 *
 *  \param sys the lmm_system_t
 *  \param cnst the lmm_constraint_t affected by the change
 *
 *  A recursive algorithm to optimize the system recalculation selecting only
 *  constraints that have changed. Each constraint change is propagated
 *  to the list of constraints for each variable.
 */
static void lmm_update_modified_set(lmm_system_t sys,
                                    lmm_constraint_t cnst)
{
  lmm_element_t elem = NULL;
  lmm_variable_t var = NULL;
  xbt_swag_t elem_list = NULL;
  int i;

  /* return if selective update isn't active */
  if (!sys->selective_update_active)
    return;

  //DEBUG1("Updating modified constraint set with constraint %d", cnst->id_int);

  if (xbt_swag_belongs(cnst, &(sys->modified_constraint_set)))
    return;

  //DEBUG1("Inserting into modified constraint set %d", cnst->id_int);

  /* add to modified set */
  xbt_swag_insert(cnst, &(sys->modified_constraint_set));

  elem_list = &(cnst->element_set);
  xbt_swag_foreach(elem, elem_list) {
    var = elem->variable;
    for (i = 0; i < var->cnsts_number; i++)
      if (cnst != var->cnsts[i].constraint) {
        //DEBUG2("Updating modified %d calling for %d", cnst->id_int, var->cnsts[i].constraint->id_int);
        lmm_update_modified_set(sys, var->cnsts[i].constraint);
      }
  }
}

/** \brief Remove all constraints of the modified_constraint_set.
 *
 *  \param sys the lmm_system_t
 */
static void lmm_remove_all_modified_set(lmm_system_t sys)
{
  lmm_element_t elem = NULL;
  lmm_element_t elem_next = NULL;
  xbt_swag_t elem_list = NULL;

  elem_list = &(sys->modified_constraint_set);
  xbt_swag_foreach_safe(elem, elem_next, elem_list) {
    xbt_swag_remove(elem, elem_list);
  }
}
