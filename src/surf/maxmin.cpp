/* Copyright (c) 2004-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* \file callbacks.h */

#include "maxmin_private.hpp"
#include "xbt/backtrace.hpp"
#include "xbt/log.h"
#include "xbt/mallocator.h"
#include "xbt/sysdep.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cxxabi.h>
#include <limits>
#include <vector>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_maxmin, surf, "Logging specific to SURF (maxmin)");

typedef std::vector<int> dyn_light_t;

double sg_maxmin_precision = 0.00001; /* Change this with --cfg=maxmin/precision:VALUE */
double sg_surf_precision   = 0.00001; /* Change this with --cfg=surf/precision:VALUE */
int sg_concurrency_limit   = -1;      /* Change this with --cfg=maxmin/concurrency-limit:VALUE */

static int Global_debug_id = 1;
static int Global_const_debug_id = 1;

static int lmm_can_enable_var(lmm_variable_t var);
static int lmm_concurrency_slack(lmm_constraint_t cnstr);
static int lmm_cnstrs_min_concurrency_slack(lmm_variable_t var);

static inline int lmm_element_concurrency(lmm_element_t elem)
{
  //Ignore element with weight less than one (e.g. cross-traffic)
  return (elem->consumption_weight >= 1) ? 1 : 0;
  //There are other alternatives, but they will change the behaviour of the model..
  //So do not use it unless you want to make a new model.
  //If you do, remember to change the variables concurrency share to reflect it.
  //Potential examples are:
  //return (elem->weight>0)?1:0;//Include element as soon  as weight is non-zero
  //return (int)ceil(elem->weight);//Include element as the rounded-up integer value of the element weight
}

static inline void lmm_decrease_concurrency(lmm_element_t elem)
{
  xbt_assert(elem->constraint->concurrency_current>=lmm_element_concurrency(elem));
  elem->constraint->concurrency_current-=lmm_element_concurrency(elem);
}

static inline void lmm_increase_concurrency(lmm_element_t elem)
{
  elem->constraint->concurrency_current+= lmm_element_concurrency(elem);

  lmm_constraint_t cnstr=elem->constraint;

  if(cnstr->concurrency_current > cnstr->concurrency_maximum)
    cnstr->concurrency_maximum= cnstr->concurrency_current;

  xbt_assert(cnstr->concurrency_limit<0 || cnstr->concurrency_current<=cnstr->concurrency_limit,
             "Concurrency limit overflow!");
}

void s_lmm_system_t::check_concurrency()
{
  // These checks are very expensive, so do them only if we want to debug SURF LMM
  if (not XBT_LOG_ISENABLED(surf_maxmin, xbt_log_priority_debug))
    return;

  void* cnstIt;
  xbt_swag_foreach(cnstIt, &constraint_set)
  {
    lmm_constraint_t cnst = (lmm_constraint_t)cnstIt;
    int concurrency       = 0;
    void* elemIt;
    xbt_swag_foreach(elemIt, &(cnst->enabled_element_set))
    {
      lmm_element_t elem = (lmm_element_t)elemIt;
      xbt_assert(elem->variable->sharing_weight > 0);
      concurrency += lmm_element_concurrency(elem);
    }

    xbt_swag_foreach(elemIt, &(cnst->disabled_element_set))
    {
      lmm_element_t elem = (lmm_element_t)elemIt;
      // We should have staged variables only if concurrency is reached in some constraint
      xbt_assert(cnst->concurrency_limit < 0 || elem->variable->staged_weight == 0 ||
                     lmm_cnstrs_min_concurrency_slack(elem->variable) < elem->variable->concurrency_share,
                 "should not have staged variable!");
    }

    xbt_assert(cnst->concurrency_limit < 0 || cnst->concurrency_limit >= concurrency, "concurrency check failed!");
    xbt_assert(cnst->concurrency_current == concurrency, "concurrency_current is out-of-date!");
  }

  // Check that for each variable, all corresponding elements are in the same state (i.e. same element sets)
  void* varIt;
  xbt_swag_foreach(varIt, &variable_set)
  {
    lmm_variable_t var = (lmm_variable_t)varIt;

    if (var->cnsts.empty())
      continue;

    lmm_element_t elem     = &var->cnsts[0];
    int belong_to_enabled  = xbt_swag_belongs(elem, &(elem->constraint->enabled_element_set));
    int belong_to_disabled = xbt_swag_belongs(elem, &(elem->constraint->disabled_element_set));
    int belong_to_active   = xbt_swag_belongs(elem, &(elem->constraint->active_element_set));

    for (s_lmm_element_t const& elem : var->cnsts) {
      xbt_assert(belong_to_enabled == xbt_swag_belongs(&elem, &(elem.constraint->enabled_element_set)),
                 "Variable inconsistency (1): enabled_element_set");
      xbt_assert(belong_to_disabled == xbt_swag_belongs(&elem, &(elem.constraint->disabled_element_set)),
                 "Variable inconsistency (2): disabled_element_set");
      xbt_assert(belong_to_active == xbt_swag_belongs(&elem, &(elem.constraint->active_element_set)),
                 "Variable inconsistency (3): active_element_set");
    }
  }
}

void s_lmm_system_t::var_free(lmm_variable_t var)
{
  XBT_IN("(sys=%p, var=%p)", this, var);
  modified = 1;

  // TODOLATER Can do better than that by leaving only the variable in only one enabled_element_set, call
  // update_modified_set, and then remove it..
  if (not var->cnsts.empty())
    update_modified_set(var->cnsts[0].constraint);

  for (s_lmm_element_t& elem : var->cnsts) {
    if (var->sharing_weight > 0)
      lmm_decrease_concurrency(&elem);
    xbt_swag_remove(&elem, &(elem.constraint->enabled_element_set));
    xbt_swag_remove(&elem, &(elem.constraint->disabled_element_set));
    xbt_swag_remove(&elem, &(elem.constraint->active_element_set));
    int nelements = xbt_swag_size(&(elem.constraint->enabled_element_set)) +
                    xbt_swag_size(&(elem.constraint->disabled_element_set));
    if (nelements == 0)
      make_constraint_inactive(elem.constraint);
    else
      on_disabled_var(elem.constraint);
  }

  var->cnsts.clear();

  check_concurrency();

  xbt_mallocator_release(variable_mallocator, var);
  XBT_OUT();
}

s_lmm_system_t::s_lmm_system_t(bool selective_update)
{
  s_lmm_variable_t var;
  s_lmm_constraint_t cnst;

  modified                = 0;
  selective_update_active = selective_update;
  visited_counter         = 1;

  XBT_DEBUG("Setting selective_update_active flag to %d", selective_update_active);

  xbt_swag_init(&variable_set, xbt_swag_offset(var, variable_set_hookup));
  xbt_swag_init(&constraint_set, xbt_swag_offset(cnst, constraint_set_hookup));

  xbt_swag_init(&active_constraint_set, xbt_swag_offset(cnst, active_constraint_set_hookup));

  xbt_swag_init(&modified_constraint_set, xbt_swag_offset(cnst, modified_constraint_set_hookup));
  xbt_swag_init(&saturated_variable_set, xbt_swag_offset(var, saturated_variable_set_hookup));
  xbt_swag_init(&saturated_constraint_set, xbt_swag_offset(cnst, saturated_constraint_set_hookup));

  keep_track          = nullptr;
  variable_mallocator = xbt_mallocator_new(65536, s_lmm_system_t::variable_mallocator_new_f,
                                           s_lmm_system_t::variable_mallocator_free_f, nullptr);
  solve_fun = &lmm_solve;
}

s_lmm_system_t::~s_lmm_system_t()
{
  lmm_variable_t var;
  lmm_constraint_t cnst;

  while ((var = extract_variable())) {
    auto demangled = simgrid::xbt::demangle(typeid(*var->id).name());
    XBT_WARN("Probable bug: a %s variable (#%d) not removed before the LMM system destruction.", demangled.get(),
             var->id_int);
    var_free(var);
  }
  while ((cnst = extract_constraint()))
    cnst_free(cnst);

  xbt_mallocator_free(variable_mallocator);
}

void s_lmm_system_t::cnst_free(lmm_constraint_t cnst)
{
  make_constraint_inactive(cnst);
  delete cnst;
}

lmm_constraint_t s_lmm_system_t::constraint_new(void* id, double bound_value)
{
  lmm_constraint_t cnst = nullptr;
  s_lmm_element_t elem;

  cnst         = new s_lmm_constraint_t();
  cnst->id     = id;
  cnst->id_int = Global_const_debug_id++;
  xbt_swag_init(&(cnst->enabled_element_set), xbt_swag_offset(elem, enabled_element_set_hookup));
  xbt_swag_init(&(cnst->disabled_element_set), xbt_swag_offset(elem, disabled_element_set_hookup));
  xbt_swag_init(&(cnst->active_element_set), xbt_swag_offset(elem, active_element_set_hookup));

  cnst->bound = bound_value;
  cnst->concurrency_maximum=0;
  cnst->concurrency_current=0;
  cnst->concurrency_limit  = sg_concurrency_limit;
  cnst->usage = 0;
  cnst->sharing_policy = 1; /* FIXME: don't hardcode the value */
  insert_constraint(cnst);

  return cnst;
}

int lmm_constraint_concurrency_limit_get(lmm_constraint_t cnst)
{
 return cnst->concurrency_limit;
}

void lmm_constraint_concurrency_limit_set(lmm_constraint_t cnst, int concurrency_limit)
{
  xbt_assert(concurrency_limit<0 || cnst->concurrency_maximum<=concurrency_limit,
             "New concurrency limit should be larger than observed concurrency maximum. Maybe you want to call"
             " lmm_constraint_concurrency_maximum_reset() to reset the maximum?");
  cnst->concurrency_limit = concurrency_limit;
}

void lmm_constraint_concurrency_maximum_reset(lmm_constraint_t cnst)
{
  cnst->concurrency_maximum = 0;
}

int lmm_constraint_concurrency_maximum_get(lmm_constraint_t cnst)
{
 xbt_assert(cnst->concurrency_limit<0 || cnst->concurrency_maximum<=cnst->concurrency_limit,
            "Very bad: maximum observed concurrency is higher than limit. This is a bug of SURF, please report it.");
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

void* s_lmm_system_t::variable_mallocator_new_f()
{
  return new s_lmm_variable_t;
}

void s_lmm_system_t::variable_mallocator_free_f(void* var)
{
  delete static_cast<lmm_variable_t>(var);
}

lmm_variable_t s_lmm_system_t::variable_new(simgrid::surf::Action* id, double sharing_weight, double bound,
                                            int number_of_constraints)
{
  XBT_IN("(sys=%p, id=%p, weight=%f, bound=%f, num_cons =%d)", this, id, sharing_weight, bound, number_of_constraints);

  lmm_variable_t var = (lmm_variable_t)xbt_mallocator_get(variable_mallocator);
  var->id = id;
  var->id_int = Global_debug_id++;
  var->cnsts.reserve(number_of_constraints);
  var->sharing_weight    = sharing_weight;
  var->staged_weight = 0.0;
  var->bound = bound;
  var->concurrency_share = 1;
  var->value = 0.0;
  var->visited           = visited_counter - 1;
  var->mu = 0.0;
  var->new_mu = 0.0;
  var->func_f = func_f_def;
  var->func_fp = func_fp_def;
  var->func_fpi = func_fpi_def;

  var->variable_set_hookup.next = nullptr;
  var->variable_set_hookup.prev = nullptr;
  var->saturated_variable_set_hookup.next = nullptr;
  var->saturated_variable_set_hookup.prev = nullptr;

  if (sharing_weight)
    xbt_swag_insert_at_head(var, &variable_set);
  else
    xbt_swag_insert_at_tail(var, &variable_set);

  XBT_OUT(" returns %p", var);
  return var;
}

void s_lmm_system_t::variable_free(lmm_variable_t var)
{
  remove_variable(var);
  var_free(var);
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

void s_lmm_system_t::expand(lmm_constraint_t cnst, lmm_variable_t var, double consumption_weight)
{
  modified = 1;

  //Check if this variable already has an active element in this constraint
  //If it does, substract it from the required slack
  int current_share = 0;
  if(var->concurrency_share>1){
    for (s_lmm_element_t& elem : var->cnsts) {
      if (elem.constraint == cnst && xbt_swag_belongs(&elem, &(elem.constraint->enabled_element_set)))
        current_share += lmm_element_concurrency(&elem);
    }
  }

  //Check if we need to disable the variable
  if (var->sharing_weight > 0 && var->concurrency_share - current_share > lmm_concurrency_slack(cnst)) {
    double weight = var->sharing_weight;
    disable_var(var);
    for (s_lmm_element_t const& elem : var->cnsts)
      on_disabled_var(elem.constraint);
    consumption_weight = 0;
    var->staged_weight=weight;
    xbt_assert(not var->sharing_weight);
  }

  xbt_assert(var->cnsts.size() < var->cnsts.capacity(), "Too much constraints");

  var->cnsts.resize(var->cnsts.size() + 1);
  s_lmm_element_t& elem = var->cnsts.back();

  elem.consumption_weight = consumption_weight;
  elem.constraint         = cnst;
  elem.variable           = var;

  if (var->sharing_weight) {
    xbt_swag_insert_at_head(&elem, &(elem.constraint->enabled_element_set));
    lmm_increase_concurrency(&elem);
  } else
    xbt_swag_insert_at_tail(&elem, &(elem.constraint->disabled_element_set));

  if (not selective_update_active) {
    make_constraint_active(cnst);
  } else if (elem.consumption_weight > 0 || var->sharing_weight > 0) {
    make_constraint_active(cnst);
    update_modified_set(cnst);
    //TODOLATER: Why do we need this second call?
    if (var->cnsts.size() > 1)
      update_modified_set(var->cnsts[0].constraint);
  }

  check_concurrency();
}

void s_lmm_system_t::expand_add(lmm_constraint_t cnst, lmm_variable_t var, double value)
{
  modified = 1;

  check_concurrency();

  //BEWARE: In case you have multiple elements in one constraint, this will always add value to the first element.
  auto elem_it = std::find_if(begin(var->cnsts), end(var->cnsts),
                              [&cnst](s_lmm_element_t const& x) { return x.constraint == cnst; });
  if (elem_it != end(var->cnsts)) {
    s_lmm_element_t& elem = *elem_it;
    if (var->sharing_weight)
      lmm_decrease_concurrency(&elem);

    if (cnst->sharing_policy)
      elem.consumption_weight += value;
    else
      elem.consumption_weight = std::max(elem.consumption_weight, value);

    //We need to check that increasing value of the element does not cross the concurrency limit
    if (var->sharing_weight) {
      if (lmm_concurrency_slack(cnst) < lmm_element_concurrency(&elem)) {
        double weight = var->sharing_weight;
        disable_var(var);
        for (s_lmm_element_t const& elem2 : var->cnsts)
          on_disabled_var(elem2.constraint);
        var->staged_weight=weight;
        xbt_assert(not var->sharing_weight);
      }
      lmm_increase_concurrency(&elem);
    }
    update_modified_set(cnst);
  } else
    expand(cnst, var, value);

  check_concurrency();
}

lmm_constraint_t lmm_get_cnst_from_var(lmm_system_t /*sys*/, lmm_variable_t var, unsigned num)
{
  if (num < var->cnsts.size())
    return (var->cnsts[num].constraint);
  else
    return nullptr;
}

double lmm_get_cnst_weight_from_var(lmm_system_t /*sys*/, lmm_variable_t var, unsigned num)
{
  if (num < var->cnsts.size())
    return (var->cnsts[num].consumption_weight);
  else
    return 0.0;
}

int lmm_get_number_of_cnst_from_var(lmm_system_t /*sys*/, lmm_variable_t var)
{
  return (var->cnsts.size());
}

lmm_variable_t lmm_get_var_from_cnst(lmm_system_t /*sys*/, lmm_constraint_t cnst, lmm_element_t * elem)
{
  if (*elem == nullptr) {
    // That is the first call, pick the first element among enabled_element_set (or disabled_element_set if
    // enabled_element_set is empty)
    *elem = (lmm_element_t) xbt_swag_getFirst(&(cnst->enabled_element_set));
    if (*elem == nullptr)
      *elem = (lmm_element_t) xbt_swag_getFirst(&(cnst->disabled_element_set));
  } else {
    //elem is not null, so we carry on
    if(xbt_swag_belongs(*elem,&(cnst->enabled_element_set))){
      //Look at enabled_element_set, and jump to disabled_element_set when finished
      *elem = (lmm_element_t) xbt_swag_getNext(*elem, cnst->enabled_element_set.offset);
      if (*elem == nullptr)
        *elem = (lmm_element_t) xbt_swag_getFirst(&(cnst->disabled_element_set));
    } else {
      *elem = (lmm_element_t) xbt_swag_getNext(*elem, cnst->disabled_element_set.offset);
    }
  }
  if (*elem)
    return (*elem)->variable;
  else
    return nullptr;
}

//if we modify the swag between calls, normal version may loop forever
//this safe version ensures that we browse the swag elements only once
lmm_variable_t lmm_get_var_from_cnst_safe(lmm_system_t /*sys*/, lmm_constraint_t cnst, lmm_element_t * elem,
                                          lmm_element_t * nextelem, int * numelem)
{
  if (*elem == nullptr) {
    *elem = (lmm_element_t) xbt_swag_getFirst(&(cnst->enabled_element_set));
    *numelem = xbt_swag_size(&(cnst->enabled_element_set))+xbt_swag_size(&(cnst->disabled_element_set))-1;
    if (*elem == nullptr)
      *elem = (lmm_element_t) xbt_swag_getFirst(&(cnst->disabled_element_set));
  }else{
    *elem = *nextelem;
    if(*numelem>0){
     (*numelem) --;
    }else
      return nullptr;
  }
  if (*elem){
    //elem is not null, so we carry on
    if(xbt_swag_belongs(*elem,&(cnst->enabled_element_set))){
      //Look at enabled_element_set, and jump to disabled_element_set when finished
      *nextelem = (lmm_element_t) xbt_swag_getNext(*elem, cnst->enabled_element_set.offset);
      if (*nextelem == nullptr)
        *nextelem = (lmm_element_t) xbt_swag_getFirst(&(cnst->disabled_element_set));
    } else {
      *nextelem = (lmm_element_t) xbt_swag_getNext(*elem, cnst->disabled_element_set.offset);
    }
    return (*elem)->variable;
  }else
    return nullptr;
}

void *lmm_constraint_id(lmm_constraint_t cnst)
{
  return cnst->id;
}

void *lmm_variable_id(lmm_variable_t var)
{
  return var->id;
}

static inline void saturated_constraint_set_update(double usage, int cnst_light_num,
                                                   dyn_light_t& saturated_constraint_set, double* min_usage)
{
  xbt_assert(usage > 0,"Impossible");

  if (*min_usage < 0 || *min_usage > usage) {
    *min_usage = usage;
    XBT_HERE(" min_usage=%f (cnst->remaining / cnst->usage =%f)", *min_usage, usage);
    saturated_constraint_set.assign(1, cnst_light_num);
  } else if (*min_usage == usage) {
    saturated_constraint_set.emplace_back(cnst_light_num);
  }
}

static inline void saturated_variable_set_update(s_lmm_constraint_light_t* cnst_light_tab,
                                                 const dyn_light_t& saturated_constraint_set, lmm_system_t sys)
{
  /* Add active variables (i.e. variables that need to be set) from the set of constraints to saturate (cnst_light_tab)*/
  for (int const& saturated_cnst : saturated_constraint_set) {
    lmm_constraint_light_t cnst = &cnst_light_tab[saturated_cnst];
    void* _elem;
    xbt_swag_t elem_list = &(cnst->cnst->active_element_set);
    xbt_swag_foreach(_elem, elem_list) {
      lmm_element_t elem = (lmm_element_t)_elem;
      //Visiting active_element_set, so, by construction, should never get a zero weight, correct?
      xbt_assert(elem->variable->sharing_weight > 0);
      if (elem->consumption_weight > 0)
        xbt_swag_insert(elem->variable, &(sys->saturated_variable_set));
    }
  }
}

void s_lmm_system_t::print()
{
  std::string buf       = std::string("MAX-MIN ( ");
  void* _var;

  /* Printing Objective */
  xbt_swag_t var_list = &variable_set;
  xbt_swag_foreach(_var, var_list) {
    lmm_variable_t var = (lmm_variable_t)_var;
    buf = buf + "'" + std::to_string(var->id_int) + "'(" + std::to_string(var->sharing_weight) + ") ";
  }
  buf += ")";
  XBT_DEBUG("%20s", buf.c_str());
  buf.clear();

  XBT_DEBUG("Constraints");
  /* Printing Constraints */
  void* _cnst;
  xbt_swag_t cnst_list = &active_constraint_set;
  xbt_swag_foreach(_cnst, cnst_list) {
    lmm_constraint_t cnst = (lmm_constraint_t)_cnst;
    double sum = 0.0;
    //Show  the enabled variables
    void* _elem;
    xbt_swag_t elem_list = &(cnst->enabled_element_set);
    buf += "\t";
    buf += ((cnst->sharing_policy) ? "(" : "max(");
    xbt_swag_foreach(_elem, elem_list) {
      lmm_element_t elem = (lmm_element_t)_elem;
      buf  = buf + std::to_string(elem->consumption_weight) + ".'" + std::to_string(elem->variable->id_int) + "'(" +
            std::to_string(elem->variable->value) + ")" + ((cnst->sharing_policy) ? " + " : " , ");
      if(cnst->sharing_policy)
        sum += elem->consumption_weight * elem->variable->value;
      else
        sum = std::max(sum, elem->consumption_weight * elem->variable->value);
    }
    //TODO: Adding disabled elements only for test compatibility, but do we really want them to be printed?
    elem_list = &(cnst->disabled_element_set);
    xbt_swag_foreach(_elem, elem_list) {
      lmm_element_t elem = (lmm_element_t)_elem;
      buf  = buf + std::to_string(elem->consumption_weight) + ".'" + std::to_string(elem->variable->id_int) + "'(" +
            std::to_string(elem->variable->value) + ")" + ((cnst->sharing_policy) ? " + " : " , ");
      if(cnst->sharing_policy)
        sum += elem->consumption_weight * elem->variable->value;
      else
        sum = std::max(sum, elem->consumption_weight * elem->variable->value);
    }

    buf = buf + "0) <= " + std::to_string(cnst->bound) + " ('" + std::to_string(cnst->id_int) + "')";

    if (not cnst->sharing_policy) {
      buf += " [MAX-Constraint]";
    }
    XBT_DEBUG("%s", buf.c_str());
    buf.clear();
    xbt_assert(not double_positive(sum - cnst->bound, cnst->bound * sg_maxmin_precision),
               "Incorrect value (%f is not smaller than %f): %g", sum, cnst->bound, sum - cnst->bound);
  }

  XBT_DEBUG("Variables");
  /* Printing Result */
  xbt_swag_foreach(_var, var_list) {
    lmm_variable_t var = (lmm_variable_t)_var;
    if (var->bound > 0) {
      XBT_DEBUG("'%d'(%f) : %f (<=%f)", var->id_int, var->sharing_weight, var->value, var->bound);
      xbt_assert(not double_positive(var->value - var->bound, var->bound * sg_maxmin_precision),
                 "Incorrect value (%f is not smaller than %f", var->value, var->bound);
    } else {
      XBT_DEBUG("'%d'(%f) : %f", var->id_int, var->sharing_weight, var->value);
    }
  }
}

void s_lmm_system_t::solve()
{
  void* _cnst;
  void* _cnst_next;
  void* _elem;
  double min_usage = -1;
  double min_bound = -1;

  if (not modified)
    return;

  XBT_IN("(sys=%p)", this);

  /* Compute Usage and store the variables that reach the maximum. If selective_update_active is true, only constraints
   * that changed are considered. Otherwise all constraints with active actions are considered.
   */
  xbt_swag_t cnst_list = selective_update_active ? &modified_constraint_set : &active_constraint_set;

  XBT_DEBUG("Active constraints : %d", xbt_swag_size(cnst_list));
  /* Init: Only modified code portions: reset the value of active variables */
  xbt_swag_foreach(_cnst, cnst_list) {
    lmm_constraint_t cnst = (lmm_constraint_t)_cnst;
    xbt_swag_t elem_list  = &(cnst->enabled_element_set);
    xbt_swag_foreach(_elem, elem_list) {
      lmm_variable_t var = ((lmm_element_t)_elem)->variable;
      xbt_assert(var->sharing_weight > 0.0);
      var->value = 0.0;
    }
  }

  s_lmm_constraint_light_t* cnst_light_tab = new s_lmm_constraint_light_t[xbt_swag_size(cnst_list)]();
  int cnst_light_num = 0;
  dyn_light_t saturated_constraint_set;

  xbt_swag_foreach_safe(_cnst, _cnst_next, cnst_list) {
    lmm_constraint_t cnst = (lmm_constraint_t)_cnst;
    /* INIT: Collect constraints that actually need to be saturated (i.e remaining  and usage are strictly positive)
     * into cnst_light_tab. */
    cnst->remaining = cnst->bound;
    if (not double_positive(cnst->remaining, cnst->bound * sg_maxmin_precision))
      continue;
    cnst->usage = 0;
    xbt_swag_t elem_list = &(cnst->enabled_element_set);
    xbt_swag_foreach(_elem, elem_list) {
      lmm_element_t elem = (lmm_element_t)_elem;
      xbt_assert(elem->variable->sharing_weight > 0);
      if (elem->consumption_weight > 0) {
        if (cnst->sharing_policy)
          cnst->usage += elem->consumption_weight / elem->variable->sharing_weight;
        else if (cnst->usage < elem->consumption_weight / elem->variable->sharing_weight)
          cnst->usage = elem->consumption_weight / elem->variable->sharing_weight;

        make_elem_active(elem);
        simgrid::surf::Action *action = static_cast<simgrid::surf::Action*>(elem->variable->id);
        if (keep_track && not action->is_linked())
          keep_track->push_back(*action);
      }
    }
    XBT_DEBUG("Constraint '%d' usage: %f remaining: %f concurrency: %i<=%i<=%i", cnst->id_int, cnst->usage,
              cnst->remaining,cnst->concurrency_current,cnst->concurrency_maximum,cnst->concurrency_limit);
    /* Saturated constraints update */

    if(cnst->usage > 0) {
      cnst_light_tab[cnst_light_num].cnst = cnst;
      cnst->cnst_light = &(cnst_light_tab[cnst_light_num]);
      cnst_light_tab[cnst_light_num].remaining_over_usage = cnst->remaining / cnst->usage;
      saturated_constraint_set_update(cnst_light_tab[cnst_light_num].remaining_over_usage,
        cnst_light_num, saturated_constraint_set, &min_usage);
      xbt_assert(cnst->active_element_set.count>0, "There is no sense adding a constraint that has no active element!");
      cnst_light_num++;
    }
  }

  saturated_variable_set_update(cnst_light_tab, saturated_constraint_set, this);

  /* Saturated variables update */
  do {
    /* Fix the variables that have to be */
    xbt_swag_t var_list = &saturated_variable_set;
    void* _var;
    lmm_variable_t var = nullptr;
    xbt_swag_foreach(_var, var_list) {
      var = (lmm_variable_t)_var;
      if (var->sharing_weight <= 0.0)
        DIE_IMPOSSIBLE;
      /* First check if some of these variables could reach their upper bound and update min_bound accordingly. */
      XBT_DEBUG("var=%d, var->bound=%f, var->weight=%f, min_usage=%f, var->bound*var->weight=%f", var->id_int,
                var->bound, var->sharing_weight, min_usage, var->bound * var->sharing_weight);
      if ((var->bound > 0) && (var->bound * var->sharing_weight < min_usage)) {
        if (min_bound < 0)
          min_bound = var->bound * var->sharing_weight;
        else
          min_bound = std::min(min_bound, (var->bound * var->sharing_weight));
        XBT_DEBUG("Updated min_bound=%f", min_bound);
      }
    }

    while ((var = (lmm_variable_t)xbt_swag_getFirst(var_list))) {
      if (min_bound < 0) {
        //If no variable could reach its bound, deal iteratively the constraints usage ( at worst one constraint is
        // saturated at each cycle)
        var->value = min_usage / var->sharing_weight;
        XBT_DEBUG("Setting var (%d) value to %f\n", var->id_int, var->value);
      } else {
         //If there exist a variable that can reach its bound, only update it (and other with the same bound) for now.
         if (double_equals(min_bound, var->bound * var->sharing_weight, sg_maxmin_precision)) {
           var->value = var->bound;
           XBT_DEBUG("Setting %p (%d) value to %f\n", var, var->id_int, var->value);
         } else {
           // Variables which bound is different are not considered for this cycle, but they will be afterwards.
           XBT_DEBUG("Do not consider %p (%d) \n", var, var->id_int);
           xbt_swag_remove(var, var_list);
           continue;
         }
      }
      XBT_DEBUG("Min usage: %f, Var(%d)->weight: %f, Var(%d)->value: %f ", min_usage, var->id_int, var->sharing_weight,
                var->id_int, var->value);

      /* Update the usage of contraints where this variable is involved */
      for (s_lmm_element_t& elem : var->cnsts) {
        lmm_constraint_t cnst = elem.constraint;
        if (cnst->sharing_policy) {
          // Remember: shared constraints require that sum(elem.value * var->value) < cnst->bound
          double_update(&(cnst->remaining), elem.consumption_weight * var->value, cnst->bound * sg_maxmin_precision);
          double_update(&(cnst->usage), elem.consumption_weight / var->sharing_weight, sg_maxmin_precision);
          // If the constraint is saturated, remove it from the set of active constraints (light_tab)
          if (not double_positive(cnst->usage, sg_maxmin_precision) ||
              not double_positive(cnst->remaining, cnst->bound * sg_maxmin_precision)) {
            if (cnst->cnst_light) {
              int index = (cnst->cnst_light-cnst_light_tab);
              XBT_DEBUG("index: %d \t cnst_light_num: %d \t || usage: %f remaining: %f bound: %f  ",
                         index,cnst_light_num, cnst->usage, cnst->remaining, cnst->bound);
              cnst_light_tab[index]=cnst_light_tab[cnst_light_num-1];
              cnst_light_tab[index].cnst->cnst_light = &cnst_light_tab[index];
              cnst_light_num--;
              cnst->cnst_light = nullptr;
            }
          } else {
            cnst->cnst_light->remaining_over_usage = cnst->remaining / cnst->usage;
          }
          make_elem_inactive(&elem);
        } else {
          // Remember: non-shared constraints only require that max(elem.value * var->value) < cnst->bound
          cnst->usage = 0.0;
          make_elem_inactive(&elem);
          xbt_swag_t elem_list = &(cnst->enabled_element_set);
          xbt_swag_foreach(_elem, elem_list) {
            lmm_element_t elem2 = static_cast<lmm_element_t>(_elem);
            xbt_assert(elem2->variable->sharing_weight > 0);
            if (elem2->variable->value > 0)
              continue;
            if (elem2->consumption_weight > 0)
              cnst->usage = std::max(cnst->usage, elem2->consumption_weight / elem2->variable->sharing_weight);
          }
          //If the constraint is saturated, remove it from the set of active constraints (light_tab)
          if (not double_positive(cnst->usage, sg_maxmin_precision) ||
              not double_positive(cnst->remaining, cnst->bound * sg_maxmin_precision)) {
            if(cnst->cnst_light) {
              int index = (cnst->cnst_light-cnst_light_tab);
              XBT_DEBUG("index: %d \t cnst_light_num: %d \t || \t cnst: %p \t cnst->cnst_light: %p "
                        "\t cnst_light_tab: %p usage: %f remaining: %f bound: %f  ", index,cnst_light_num, cnst,
                        cnst->cnst_light, cnst_light_tab, cnst->usage, cnst->remaining, cnst->bound);
              cnst_light_tab[index]=cnst_light_tab[cnst_light_num-1];
              cnst_light_tab[index].cnst->cnst_light = &cnst_light_tab[index];
              cnst_light_num--;
              cnst->cnst_light = nullptr;
            }
          } else {
            cnst->cnst_light->remaining_over_usage = cnst->remaining / cnst->usage;
            xbt_assert(cnst->active_element_set.count>0, "Should not keep a maximum constraint that has no active"
                       " element! You want to check the maxmin precision and possible rounding effects." );
          }
        }
      }
      xbt_swag_remove(var, var_list);
    }

    /* Find out which variables reach the maximum */
    min_usage = -1;
    min_bound = -1;
    saturated_constraint_set.clear();
    int pos;
    for(pos=0; pos<cnst_light_num; pos++){
      xbt_assert(cnst_light_tab[pos].cnst->active_element_set.count>0, "Cannot saturate more a constraint that has"
                 " no active element! You may want to change the maxmin precision (--cfg=maxmin/precision:<new_value>)"
                 " because of possible rounding effects.\n\tFor the record, the usage of this constraint is %g while "
                 "the maxmin precision to which it is compared is %g.\n\tThe usage of the previous constraint is %g.",
                 cnst_light_tab[pos].cnst->usage, sg_maxmin_precision, cnst_light_tab[pos-1].cnst->usage);
      saturated_constraint_set_update(cnst_light_tab[pos].remaining_over_usage, pos, saturated_constraint_set,
                                      &min_usage);
    }

    saturated_variable_set_update(cnst_light_tab, saturated_constraint_set, this);

  } while (cnst_light_num > 0);

  modified = 0;
  if (selective_update_active)
    remove_all_modified_set();

  if (XBT_LOG_ISENABLED(surf_maxmin, xbt_log_priority_debug)) {
    print();
  }

  check_concurrency();

  delete[] cnst_light_tab;
  XBT_OUT();
}

void lmm_solve(lmm_system_t sys)
{
  sys->solve();
}

/** \brief Attribute the value bound to var->bound.
 *
 *  \param sys the lmm_system_t
 *  \param var the lmm_variable_t
 *  \param bound the new bound to associate with var
 *
 *  Makes var->bound equal to bound. Whenever this function is called a change is  signed in the system. To
 *  avoid false system changing detection it is a good idea to test (bound != 0) before calling it.
 */
void s_lmm_system_t::update_variable_bound(lmm_variable_t var, double bound)
{
  modified   = 1;
  var->bound = bound;

  if (not var->cnsts.empty())
    update_modified_set(var->cnsts[0].constraint);
}

int lmm_concurrency_slack(lmm_constraint_t cnstr)
{
  if (cnstr->concurrency_limit < 0)
    return std::numeric_limits<int>::max();
  return  cnstr->concurrency_limit - cnstr->concurrency_current;
}

/** \brief Measure the minimum concurrency slack across all constraints where the given var is involved */
int lmm_cnstrs_min_concurrency_slack(lmm_variable_t var)
{
  int minslack = std::numeric_limits<int>::max();
  for (s_lmm_element_t const& elem : var->cnsts) {
    int slack = lmm_concurrency_slack(elem.constraint);
    if (slack < minslack) {
      // This is only an optimization, to avoid looking at more constraints when slack is already zero
      if (slack == 0)
        return 0;
      minslack = slack;
    }
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

//Small remark: In this implementation of lmm_enable_var and lmm_disable_var, we will meet multiple times with var when
// running sys->update_modified_set.
// A priori not a big performance issue, but we might do better by calling sys->update_modified_set within the for loops
// (after doing the first for enabling==1, and before doing the last for disabling==1)
void s_lmm_system_t::enable_var(lmm_variable_t var)
{
  xbt_assert(not XBT_LOG_ISENABLED(surf_maxmin, xbt_log_priority_debug) || lmm_can_enable_var(var));

  var->sharing_weight = var->staged_weight;
  var->staged_weight = 0;

  // Enabling the variable, move to var to list head. Subtlety is: here, we need to call update_modified_set AFTER
  // moving at least one element of var.

  xbt_swag_remove(var, &variable_set);
  xbt_swag_insert_at_head(var, &variable_set);
  for (s_lmm_element_t& elem : var->cnsts) {
    xbt_swag_remove(&elem, &(elem.constraint->disabled_element_set));
    xbt_swag_insert_at_head(&elem, &(elem.constraint->enabled_element_set));
    lmm_increase_concurrency(&elem);
  }
  if (not var->cnsts.empty())
    update_modified_set(var->cnsts[0].constraint);

  // When used within on_disabled_var, we would get an assertion fail, because transiently there can be variables
  // that are staged and could be activated.
  // Anyway, caller functions all call check_concurrency() in the end.
}

void s_lmm_system_t::disable_var(lmm_variable_t var)
{
  xbt_assert(not var->staged_weight, "Staged weight should have been cleared");
  // Disabling the variable, move to var to list tail. Subtlety is: here, we need to call update_modified_set
  // BEFORE moving the last element of var.
  xbt_swag_remove(var, &variable_set);
  xbt_swag_insert_at_tail(var, &variable_set);
  if (not var->cnsts.empty())
    update_modified_set(var->cnsts[0].constraint);
  for (s_lmm_element_t& elem : var->cnsts) {
    xbt_swag_remove(&elem, &(elem.constraint->enabled_element_set));
    xbt_swag_insert_at_tail(&elem, &(elem.constraint->disabled_element_set));

    xbt_swag_remove(&elem, &(elem.constraint->active_element_set));

    lmm_decrease_concurrency(&elem);
  }

  var->sharing_weight = 0.0;
  var->staged_weight=0.0;
  var->value = 0.0;
  check_concurrency();
}

/* /brief Find variables that can be enabled and enable them.
 *
 * Assuming that the variable has already been removed from non-zero weights
 * Can we find a staged variable to add?
 * If yes, check that none of the constraints that this variable is involved in is at the limit of its concurrency
 * And then add it to enabled variables
 */
void s_lmm_system_t::on_disabled_var(lmm_constraint_t cnstr)
{
  if(cnstr->concurrency_limit<0)
    return;

  int numelem = xbt_swag_size(&(cnstr->disabled_element_set));
  if (not numelem)
    return;

  lmm_element_t elem = (lmm_element_t)xbt_swag_getFirst(&(cnstr->disabled_element_set));

  //Cannot use xbt_swag_foreach, because lmm_enable_var will modify disabled_element_set.. within the loop
  while (numelem-- && elem) {

    lmm_element_t nextelem = (lmm_element_t)xbt_swag_getNext(elem, cnstr->disabled_element_set.offset);

    if (elem->variable->staged_weight > 0 && lmm_can_enable_var(elem->variable)) {
      //Found a staged variable
      //TODOLATER: Add random timing function to model reservation protocol fuzziness? Then how to make sure that
      //staged variables will eventually be called?
      enable_var(elem->variable);
    }

    xbt_assert(cnstr->concurrency_current<=cnstr->concurrency_limit,"Concurrency overflow!");
    if(cnstr->concurrency_current==cnstr->concurrency_limit)
      break;

    elem = nextelem;
  }

  //We could get an assertion fail, because transiently there can be variables that are staged and could be activated.
  //And we need to go through all constraints of the disabled var before getting back a coherent state.
  // Anyway, caller functions all call check_concurrency() in the end.
}

/* \brief update the weight of a variable, and enable/disable it.
 * @return Returns whether a change was made
 */
void s_lmm_system_t::update_variable_weight(lmm_variable_t var, double weight)
{
  xbt_assert(weight>=0,"Variable weight should not be negative!");

  if (weight == var->sharing_weight)
    return;

  int enabling_var  = (weight > 0 && var->sharing_weight <= 0);
  int disabling_var = (weight <= 0 && var->sharing_weight > 0);

  XBT_IN("(sys=%p, var=%p, weight=%f)", this, var, weight);

  modified = 1;

  //Are we enabling this variable?
  if (enabling_var){
    var->staged_weight = weight;
    int minslack       = lmm_cnstrs_min_concurrency_slack(var);
    if (minslack < var->concurrency_share) {
      XBT_DEBUG("Staging var (instead of enabling) because min concurrency slack %i, with weight %f and concurrency"
                " share %i", minslack, weight, var->concurrency_share);
      return;
    }
    XBT_DEBUG("Enabling var with min concurrency slack %i", minslack);
    enable_var(var);
  } else if (disabling_var){
    //Are we disabling this variable?
    disable_var(var);
  } else {
    var->sharing_weight = weight;
  }

  check_concurrency();

  XBT_OUT();
}

double lmm_get_variable_weight(lmm_variable_t var)
{
  return var->sharing_weight;
}

void s_lmm_system_t::update_constraint_bound(lmm_constraint_t cnst, double bound)
{
  modified = 1;
  update_modified_set(cnst);
  cnst->bound = bound;
}

/** \brief Update the constraint set propagating recursively to other constraints so the system should not be entirely
 *  computed.
 *
 *  \param sys the lmm_system_t
 *  \param cnst the lmm_constraint_t affected by the change
 *
 *  A recursive algorithm to optimize the system recalculation selecting only constraints that have changed. Each
 *  constraint change is propagated to the list of constraints for each variable.
 */
void s_lmm_system_t::update_modified_set_rec(lmm_constraint_t cnst)
{
  void* _elem;

  xbt_swag_foreach(_elem, &cnst->enabled_element_set) {
    lmm_variable_t var = ((lmm_element_t)_elem)->variable;
    for (s_lmm_element_t const& elem : var->cnsts) {
      if (var->visited == visited_counter)
        break;
      if (elem.constraint != cnst && not xbt_swag_belongs(elem.constraint, &modified_constraint_set)) {
        xbt_swag_insert(elem.constraint, &modified_constraint_set);
        update_modified_set_rec(elem.constraint);
      }
    }
    //var will be ignored in later visits as long as sys->visited_counter does not move
    var->visited = visited_counter;
  }
}

void s_lmm_system_t::update_modified_set(lmm_constraint_t cnst)
{
  /* nothing to do if selective update isn't active */
  if (selective_update_active && not xbt_swag_belongs(cnst, &modified_constraint_set)) {
    xbt_swag_insert(cnst, &modified_constraint_set);
    update_modified_set_rec(cnst);
  }
}

void s_lmm_system_t::remove_all_modified_set()
{
  // We cleverly un-flag all variables just by incrementing visited_counter
  // In effect, the var->visited value will no more be equal to visited counter
  // To be clean, when visited counter has wrapped around, we force these var->visited values so that variables that
  // were in the modified a long long time ago are not wrongly skipped here, which would lead to very nasty bugs
  // (i.e. not readibily reproducible, and requiring a lot of run time before happening).
  if (++visited_counter == 1) {
    /* the counter wrapped around, reset each variable->visited */
    void *_var;
    xbt_swag_foreach(_var, &variable_set)
      ((lmm_variable_t)_var)->visited = 0;
  }
  xbt_swag_reset(&modified_constraint_set);
}

/**
 * Returns resource load (in flop per second, or byte per second, or similar)
 *
 * If the resource is shared (the default case), the load is sum of resource usage made by every variables located on
 * this resource.
 *
 * If the resource is not shared (ie in FATPIPE mode), then the load is the max (not the sum) of all resource usages
 * located on this resource.
 *
 * \param cnst the lmm_constraint_t associated to the resource
 */
double lmm_constraint_get_usage(lmm_constraint_t cnst) {
  double usage         = 0.0;
  xbt_swag_t elem_list = &(cnst->enabled_element_set);
  void* _elem;

  xbt_swag_foreach(_elem, elem_list)
  {
    lmm_element_t elem = (lmm_element_t)_elem;
    if (elem->consumption_weight > 0) {
      if (cnst->sharing_policy)
        usage += elem->consumption_weight * elem->variable->value;
      else if (usage < elem->consumption_weight * elem->variable->value)
        usage = std::max(usage, elem->consumption_weight * elem->variable->value);
    }
  }
  return usage;
}

int lmm_constraint_get_variable_amount(lmm_constraint_t cnst) {
  int usage = 0;
  xbt_swag_t elem_list = &(cnst->enabled_element_set);
  void *_elem;

  xbt_swag_foreach(_elem, elem_list) {
    lmm_element_t elem = (lmm_element_t)_elem;
    if (elem->consumption_weight > 0)
      usage++;
  }
 return usage;
}
