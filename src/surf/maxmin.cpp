/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* \file callbacks.h */

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/mallocator.h"
#include "maxmin_private.hpp"
#include <stdlib.h>
#include <stdio.h>              /* sprintf */
#include <math.h>
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_maxmin, surf,
                                "Logging specific to SURF (maxmin)");

typedef struct s_dyn_light {
  int *data;
  int pos;
  int size;
} s_dyn_light_t, *dyn_light_t;

double sg_maxmin_precision = 0.00001;
double sg_surf_precision   = 0.00001;

static void *lmm_variable_mallocator_new_f(void);
static void lmm_variable_mallocator_free_f(void *var);
#define lmm_variable_mallocator_reset_f ((void_f_pvoid_t)NULL)
static void lmm_update_modified_set(lmm_system_t sys,
                                    lmm_constraint_t cnst);
static void lmm_remove_all_modified_set(lmm_system_t sys);
static int Global_debug_id = 1;
static int Global_const_debug_id = 1;

static void lmm_var_free(lmm_system_t sys, lmm_variable_t var);
static inline void lmm_cnst_free(lmm_system_t sys,
                                     lmm_constraint_t cnst);

static void lmm_on_disabled_var(lmm_system_t sys, lmm_constraint_t cnstr);
static void lmm_enable_var(lmm_system_t sys, lmm_variable_t var);
static int lmm_can_enable_var(lmm_variable_t var);
static void lmm_disable_var(lmm_system_t sys, lmm_variable_t var);
static int lmm_concurrency_slack(lmm_constraint_t cnstr);
static int lmm_cnstrs_min_concurrency_slack(lmm_variable_t var);

static void lmm_check_concurrency(lmm_system_t sys);

inline int lmm_element_concurrency(lmm_element_t elem) {
  //Ignore element with weight less than one (e.g. cross-traffic)
  return (elem->value>=1)?1:0;
  //There are other alternatives, but they will change the behaviour of the model..
  //So do not use it unless you want to make a new model.
  //If you do, remember to change the variables concurrency share to reflect it.
  //Potential examples are:
  //return (elem->weight>0)?1:0;//Include element as soon  as weight is non-zero
  //return (int)ceil(elem->weight);//Include element as the rounded-up integer value of the element weight
}

inline void lmm_decrease_concurrency(lmm_element_t elem) {
  xbt_assert(elem->constraint->concurrency_current>=lmm_element_concurrency(elem));
  elem->constraint->concurrency_current-=lmm_element_concurrency(elem);
}

inline void lmm_increase_concurrency(lmm_element_t elem) {

  elem->constraint->concurrency_current+= lmm_element_concurrency(elem);

  lmm_constraint_t cnstr=elem->constraint;
  
  if(cnstr->concurrency_current > cnstr->concurrency_maximum)
    cnstr->concurrency_maximum= cnstr->concurrency_current;

  xbt_assert(cnstr->concurrency_limit<0 || cnstr->concurrency_current<=cnstr->concurrency_limit,"Concurrency limit overflow!");
}

lmm_system_t lmm_system_new(int selective_update)
{
  lmm_system_t l = NULL;
  s_lmm_variable_t var;
  s_lmm_constraint_t cnst;

  l = xbt_new0(s_lmm_system_t, 1);

  l->modified = 0;
  l->selective_update_active = selective_update;
  l->visited_counter = 1;

  XBT_DEBUG("Setting selective_update_active flag to %d",
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

  l->variable_mallocator = xbt_mallocator_new(65536,
                                              lmm_variable_mallocator_new_f,
                                              lmm_variable_mallocator_free_f,
                                              lmm_variable_mallocator_reset_f);

  return l;
}

void lmm_system_free(lmm_system_t sys)
{
  lmm_variable_t var = NULL;
  lmm_constraint_t cnst = NULL;

  
  while ((var = (lmm_variable_t) extract_variable(sys))) {
    XBT_WARN("Variable %d still in system when freing it: this may be a bug", var->id_int);
    lmm_var_free(sys, var);
  }

  while ((cnst = (lmm_constraint_t) extract_constraint(sys)))
    lmm_cnst_free(sys, cnst);

  xbt_mallocator_free(sys->variable_mallocator);
  free(sys);
}

static inline void lmm_variable_remove(lmm_system_t sys, lmm_variable_t var)
{
  int i;
  int nelements;
  
  lmm_element_t elem = NULL;
  
  XBT_IN("(sys=%p, var=%p)", sys, var);
  sys->modified = 1;

  //TODOLATER Can do better than that by leaving only the variable in only one enabled_element_set, call lmm_update_modified_set, and then remove it..
  if(var->cnsts_number)
      lmm_update_modified_set(sys, var->cnsts[0].constraint);

  for (i = 0; i < var->cnsts_number; i++) {
    elem = &var->cnsts[i];
    if(var->weight>0)
    lmm_decrease_concurrency(elem);
    xbt_swag_remove(elem, &(elem->constraint->enabled_element_set));
    xbt_swag_remove(elem, &(elem->constraint->disabled_element_set));
    xbt_swag_remove(elem, &(elem->constraint->active_element_set));
    nelements=xbt_swag_size(&(elem->constraint->enabled_element_set)) +
              xbt_swag_size(&(elem->constraint->disabled_element_set));
    if (!nelements)
      make_constraint_inactive(sys, elem->constraint);
    else
      lmm_on_disabled_var(sys,elem->constraint);
  }
  
  //Check if we can enable new variables going through the constraints where var was.
  //Do it after removing all elements, so he first disabled variables get priority over those with smaller
  //requirement
  for (i = 0; i < var->cnsts_number; i++) {
    elem = &var->cnsts[i];
    if(xbt_swag_size(&(elem->constraint->disabled_element_set)))
      lmm_on_disabled_var(sys,elem->constraint);  
  }
  
  var->cnsts_number = 0;

  lmm_check_concurrency(sys);
    
  XBT_OUT();
}

static void lmm_var_free(lmm_system_t sys, lmm_variable_t var)
{

  lmm_variable_remove(sys, var);
  xbt_mallocator_release(sys->variable_mallocator, var);
}

static inline void lmm_cnst_free(lmm_system_t sys,
                                     lmm_constraint_t cnst)
{
  make_constraint_inactive(sys, cnst);
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
  xbt_swag_init(&(cnst->enabled_element_set),
                xbt_swag_offset(elem, enabled_element_set_hookup));
  xbt_swag_init(&(cnst->disabled_element_set),
                xbt_swag_offset(elem, disabled_element_set_hookup));
  xbt_swag_init(&(cnst->active_element_set),
                xbt_swag_offset(elem, active_element_set_hookup));

  cnst->bound = bound_value;
  cnst->concurrency_maximum=0;
  cnst->concurrency_current=0;
  //TODO MARTIN Maybe a configuration item for the default cap concurrency? 
  cnst->concurrency_limit=100;
  cnst->usage = 0;
  cnst->sharing_policy = 1; /* FIXME: don't hardcode the value */
  insert_constraint(sys, cnst);

  return cnst;
}

int lmm_constraint_concurrency_limit_get(lmm_constraint_t cnst)
{
 return cnst->concurrency_limit;
}

void lmm_constraint_concurrency_limit_set(lmm_constraint_t cnst, int concurrency_limit)
{
  xbt_assert(concurrency_limit<0 || cnst->concurrency_maximum<=concurrency_limit,"New concurrency limit should be larger than observed concurrency maximum. Maybe you want to call  lmm_constraint_concurrency_maximum_reset() to reset the maximum?");
  cnst->concurrency_limit = concurrency_limit;
}

void lmm_constraint_concurrency_maximum_reset(lmm_constraint_t cnst)
{
  cnst->concurrency_maximum = 0;
}

int lmm_constraint_concurrency_maximum_get(lmm_constraint_t cnst)
{
 xbt_assert(cnst->concurrency_limit<0 || cnst->concurrency_maximum<=cnst->concurrency_limit,"Very bad: maximum observed concurrency is higher than limit. This is a bug of SURF, please report it.");
  return cnst->concurrency_maximum;
}

void lmm_constraint_shared(lmm_constraint_t cnst)
{
  cnst->sharing_policy = 0;
}

/** Return true if the constraint is shared, and false if it's FATPIPE */
int lmm_constraint_sharing_policy(lmm_constraint_t cnst)
{
  return (cnst->sharing_policy);
}

/* @brief Remove a constraint 
 * Currently this is dead code, but it is exposed in maxmin.h
 * Apparently, this call was designed assuming that constraint would no more have elements in it. 
 * If this is not the case, assertion will fail, and you need to add calls e.g. to lmm_shrink before effectively removing it.
 */
inline void lmm_constraint_free(lmm_system_t sys,
                                    lmm_constraint_t cnst)
{
  xbt_assert(!xbt_swag_size(&(cnst->active_element_set)),"Removing constraint but it still has active elements");
  xbt_assert(!xbt_swag_size(&(cnst->enabled_element_set)),"Removing constraint but it still has enabled elements");
  xbt_assert(!xbt_swag_size(&(cnst->disabled_element_set)),"Removing constraint but it still has disabled elements");
  remove_constraint(sys, cnst);
  lmm_cnst_free(sys, cnst);
}

static void *lmm_variable_mallocator_new_f(void)
{
  lmm_variable_t var = xbt_new(s_lmm_variable_t, 1);
  var->cnsts = NULL; /* will be created by realloc */
  return var;
}

static void lmm_variable_mallocator_free_f(void *var)
{
  xbt_free(((lmm_variable_t) var)->cnsts);
  xbt_free(var);
}

lmm_variable_t lmm_variable_new(lmm_system_t sys, void *id,
                                double weight,
                                double bound, int number_of_constraints)
{
  lmm_variable_t var = NULL;
  int i;

  XBT_IN("(sys=%p, id=%p, weight=%f, bound=%f, num_cons =%d)",
          sys, id, weight, bound, number_of_constraints);

  var = (lmm_variable_t) xbt_mallocator_get(sys->variable_mallocator);
  var->id = id;
  var->id_int = Global_debug_id++;
  var->cnsts = (s_lmm_element_t *) xbt_realloc(var->cnsts, number_of_constraints * sizeof(s_lmm_element_t));
  for (i = 0; i < number_of_constraints; i++) {
    var->cnsts[i].enabled_element_set_hookup.next = NULL;
    var->cnsts[i].enabled_element_set_hookup.prev = NULL;
    var->cnsts[i].disabled_element_set_hookup.next = NULL;
    var->cnsts[i].disabled_element_set_hookup.prev = NULL;
    var->cnsts[i].active_element_set_hookup.next = NULL;
    var->cnsts[i].active_element_set_hookup.prev = NULL;
    var->cnsts[i].constraint = NULL;
    var->cnsts[i].variable = NULL;
    var->cnsts[i].value = 0.0;
  }
  var->cnsts_size = number_of_constraints;
  var->cnsts_number = 0;
  var->weight = weight;
  var->staged_weight = 0.0;
  var->bound = bound;
  var->concurrency_share = 1;
  var->value = 0.0;
  var->visited = sys->visited_counter - 1;
  var->mu = 0.0;
  var->new_mu = 0.0;
  var->func_f = func_f_def;
  var->func_fp = func_fp_def;
  var->func_fpi = func_fpi_def;

  var->variable_set_hookup.next = NULL;
  var->variable_set_hookup.prev = NULL;
  var->saturated_variable_set_hookup.next = NULL;
  var->saturated_variable_set_hookup.prev = NULL;

  if (weight)
    xbt_swag_insert_at_head(var, &(sys->variable_set));
  else
    xbt_swag_insert_at_tail(var, &(sys->variable_set));

  XBT_OUT(" returns %p", var);
  return var;
}

void lmm_variable_free(lmm_system_t sys, lmm_variable_t var)
{
  remove_variable(sys, var);
  lmm_var_free(sys, var);
}

double lmm_variable_getvalue(lmm_variable_t var)
{
  return (var->value);
}


void lmm_variable_concurrency_share_set(lmm_variable_t var, short int concurrency_share)
{
  var->concurrency_share=concurrency_share;
}

double lmm_variable_getbound(lmm_variable_t var)
{
  return (var->bound);
}

void lmm_shrink(lmm_system_t sys, lmm_constraint_t cnst,
                lmm_variable_t var)
{
  lmm_element_t elem = NULL;
  int found = 0;

  int i;
  for (i = 0; i < var->cnsts_number; i++) {
    elem = &(var->cnsts[i]);
    if (elem->constraint == cnst) {
      found = 1;
      break;
    }
  }

  if (!found) {
    XBT_DEBUG("cnst %p is not found in var %p", cnst, var);
    return;
  }

  sys->modified = 1;

  XBT_DEBUG("remove elem(value %f, cnst %p, var %p) in var %p",
      elem->value, elem->constraint, elem->variable, var);

  /* We are going to change the constraint object and the variable object.
   * Propagate this change to other objects. Calling here before removing variable from not active elements (inactive elements are not visited)
   */
  lmm_update_modified_set(sys, cnst);
  //Useful in case var was already removed from the constraint
  lmm_update_modified_set(sys, var->cnsts[0].constraint); // will look up enabled_element_set of this constraint, and then each var in the enabled_element_set, and each var->cnsts[i].

  if(xbt_swag_remove(elem, &(elem->constraint->enabled_element_set)))
    lmm_decrease_concurrency(elem);

  xbt_swag_remove(elem, &(elem->constraint->active_element_set));
  elem->constraint = NULL;
  elem->variable = NULL;
  elem->value = 0;

  var->cnsts_number -= 1;

  //No variable in this constraint -> make it inactive
  if (xbt_swag_size(&(cnst->enabled_element_set))+xbt_swag_size(&(cnst->disabled_element_set)) == 0)
    make_constraint_inactive(sys, cnst);
  else {
    //Check maxconcurrency to see if we can enable new variables
    lmm_on_disabled_var(sys,elem->constraint);       
  }

  lmm_check_concurrency(sys);
}

void lmm_expand(lmm_system_t sys, lmm_constraint_t cnst,
                lmm_variable_t var, double value)
{
  lmm_element_t elem = NULL;
  double weight;
  int i,current_share;
  
  
  sys->modified = 1;

  //Check if this variable already has an active element in this constraint
  //If it does, substract it from the required slack
  current_share=0;
  if(var->concurrency_share>1){
    for( i=0; i<var->cnsts_number;i++){
      if(var->cnsts[i].constraint==cnst && xbt_swag_belongs(&var->cnsts[i],&(var->cnsts[i].constraint->enabled_element_set)))
	current_share+=lmm_element_concurrency(&(var->cnsts[i]));
    }
  }

  //Check if we need to disable the variable 
  if(var->weight>0 && var->concurrency_share-current_share>lmm_concurrency_slack(cnst))
   {
    weight=var->weight;
    lmm_disable_var(sys,var);
    for (i = 0; i < var->cnsts_number; i++)
      lmm_on_disabled_var(sys,var->cnsts[i].constraint);
    value=0;
    var->staged_weight=weight;
    xbt_assert(!var->weight);
  }

  xbt_assert(var->cnsts_number < var->cnsts_size, "Too much constraints");

  elem = &(var->cnsts[var->cnsts_number++]);

  elem->value = value;
  elem->constraint = cnst;
  elem->variable = var;

  
  if (var->weight){
    xbt_swag_insert_at_head(elem, &(elem->constraint->enabled_element_set));
    lmm_increase_concurrency(elem);
  }
  else
    xbt_swag_insert_at_tail(elem, &(elem->constraint->disabled_element_set));

  if(!sys->selective_update_active) {
    make_constraint_active(sys, cnst);
  } else if(elem->value>0 || var->weight >0) {
    make_constraint_active(sys, cnst);
    lmm_update_modified_set(sys, cnst);
    //TODOLATER: Why do we need this second call?
    if (var->cnsts_number > 1)
      lmm_update_modified_set(sys, var->cnsts[0].constraint);
  }

  lmm_check_concurrency(sys);
}

void lmm_expand_add(lmm_system_t sys, lmm_constraint_t cnst,
                    lmm_variable_t var, double value)
{
  int i,j;
  double weight;
  sys->modified = 1;
  
  lmm_check_concurrency(sys);

  //BEWARE: In case you have multiple elements in one constraint, this will always
  //add value to the first element.
  for (i = 0; i < var->cnsts_number; i++)
    if (var->cnsts[i].constraint == cnst)
      break;

  if (i < var->cnsts_number) {
    if (var->weight)
      lmm_decrease_concurrency(&var->cnsts[i]);

    if (cnst->sharing_policy)
      var->cnsts[i].value += value;
    else
      var->cnsts[i].value = MAX(var->cnsts[i].value, value);

    //We need to check that increasing value of the element does not cross the concurrency limit
    if (var->weight){
      if(lmm_concurrency_slack(cnst)<lmm_element_concurrency(&var->cnsts[i])){
	weight=var->weight;
	lmm_disable_var(sys,var);
	for (j = 0; j < var->cnsts_number; j++)
	  lmm_on_disabled_var(sys,var->cnsts[j].constraint);
	var->staged_weight=weight;
	xbt_assert(!var->weight);
      }
      lmm_increase_concurrency(&var->cnsts[i]);
    }
    
    lmm_update_modified_set(sys, cnst);
  } else
    lmm_expand(sys, cnst, var, value);

  lmm_check_concurrency(sys);
}

lmm_constraint_t lmm_get_cnst_from_var(lmm_system_t /*sys*/,
                                                  lmm_variable_t var,
                                                  int num)
{
  if (num < var->cnsts_number)
    return (var->cnsts[num].constraint);
  else
    return NULL;
}

double lmm_get_cnst_weight_from_var(lmm_system_t /*sys*/,
                                                         lmm_variable_t var,
                                                         int num)
{
  if (num < var->cnsts_number)
    return (var->cnsts[num].value);
  else
    return 0.0;
}

int lmm_get_number_of_cnst_from_var(lmm_system_t /*sys*/,
                                               lmm_variable_t var)
{
  return (var->cnsts_number);
}

lmm_variable_t lmm_get_var_from_cnst(lmm_system_t /*sys*/,
                                     lmm_constraint_t cnst,
                                     lmm_element_t * elem)
{
  
  if (!(*elem)) {
    //That is the first call, pick the first element among enabled_element_set (or disabled_element_set if enabled_element_set is empty)
    *elem = (lmm_element_t) xbt_swag_getFirst(&(cnst->enabled_element_set));
    if (!(*elem))
      *elem = (lmm_element_t) xbt_swag_getFirst(&(cnst->disabled_element_set));
  }  else {
    //elem is not null, so we carry on
    if(xbt_swag_belongs(*elem,&(cnst->enabled_element_set))){
      //Look at enabled_element_set, and jump to disabled_element_set when finished
      *elem = (lmm_element_t) xbt_swag_getNext(*elem, cnst->enabled_element_set.offset);
      if (!(*elem))
  *elem = (lmm_element_t) xbt_swag_getFirst(&(cnst->disabled_element_set));
    } else {
      *elem = (lmm_element_t) xbt_swag_getNext(*elem, cnst->disabled_element_set.offset);      
    }
  }
  if (*elem)
    return (*elem)->variable;
  else
    return NULL;
}

//if we modify the swag between calls, normal version may loop forever
//this safe version ensures that we browse the swag elements only once
lmm_variable_t lmm_get_var_from_cnst_safe(lmm_system_t /*sys*/,
                                     lmm_constraint_t cnst,
                                     lmm_element_t * elem,
                                     lmm_element_t * nextelem,
                                     int * numelem)
{
  if (!(*elem)){
    *elem = (lmm_element_t) xbt_swag_getFirst(&(cnst->enabled_element_set));
    *numelem = xbt_swag_size(&(cnst->enabled_element_set))+xbt_swag_size(&(cnst->disabled_element_set))-1;
    if (!(*elem))
      *elem = (lmm_element_t) xbt_swag_getFirst(&(cnst->disabled_element_set));
  }else{
    *elem = *nextelem;
    if(*numelem>0){
     (*numelem) --;
    }else
      return NULL;
  }
  if (*elem){
    //elem is not null, so we carry on
    if(xbt_swag_belongs(*elem,&(cnst->enabled_element_set))){
      //Look at enabled_element_set, and jump to disabled_element_set when finished
      *nextelem = (lmm_element_t) xbt_swag_getNext(*elem, cnst->enabled_element_set.offset);
      if (!(*nextelem))
  *nextelem = (lmm_element_t) xbt_swag_getFirst(&(cnst->disabled_element_set));
    } else {
      *nextelem = (lmm_element_t) xbt_swag_getNext(*elem, cnst->disabled_element_set.offset);      
    }
    return (*elem)->variable;
  }else
    return NULL;
}

void *lmm_constraint_id(lmm_constraint_t cnst)
{
  return cnst->id;
}

void *lmm_variable_id(lmm_variable_t var)
{
  return var->id;
}

static inline void saturated_constraint_set_update(double usage,
                                                      int cnst_light_num,
                                                      dyn_light_t saturated_constraint_set,
                                                      double *min_usage)
{
  xbt_assert(usage > 0,"Impossible");

  if (*min_usage < 0 || *min_usage > usage) {
    *min_usage = usage;
    XBT_HERE(" min_usage=%f (cnst->remaining / cnst->usage =%f)", *min_usage, usage);
    saturated_constraint_set->data[0] = cnst_light_num;
    saturated_constraint_set->pos = 1;
  } else if (*min_usage == usage) {
    if(saturated_constraint_set->pos == saturated_constraint_set->size) { // realloc the size
      saturated_constraint_set->size *= 2;
      saturated_constraint_set->data = (int*) xbt_realloc(saturated_constraint_set->data, (saturated_constraint_set->size) * sizeof(int));
    }
    saturated_constraint_set->data[saturated_constraint_set->pos] = cnst_light_num;
    saturated_constraint_set->pos++;
  }
}

static inline void saturated_variable_set_update(
    s_lmm_constraint_light_t *cnst_light_tab,
    dyn_light_t saturated_constraint_set,
    lmm_system_t sys)
{
  /* Add active variables (i.e. variables that need to be set) from the set of constraints to saturate (cnst_light_tab)*/ 
  lmm_constraint_light_t cnst = NULL;
  void *_elem;
  lmm_element_t elem = NULL;
  xbt_swag_t elem_list = NULL;
  int i;
  for(i = 0; i< saturated_constraint_set->pos; i++){
    cnst = &cnst_light_tab[saturated_constraint_set->data[i]];
    elem_list = &(cnst->cnst->active_element_set);
    xbt_swag_foreach(_elem, elem_list) {
      elem = (lmm_element_t)_elem;
      //Visiting active_element_set, so, by construction, should never get a zero weight, correct?
      xbt_assert(elem->variable->weight > 0);
      if ((elem->value > 0))
        xbt_swag_insert(elem->variable, &(sys->saturated_variable_set));
    }
  }
}

void lmm_print(lmm_system_t sys)
{
  void *_cnst, *_elem, *_var;
  lmm_constraint_t cnst = NULL;
  lmm_element_t elem = NULL;
  lmm_variable_t var = NULL;
  xbt_swag_t cnst_list = NULL;
  xbt_swag_t var_list = NULL;
  xbt_swag_t elem_list = NULL;
  char print_buf[1024];
  char *trace_buf = (char*) xbt_malloc0(sizeof(char));
  double sum = 0.0;

  /* Printing Objective */
  var_list = &(sys->variable_set);
  sprintf(print_buf, "MAX-MIN ( ");
  trace_buf = (char*)
      xbt_realloc(trace_buf, strlen(trace_buf) + strlen(print_buf) + 1);
  strcat(trace_buf, print_buf);
  xbt_swag_foreach(_var, var_list) {
  var = (lmm_variable_t)_var;
    sprintf(print_buf, "'%d'(%f) ", var->id_int, var->weight);
    trace_buf = (char*)
        xbt_realloc(trace_buf, strlen(trace_buf) + strlen(print_buf) + 1);
    strcat(trace_buf, print_buf);
  }
  sprintf(print_buf, ")");
  trace_buf = (char*)
      xbt_realloc(trace_buf, strlen(trace_buf) + strlen(print_buf) + 1);
  strcat(trace_buf, print_buf);
  XBT_DEBUG("%20s", trace_buf);
  trace_buf[0] = '\000';

  XBT_DEBUG("Constraints");
  /* Printing Constraints */
  cnst_list = &(sys->active_constraint_set);
  xbt_swag_foreach(_cnst, cnst_list) {
  cnst = (lmm_constraint_t)_cnst;
    sum = 0.0;
    //Show  the enabled variables
    elem_list = &(cnst->enabled_element_set);
    sprintf(print_buf, "\t");
    trace_buf = (char*)
        xbt_realloc(trace_buf, strlen(trace_buf) + strlen(print_buf) + 1);
    strcat(trace_buf, print_buf);
    sprintf(print_buf, "%s(",(cnst->sharing_policy)?"":"max");
    trace_buf = (char*)
      xbt_realloc(trace_buf,
      strlen(trace_buf) + strlen(print_buf) + 1);
    strcat(trace_buf, print_buf);      
    xbt_swag_foreach(_elem, elem_list) {
      elem = (lmm_element_t)_elem;
      sprintf(print_buf, "%f.'%d'(%f) %s ", elem->value,
              elem->variable->id_int, elem->variable->value,(cnst->sharing_policy)?"+":",");
      trace_buf = (char*)
          xbt_realloc(trace_buf,
                      strlen(trace_buf) + strlen(print_buf) + 1);
      strcat(trace_buf, print_buf);
      if(cnst->sharing_policy)
        sum += elem->value * elem->variable->value;
      else 
        sum = MAX(sum,elem->value * elem->variable->value);
    }
    //TODO: Adding disabled elements only for test compatibility, but do we really want them to be printed?
    elem_list = &(cnst->disabled_element_set);
    xbt_swag_foreach(_elem, elem_list) {
      elem = (lmm_element_t)_elem;
      sprintf(print_buf, "%f.'%d'(%f) %s ", elem->value,
              elem->variable->id_int, elem->variable->value,(cnst->sharing_policy)?"+":",");
      trace_buf = (char*)
          xbt_realloc(trace_buf,
                      strlen(trace_buf) + strlen(print_buf) + 1);
      strcat(trace_buf, print_buf);
      if(cnst->sharing_policy)
        sum += elem->value * elem->variable->value;
      else 
        sum = MAX(sum,elem->value * elem->variable->value);
    }
    
    sprintf(print_buf, "0) <= %f ('%d')", cnst->bound, cnst->id_int);
    trace_buf = (char*)
        xbt_realloc(trace_buf, strlen(trace_buf) + strlen(print_buf) + 1);
    strcat(trace_buf, print_buf);

    if (!cnst->sharing_policy) {
      sprintf(print_buf, " [MAX-Constraint]");
      trace_buf = (char*)
          xbt_realloc(trace_buf,
                      strlen(trace_buf) + strlen(print_buf) + 1);
      strcat(trace_buf, print_buf);
    }
    XBT_DEBUG("%s", trace_buf);
    trace_buf[0] = '\000';
       xbt_assert(!double_positive(sum - cnst->bound, cnst->bound*sg_maxmin_precision),
               "Incorrect value (%f is not smaller than %f): %g",
                 sum, cnst->bound, sum - cnst->bound);
       //if(double_positive(sum - cnst->bound, cnst->bound*sg_maxmin_precision))
       //XBT_ERROR("Incorrect value (%f is not smaller than %f): %g",sum, cnst->bound, sum - cnst->bound);
      
  }

  XBT_DEBUG("Variables");
  /* Printing Result */
  xbt_swag_foreach(_var, var_list) {
  var = (lmm_variable_t)_var;
    if (var->bound > 0) {
      XBT_DEBUG("'%d'(%f) : %f (<=%f)", var->id_int, var->weight, var->value,
             var->bound);
      xbt_assert(!double_positive(var->value - var->bound, var->bound*sg_maxmin_precision),
                  "Incorrect value (%f is not smaller than %f",
                  var->value, var->bound);
    } else {
      XBT_DEBUG("'%d'(%f) : %f", var->id_int, var->weight, var->value);
    }
  }

  free(trace_buf);
}

void lmm_solve(lmm_system_t sys)
{
  void *_var, *_cnst, *_cnst_next, *_elem;
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

  XBT_IN("(sys=%p)", sys);

  /*
   * Compute Usage and store the variables that reach the maximum. If selective_update_active is true, only constraints that changed are considered. Otherwise all constraints with active actions are considered.
   */
  cnst_list =
      sys->
      selective_update_active ? &(sys->modified_constraint_set) :
      &(sys->active_constraint_set);

  XBT_DEBUG("Active constraints : %d", xbt_swag_size(cnst_list));
  /* Init: Only modified code portions: reset the value of active variables */
  xbt_swag_foreach(_cnst, cnst_list) {
  cnst = (lmm_constraint_t)_cnst;
    elem_list = &(cnst->enabled_element_set);
    //XBT_DEBUG("Variable set : %d", xbt_swag_size(elem_list));
    xbt_swag_foreach(_elem, elem_list) {
      var = ((lmm_element_t)_elem)->variable;
      xbt_assert(var->weight > 0.0);
      var->value = 0.0;
    }
  }

  s_lmm_constraint_light_t *cnst_light_tab = (s_lmm_constraint_light_t *)xbt_malloc0(xbt_swag_size(cnst_list)*sizeof(s_lmm_constraint_light_t));
  int cnst_light_num = 0;
  dyn_light_t saturated_constraint_set = xbt_new0(s_dyn_light_t,1);
  saturated_constraint_set->size = 5;
  saturated_constraint_set->data = xbt_new0(int, saturated_constraint_set->size);

  xbt_swag_foreach_safe(_cnst, _cnst_next, cnst_list) {
  cnst = (lmm_constraint_t)_cnst;
    /* INIT: Collect constraints that actually need to be saturated (i.e remaining  and usage are strictly positive) into cnst_light_tab. */
    cnst->remaining = cnst->bound;
    if (!double_positive(cnst->remaining, cnst->bound*sg_maxmin_precision))
      continue;
    cnst->usage = 0;
    elem_list = &(cnst->enabled_element_set);
    xbt_swag_foreach(_elem, elem_list) {
      elem = (lmm_element_t)_elem;
      xbt_assert(elem->variable->weight > 0);
      if ((elem->value > 0)) {
        if (cnst->sharing_policy)
          cnst->usage += elem->value / elem->variable->weight;
        else if (cnst->usage < elem->value / elem->variable->weight)
          cnst->usage = elem->value / elem->variable->weight;

        make_elem_active(elem);
        simgrid::surf::Action *action = static_cast<simgrid::surf::Action*>(elem->variable->id);
        if (sys->keep_track && !action->is_linked())
          sys->keep_track->push_back(*action);
      }
    }
    XBT_DEBUG("Constraint '%d' usage: %f remaining: %f concurrency: %i<=%i<=%i", cnst->id_int, cnst->usage, cnst->remaining,cnst->concurrency_current,cnst->concurrency_maximum,cnst->concurrency_limit);
    /* Saturated constraints update */

    if(cnst->usage > 0) {
      cnst_light_tab[cnst_light_num].cnst = cnst;
      cnst->cnst_light = &(cnst_light_tab[cnst_light_num]);
      cnst_light_tab[cnst_light_num].remaining_over_usage = cnst->remaining / cnst->usage;
      saturated_constraint_set_update(cnst_light_tab[cnst_light_num].remaining_over_usage,
        cnst_light_num, saturated_constraint_set, &min_usage);
      xbt_assert(cnst->active_element_set.count>0, "There is no sense adding a constraint that has no active element!" );
      cnst_light_num++;
    }
  }

  saturated_variable_set_update(  cnst_light_tab,
                                  saturated_constraint_set,
                                  sys);

  /* Saturated variables update */

  do {
    /* Fix the variables that have to be */
    var_list = &(sys->saturated_variable_set);

    xbt_swag_foreach(_var, var_list) {
      var = (lmm_variable_t)_var;
      if (var->weight <= 0.0)
        DIE_IMPOSSIBLE;
      /* First check if some of these variables could reach their upper
         bound and update min_bound accordingly. */
      XBT_DEBUG
          ("var=%d, var->bound=%f, var->weight=%f, min_usage=%f, var->bound*var->weight=%f",
           var->id_int, var->bound, var->weight, min_usage,
           var->bound * var->weight);
      if ((var->bound > 0) && (var->bound * var->weight < min_usage)) {
        if (min_bound < 0)
          min_bound = var->bound*var->weight;
        else
          min_bound = MIN(min_bound, (var->bound*var->weight));
        XBT_DEBUG("Updated min_bound=%f", min_bound);
      }
    }


    while ((var = (lmm_variable_t)xbt_swag_getFirst(var_list))) {
      int i;

      if (min_bound < 0) {
  //If no variable could reach its bound, deal iteratively the constraints usage ( at worst one constraint is saturated at each cycle) 
        var->value = min_usage / var->weight;
	// XBT_DEBUG("Setting %p (%d) value to %f\n", var, var->id_int, var->value);
        XBT_DEBUG("Setting var (%d) value to %f\n", var->id_int, var->value);
      } else {
  //If there exist a variable that can reach its bound, only update it (and other with the same bound) for now.
      if (double_equals(min_bound, var->bound*var->weight, sg_maxmin_precision)){
          var->value = var->bound;
          XBT_DEBUG("Setting %p (%d) value to %f\n", var, var->id_int, var->value);
        }
        else {
    // Variables which bound is different are not considered for this cycle, but they will be afterwards.  
          XBT_DEBUG("Do not consider %p (%d) \n", var, var->id_int);
          xbt_swag_remove(var, var_list);
          continue;
        }
      }
      XBT_DEBUG("Min usage: %f, Var(%d)->weight: %f, Var(%d)->value: %f ",
             min_usage, var->id_int, var->weight, var->id_int, var->value);


      /* Update the usage of contraints where this variable is involved */
      for (i = 0; i < var->cnsts_number; i++) {
        elem = &var->cnsts[i];
        cnst = elem->constraint;
        if (cnst->sharing_policy) {
    //Remember: shared constraints require that sum(elem->value * var->value) < cnst->bound
          double_update(&(cnst->remaining),  elem->value * var->value, cnst->bound*sg_maxmin_precision);
          double_update(&(cnst->usage), elem->value / var->weight, sg_maxmin_precision);
    //If the constraint is saturated, remove it from the set of active constraints (light_tab)
          if(!double_positive(cnst->usage,sg_maxmin_precision) || !double_positive(cnst->remaining,cnst->bound*sg_maxmin_precision)) {
            if (cnst->cnst_light) {
              int index = (cnst->cnst_light-cnst_light_tab);
              XBT_DEBUG("index: %d \t cnst_light_num: %d \t || usage: %f remaining: %f bound: %f  ",
	      index,cnst_light_num, cnst->usage, cnst->remaining, cnst->bound);
	      //              XBT_DEBUG("index: %d \t cnst_light_num: %d \t || \t cnst: %p \t cnst->cnst_light: %p \t cnst_light_tab: %p usage: %f remaining: %f bound: %f  ",
	      //index,cnst_light_num, cnst, cnst->cnst_light, cnst_light_tab, cnst->usage, cnst->remaining, cnst->bound);
              cnst_light_tab[index]=cnst_light_tab[cnst_light_num-1];
              cnst_light_tab[index].cnst->cnst_light = &cnst_light_tab[index];
              cnst_light_num--;
              cnst->cnst_light = NULL;
            }
          } else {
            cnst->cnst_light->remaining_over_usage = cnst->remaining / cnst->usage;
          }
          make_elem_inactive(elem);
        } else {
    //Remember: non-shared constraints only require that max(elem->value * var->value) < cnst->bound
          cnst->usage = 0.0;
          make_elem_inactive(elem);
          elem_list = &(cnst->enabled_element_set);
          xbt_swag_foreach(_elem, elem_list) {
      elem = (lmm_element_t)_elem;
      xbt_assert(elem->variable->weight > 0);
      if (elem->variable->value > 0) continue;
      if (elem->value > 0)
        cnst->usage = MAX(cnst->usage, elem->value / elem->variable->weight);
          }
    //If the constraint is saturated, remove it from the set of active constraints (light_tab)
          if(!double_positive(cnst->usage,sg_maxmin_precision) || !double_positive(cnst->remaining,cnst->bound*sg_maxmin_precision)) {
            if(cnst->cnst_light) {
              int index = (cnst->cnst_light-cnst_light_tab);
              XBT_DEBUG("index: %d \t cnst_light_num: %d \t || \t cnst: %p \t cnst->cnst_light: %p \t cnst_light_tab: %p usage: %f remaining: %f bound: %f  ",
      index,cnst_light_num, cnst, cnst->cnst_light, cnst_light_tab, cnst->usage, cnst->remaining, cnst->bound);
              cnst_light_tab[index]=cnst_light_tab[cnst_light_num-1];
              cnst_light_tab[index].cnst->cnst_light = &cnst_light_tab[index];
              cnst_light_num--;
              cnst->cnst_light = NULL;
            }
          } else {
            cnst->cnst_light->remaining_over_usage = cnst->remaining / cnst->usage;
            xbt_assert(cnst->active_element_set.count>0, "Should not keep a maximum constraint that has no active element! You want to check the maxmin precision and possible rounding effects." );
          }
        }
      }
      xbt_swag_remove(var, var_list);
    }

    /* Find out which variables reach the maximum */
    min_usage = -1;
    min_bound = -1;
    saturated_constraint_set->pos = 0;
    int pos;
    for(pos=0; pos<cnst_light_num; pos++){
      xbt_assert(cnst_light_tab[pos].cnst->active_element_set.count>0, "Cannot saturate more a constraint that has no active element! You may want to change the maxmin precision (--cfg=maxmin/precision:<new_value>) because of possible rounding effects.\n\tFor the record, the usage of this constraint is %g while the maxmin precision to which it is compared is %g.\n\tThe usage of the previous constraint is %g.", cnst_light_tab[pos].cnst->usage, sg_maxmin_precision, cnst_light_tab[pos-1].cnst->usage);
      saturated_constraint_set_update(
          cnst_light_tab[pos].remaining_over_usage,
          pos,
          saturated_constraint_set,
          &min_usage);
  }

    saturated_variable_set_update(  cnst_light_tab,
                                    saturated_constraint_set,
                                    sys);

  } while (cnst_light_num > 0);

  sys->modified = 0;
  if (sys->selective_update_active)
    lmm_remove_all_modified_set(sys);

  if (XBT_LOG_ISENABLED(surf_maxmin, xbt_log_priority_debug)) {
    lmm_print(sys);
  }

  lmm_check_concurrency(sys);
  
  xbt_free(saturated_constraint_set->data);
  xbt_free(saturated_constraint_set);
  xbt_free(cnst_light_tab);
  XBT_OUT();
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
  sys->modified = 1;
  var->bound = bound;

  if (var->cnsts_number)
    lmm_update_modified_set(sys, var->cnsts[0].constraint);
}



int lmm_concurrency_slack(lmm_constraint_t cnstr){

  //FIXME MARTIN: Replace by infinite value std::numeric_limits<int>::(max)(), or something better within Simgrid?
  if(cnstr->concurrency_limit<0)
    return 666;

  return  cnstr->concurrency_limit - cnstr->concurrency_current;  
}

/** \brief Measure the minimum concurrency slack across all constraints where var is involved
 *
 * \param The variable to check for
 *
 */
int lmm_cnstrs_min_concurrency_slack(lmm_variable_t var){
  int i;
  //FIXME MARTIN: Replace by infinite value std::numeric_limits<int>::(max)(), or something better within Simgrid?
  int slack,minslack=666;
  for (i = 0; i < var->cnsts_number; i++) {
    slack=lmm_concurrency_slack(var->cnsts[i].constraint);
    
    //This is only an optimization, to avoid looking at more constraints when slack is already zero
    //Disable it when debugging to let lmm_concurrency_slack catch nasty things
    if(!slack   && !XBT_LOG_ISENABLED(surf_maxmin, xbt_log_priority_debug))
      return 0;

    if(minslack>slack)
      minslack=slack;
  }

  return minslack;
}

/* /Check if a variable can be enabled
 *
 * Make sure to set staged_weight before, if your intent is only to check concurrency 
 */
int lmm_can_enable_var(lmm_variable_t var){
  return var->staged_weight>0 && lmm_cnstrs_min_concurrency_slack(var)>=var->concurrency_share;
}


//Small remark: In this implementation of lmm_enable_var and lmm_disable_var, we will meet multiple times with var when running lmm_update_modified_set.
//A priori not a big performance issue, but we might do better by calling lmm_update_modified_set within the for loops (after doing the first for enabling==1, and before doing the last for disabling==1)

void lmm_enable_var(lmm_system_t sys, lmm_variable_t var){

  int i;
  lmm_element_t elem;
  
  xbt_assert(lmm_can_enable_var(var));

  var->weight = var->staged_weight;
  var->staged_weight = 0;

  //Enabling the variable, move to var to list head. Subtility is: here, we need to call lmm_update_modified_set AFTER moving at least one element of var.

  xbt_swag_remove(var, &(sys->variable_set));
  xbt_swag_insert_at_head(var, &(sys->variable_set));
  for (i = 0; i < var->cnsts_number; i++) {
    elem = &var->cnsts[i];
    xbt_swag_remove(elem, &(elem->constraint->disabled_element_set));
    xbt_swag_insert_at_head(elem, &(elem->constraint->enabled_element_set));
    lmm_increase_concurrency(elem);
  }
  if (var->cnsts_number)
    lmm_update_modified_set(sys, var->cnsts[0].constraint);

  //When used within lmm_on_disabled_var, we would get an assertion fail, because transiently there can be variables that are staged and could be activated.
  //Anyway, caller functions all call lmm_check_concurrency() in the end. 
  //  lmm_check_concurrency(sys);
}

void lmm_disable_var(lmm_system_t sys, lmm_variable_t var){
  int i;
  lmm_element_t elem;

  xbt_assert(!var->staged_weight,"Staged weight should have been cleared");
  //Disabling the variable, move to var to list tail. Subtility is: here, we need to call lmm_update_modified_set BEFORE moving the last element of var.
  xbt_swag_remove(var, &(sys->variable_set));
  xbt_swag_insert_at_tail(var, &(sys->variable_set));
  if (var->cnsts_number)
    lmm_update_modified_set(sys, var->cnsts[0].constraint);
  for (i = 0; i < var->cnsts_number; i++) {
    elem = &var->cnsts[i];
    xbt_swag_remove(elem, &(elem->constraint->enabled_element_set));
    xbt_swag_insert_at_tail(elem, &(elem->constraint->disabled_element_set));

    xbt_swag_remove(elem, &(elem->constraint->active_element_set));

    lmm_decrease_concurrency(elem);
  }

  var->weight=0.0;
  var->staged_weight=0.0;
  var->value = 0.0;
  lmm_check_concurrency(sys);
}
 
/* /brief Find variables that can be enabled and enable them.
 * 
 * Assuming that the variable has already been removed from non-zero weights
 * Can we find a staged variable to add?
 * If yes, check that none of the constraints that this variable is involved in is at the limit of its concurrency
 * And then add it to enabled variables
 */
void lmm_on_disabled_var(lmm_system_t sys, lmm_constraint_t cnstr){

  lmm_element_t elem;
  lmm_element_t nextelem;
  int numelem;
  
  if(cnstr->concurrency_limit<0)
    return;

  numelem=xbt_swag_size(&(cnstr->disabled_element_set));
  if(!numelem)
    return;

  elem= (lmm_element_t) xbt_swag_getFirst(&(cnstr->disabled_element_set));

  //Cannot use xbt_swag_foreach, because lmm_enable_var will modify disabled_element_set.. within the loop
  while(numelem-- && elem ){

    nextelem = (lmm_element_t) xbt_swag_getNext(elem, cnstr->disabled_element_set.offset);      

    if (elem->variable->staged_weight>0 )
      {
	//Found a staged variable
	//TODOLATER: Add random timing function to model reservation protocol fuzziness? Then how to make sure that staged variables will eventually be called?
	if(lmm_can_enable_var(elem->variable)){
	  lmm_enable_var(sys,elem->variable);
	}        
      }

    xbt_assert(cnstr->concurrency_current<=cnstr->concurrency_limit,"Concurrency overflow!");
    if(cnstr->concurrency_current==cnstr->concurrency_limit)
      break;

    elem = nextelem;
  }

  //We could get an assertion fail, because transiently there can be variables that are staged and could be activated.
  //And we need to go through all constraints of the disabled var before getting back a coherent state.
  //Anyway, caller functions all call lmm_check_concurrency() in the end. 
  //  lmm_check_concurrency(sys);

}

/* \brief update the weight of a variable, and enable/disable it.
 * @return Returns whether a change was made
 */
void lmm_update_variable_weight(lmm_system_t sys, lmm_variable_t var,
             double weight)
{
  int minslack;
  
  xbt_assert(weight>=0,"Variable weight should not be negative!");
  
  if (weight == var->weight)
    return;

  int enabling_var=  (weight>0 && var->weight<=0);
  int disabling_var= (weight<=0 && var->weight>0);
 
  XBT_IN("(sys=%p, var=%p, weight=%f)", sys, var, weight);

  sys->modified = 1;
  
  //Are we enabling this variable?
  if (enabling_var){
    var->staged_weight = weight;
    minslack=lmm_cnstrs_min_concurrency_slack(var);
    if(minslack<var->concurrency_share){      
      XBT_DEBUG("Staging var (instead of enabling) because min concurrency slack %i, with weight %f and concurrency share %i", minslack, weight, var->concurrency_share);
      return;
    }
    XBT_DEBUG("Enabling var with min concurrency slack %i", minslack);
    lmm_enable_var(sys,var);   
  } else if (disabling_var){
    //Are we disabling this variable?
    lmm_disable_var(sys,var);       
  } else {
    var->weight=weight;
  }

  lmm_check_concurrency(sys);
  
  XBT_OUT();
  return;
}

double lmm_get_variable_weight(lmm_variable_t var)
{
  return var->weight;
}

void lmm_update_constraint_bound(lmm_system_t sys,
                                            lmm_constraint_t cnst,
                                            double bound)
{
  sys->modified = 1;
  lmm_update_modified_set(sys, cnst);
  cnst->bound = bound;
}

int lmm_constraint_used(lmm_system_t sys, lmm_constraint_t cnst)
{
  return xbt_swag_belongs(cnst, &(sys->active_constraint_set));
}

inline lmm_constraint_t lmm_get_first_active_constraint(lmm_system_t
                                                            sys)
{
  return (lmm_constraint_t)xbt_swag_getFirst(&(sys->active_constraint_set));
}

inline lmm_constraint_t lmm_get_next_active_constraint(lmm_system_t
                                                           sys,
                                                           lmm_constraint_t
                                                           cnst)
{
  return (lmm_constraint_t)xbt_swag_getNext(cnst, (sys->active_constraint_set).offset);
}

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
static void lmm_update_modified_set_rec(lmm_system_t sys,
                                        lmm_constraint_t cnst)
{
  void* _elem;

  //TODOLATER: Why lmm_modified_set has been changed in git version 2392B5157...? Looks equivalent logically and less obvious..
  
  xbt_swag_foreach(_elem, &cnst->enabled_element_set) {
    lmm_variable_t var = ((lmm_element_t)_elem)->variable;
    s_lmm_element_t *cnsts = var->cnsts;
    int i;
    for (i = 0; var->visited != sys->visited_counter
             && i < var->cnsts_number ; i++) {
      if (cnsts[i].constraint != cnst
          && !xbt_swag_belongs(cnsts[i].constraint,
                               &sys->modified_constraint_set)) {
        xbt_swag_insert(cnsts[i].constraint, &sys->modified_constraint_set);
        lmm_update_modified_set_rec(sys, cnsts[i].constraint);
      }
    }
    //var will be ignored in later visits as long as sys->visited_counter does not move 
    var->visited = sys->visited_counter;
  }
}

static void lmm_update_modified_set(lmm_system_t sys,
                                    lmm_constraint_t cnst)
{
  /* nothing to do if selective update isn't active */
  if (sys->selective_update_active
      && !xbt_swag_belongs(cnst, &sys->modified_constraint_set)) {
    xbt_swag_insert(cnst, &sys->modified_constraint_set);
    lmm_update_modified_set_rec(sys, cnst);
  }
}

/** \brief Remove all constraints of the modified_constraint_set.
 *
 *  \param sys the lmm_system_t
 *
 */
static void lmm_remove_all_modified_set(lmm_system_t sys)
{

  //We cleverly un-flag all variables just by incrementing sys->visited_counter
  //In effect, the var->visited value will no more be equal to sys->visited counter
  //To be clean, when visited counter has wrapped around, we force these var->visited values so that variables that were in the modified a long (long long) time ago are not wrongly skipped here, which would lead to very nasty bugs (i.e. not readibily reproducible, and requiring a lot of run time before happening).  
  if (++sys->visited_counter == 1) {
    /* the counter wrapped around, reset each variable->visited */
  void *_var;
    xbt_swag_foreach(_var, &sys->variable_set)
      ((lmm_variable_t)_var)->visited = 0;
  } 
  xbt_swag_reset(&sys->modified_constraint_set);
}

/**
 *  Returns total resource load
 *
 * \param cnst the lmm_constraint_t associated to the resource
 *
 *
 * This is dead code, but we may use it later for debug/trace.
 */
double lmm_constraint_get_usage(lmm_constraint_t cnst) {
   double usage = 0.0;
   xbt_swag_t elem_list = &(cnst->enabled_element_set);
   void *_elem;
   lmm_element_t elem = NULL;

   xbt_swag_foreach(_elem, elem_list) {
   elem = (lmm_element_t)_elem;
     if ((elem->value > 0)) {
       if (cnst->sharing_policy)
         usage += elem->value * elem->variable->value;
       else if (usage < elem->value * elem->variable->value)
         usage = elem->value * elem->variable->value;
     }
   }
  return usage;
}

void lmm_check_concurrency(lmm_system_t sys){
  void* _cnst;
  void* _elem;
  void* _var;
  lmm_element_t elem;
  lmm_constraint_t cnst;
  lmm_variable_t var;
  int concurrency;
  int i,belong_to_enabled,belong_to_disabled,belong_to_active;
  
  //These checks are very expensive, so do them only if we want to debug SURF LMM
  if (XBT_LOG_ISENABLED(surf_maxmin, xbt_log_priority_debug)) {
  
    xbt_swag_foreach(_cnst, &(sys->constraint_set)) {
      cnst = (lmm_constraint_t) _cnst;
      concurrency=0;
      xbt_swag_foreach(_elem, &(cnst->enabled_element_set)) {
	elem = (lmm_element_t)_elem;
	xbt_assert(elem->variable->weight > 0);
	concurrency+=lmm_element_concurrency(elem);
      }
      
      xbt_swag_foreach(_elem, &(cnst->disabled_element_set)) {
  elem = (lmm_element_t)_elem;
  //We should have staged variables only if concurrency is reached in some constraint
  xbt_assert(cnst->concurrency_limit<0 || elem->variable->staged_weight==0 || lmm_cnstrs_min_concurrency_slack(elem->variable) < elem->variable->concurrency_share,"should not have staged variable!");
      }
      
      xbt_assert(cnst->concurrency_limit<0 || cnst->concurrency_limit >= concurrency,"concurrency check failed!");
      xbt_assert(cnst->concurrency_current == concurrency, "concurrency_current is out-of-date!");
    }


    //Check that for each variable, all corresponding elements are in the same state (i.e. same element sets)
    xbt_swag_foreach(_var, &(sys->variable_set)) {
      var= (lmm_variable_t) _var;

      if(!var->cnsts_number)
	continue;

      elem = &var->cnsts[0];
      belong_to_enabled=xbt_swag_belongs(elem,&(elem->constraint->enabled_element_set));
      belong_to_disabled=xbt_swag_belongs(elem,&(elem->constraint->disabled_element_set));
      belong_to_active=xbt_swag_belongs(elem,&(elem->constraint->active_element_set));

      for (i = 1; i < var->cnsts_number; i++) {
	elem = &var->cnsts[i];
	xbt_assert(belong_to_enabled==xbt_swag_belongs(elem,&(elem->constraint->enabled_element_set)),
		   "Variable inconsistency (1): enabled_element_set");
	xbt_assert(belong_to_disabled==xbt_swag_belongs(elem,&(elem->constraint->disabled_element_set)),
	   "Variable inconsistency (2): disabled_element_set");
	xbt_assert(belong_to_active==xbt_swag_belongs(elem,&(elem->constraint->active_element_set)),
		   "Variable inconsistency (3): active_element_set");
      }
      
    }
  }
}
