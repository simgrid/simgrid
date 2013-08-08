#include "solver.hpp"
#include <boost/smart_ptr.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/bind.hpp>

double sg_maxmin_precision = 0.00001;
#define RENO_SCALING 1.0

void lmm_solve(lmm_system_t solver)
{
  solver->solve();	
}

void lmm_print(lmm_system_t solver)
{
  solver->print();
}

lmm_variable_t lmm_get_var_from_cnst(lmm_system_t sys, lmm_constraint_t cnst, lmm_element_t * elem)
{
  cnst->getVar(*elem);
}

lmm_constraint_t lmm_get_cnst_from_var(lmm_system_t sys, lmm_variable_t var, int num)
{
  var->getCnst(num);
}

double lmm_get_cnst_weight_from_var(lmm_system_t sys, lmm_variable_t var, int num)
{
  var->getCnstWeight(num);
}

int lmm_get_number_of_cnst_from_var(lmm_system_t sys, lmm_variable_t var)
{
  var->getNumberOfCnst();
}

lmm_variable_t lmm_variable_new(lmm_system_t sys, void *id,
                                            double weight_value,
                                            double bound,
                                            int number_of_constraints)
{
  return sys->createVariable(id, weight_value, bound);
}

void *lmm_variable_id(lmm_variable_t var)
{
  return var->id();
}

double lmm_variable_getvalue(lmm_variable_t var)
{
  return var->m_value;
}

double lmm_get_variable_weight(lmm_variable_t var)
{
  return var->m_weight;
}

void lmm_variable_free(lmm_system_t sys, lmm_variable_t var)
{
  //TOREPAIR free
}

lmm_constraint_t lmm_constraint_new(lmm_system_t sys, void *id,
                                    double bound_value)	  
{
  return sys->createConstraint(id, bound_value);
}

void *lmm_constraint_id(lmm_constraint_t cnst)
{
  return cnst->id();
}

void lmm_constraint_shared(lmm_constraint_t cnst)
{
  cnst->shared();
}

int lmm_constraint_is_shared(lmm_constraint_t cnst)
{
  return cnst->isShared();
}

int lmm_constraint_used(lmm_system_t sys, lmm_constraint_t cnst)
{
  return (int) sys->constraintUsed(cnst);
}

void lmm_constraint_free(lmm_system_t sys, lmm_constraint_t cnst)
{
  //TOREPAIR free
}

lmm_system_t lmm_system_new(int selective_update) {
  return new Solver(selective_update);
}

void lmm_system_free(lmm_system_t sys) {
  //TOREPAIR free
}

int lmm_system_modified(lmm_system_t solver)
{
  solver->m_modified;
}
void lmm_expand(lmm_system_t sys, lmm_constraint_t cnst, lmm_variable_t var, double value)
{
  sys->expand(cnst, var, value);
}

void lmm_expand_add(lmm_system_t sys, lmm_constraint_t cnst,
                    lmm_variable_t var, double value)
{
  sys->expandAdd(cnst, var, value);
}

void lmm_update_variable_bound(lmm_system_t sys, lmm_variable_t var,
                               double bound)
{
  sys->updateVariableBound(var, bound);
}

void lmm_update_variable_weight(lmm_system_t sys,
                                            lmm_variable_t var,
                                            double weight)
{
  sys->updateVariableWeight(var, weight);
}

void lmm_update_constraint_bound(lmm_system_t sys,
                                             lmm_constraint_t cnst,
                                             double bound)
{
  sys->updateConstraintBound(cnst, bound);
}


/*********
 * FIXES *
 *********/
int fix_constraint_is_active(lmm_system_t sys, lmm_constraint_t cnst)
{
  int found = 0;
  std::vector<ConstraintPtr>::iterator cnstIt;
  lmm_constraint_t cnst_tmp;  
  for (cnstIt=sys->m_activeConstraintSet.begin(); cnstIt!=sys->m_activeConstraintSet.end(); ++cnstIt) {
    cnst_tmp = *cnstIt;
    if (cnst_tmp == cnst) {
      found = 1;
      break;
    }
  }
  return found;
}

