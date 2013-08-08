#include "solver.hpp"

#include "xbt/log.h"
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_maxmin, surf,
                                "Logging specific to SURF (maxmin)");

static int Global_debug_id = 1;
static int Global_const_debug_id = 1;

Solver::Solver(int selectiveUpdate)
{
  VariablePtr var;
  ConstraintPtr cnst;

  m_modified = 0;
  m_selectiveUpdateActive = selectiveUpdate;
  m_visitedCounter = 1;

  XBT_DEBUG("Setting selective_update_active flag to %d\n",
         this->m_selectiveUpdateActive);

  /*xbt_swag_init(&(l->variable_set),
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
                                              lmm_variable_mallocator_reset_f);*/
}

ConstraintPtr Solver::createConstraint(void *id, double bound)
{
  ConstraintPtr cnst = m_constraintAllocator.construct(id, bound);
  m_constraintSet.push_back(cnst);
  return cnst;
}

void Solver::destroyConstraint(Constraint* cnst)
{
  m_constraintAllocator.destroy(cnst);
}


VariablePtr Solver::createVariable(void *id, double weight, double bound)
{
  void *mem = m_variableAllocator.malloc();
  VariablePtr var = new (mem) Variable(id, weight, bound, m_visitedCounter - 1);
  var->m_visited = m_visitedCounter - 1;
  if (weight)
    m_variableSet.insert(m_variableSet.begin(), var);
  else
    m_variableSet.push_back(var);
  return var;
}

void Solver::destroyVariable(Variable* var)
{
  m_variableAllocator.destroy(var);
}

void Solver::destroyElement(Element* elem)
{
  m_elementAllocator.destroy(elem);
}

void Solver::expand(ConstraintPtr cnst, VariablePtr var,  double value)
{
  m_modified = 1;
  
  ElementPtr elem = m_elementAllocator.construct(cnst, var, value);
  var->m_cnsts.push_back(elem);

  if (var->m_weight)
    cnst->m_elementSet.insert(cnst->m_elementSet.begin(), elem);
  else
    cnst->m_elementSet.push_back(elem);
  if(!m_selectiveUpdateActive) {
    activateConstraint(cnst);
  } else if(value>0 || var->m_weight >0) {
    activateConstraint(cnst);
    updateModifiedSet(cnst);
    if (var->m_cnsts.size() > 1)
      updateModifiedSet(var->m_cnsts.front()->p_constraint);
  }
}

void Solver::expandAdd(ConstraintPtr cnst, VariablePtr var,  double value)
{
  m_modified = 1;

  std::vector<ElementPtr>::iterator it;
  for (it=var->m_cnsts.begin(); it!=var->m_cnsts.end(); ++it )
    if ((*it)->p_constraint == cnst)
      break;
  
  if (it != var->m_cnsts.end()) {
    if (cnst->isShared())
      (*it)->m_value += value;
    else 
      (*it)->m_value = MAX((*it)->m_value, value);
    updateModifiedSet(cnst);
  } else
    expand(cnst, var, value);
}

void Solver::elementSetValue(ConstraintPtr cnst, VariablePtr var, double value)
{
  std::vector<ElementPtr>::iterator it;
  for (it=var->m_cnsts.begin(); it!=var->m_cnsts.end(); ++it ) {
    ElementPtr elem = (*it);
    if (elem->p_constraint == cnst) {
      elem->m_value = value;
      m_modified = 1;
      updateModifiedSet(cnst);
    }
  }
  if (it==var->m_cnsts.end()) {
    DIE_IMPOSSIBLE;
  }
}

//XBT_INLINE 
ConstraintPtr Variable::getCnst(int num)
{
  if (num < m_cnsts.size())
    return m_cnsts.at(num)->p_constraint;
  else {
    ConstraintPtr res;
    return res;
  }
}

//XBT_INLINE
double Variable::getCnstWeight(int num)
{
  if (num < m_cnsts.size())
    return (m_cnsts.at(num)->m_value);
  else
    return 0.0;
}

//XBT_INLINE 
int Variable::getNumberOfCnst()
{
  return m_cnsts.size();
}

VariablePtr Constraint::getVar(ElementPtr elem)
{
  vector<ElementPtr>::iterator it;
  if (!elem)
    elem = m_elementSet.front();
  else
    elem = *(++find(m_elementSet.begin(), m_elementSet.end(), elem));
  if (elem)
    return elem->p_variable;
  else {
    VariablePtr res;
    return res;
  }
}

//XBT_INLINE
void* Constraint::id()
{
  return p_id;
}

//XBT_INLINE
void* Variable::id()
{
  return p_id;
}

//static XBT_INLINE
void Solver::saturatedConstraintSetUpdate(double usage,
                                  ConstraintLightPtr cnstLight,
                                  vector<ConstraintLightPtr> saturatedConstraintSet,
                                  double *minUsage)
{
  xbt_assert(usage > 0,"Impossible");
  
  if (*minUsage < 0 || *minUsage > usage) {
    *minUsage = usage;
    saturatedConstraintSet.clear();
    saturatedConstraintSet.push_back(cnstLight);
    XBT_HERE(" min_usage=%f (cnst->remaining / cnst->usage =%f)", *minUsage, usage);
  } else if (*minUsage == usage) {
    saturatedConstraintSet.push_back(cnstLight);
  }
}

//static XBT_INLINE
void Solver::saturatedVariableSetUpdate(vector<ConstraintLightPtr> cnstLightList,
                                        vector<ConstraintLightPtr> saturatedCnstSet)
{
  ConstraintLight* cnst = NULL;
  Element* elem = NULL;
  vector<ElementPtr>* elem_list;
  std::vector<ConstraintLightPtr>::iterator it;
  std::vector<ElementPtr>::iterator elemIt;
  for (it=saturatedCnstSet.begin(); it!=saturatedCnstSet.end(); ++it) {
    cnst = (*it);
    elem_list = &(cnst->p_cnst->m_activeElementSet);
    for (elemIt=elem_list->begin(); elemIt!=elem_list->end(); ++elemIt) {
      elem = (*elemIt);
      if (elem->p_variable->m_weight <= 0)
	break;
      if (elem->m_value >0)
	m_saturatedVariableSet.push_back(elem->p_variable);
    }
  }
}

void Element::activate()
{
  p_constraint->m_activeElementSet.push_back(this);
}

void Element::inactivate()
{
  std::vector<ElementPtr>::iterator it;
  vector<ElementPtr> elemSet = p_constraint->m_activeElementSet;  
  it = find(elemSet.begin(), elemSet.end(), this);
  if (it != elemSet.end())
    elemSet.erase(it);
}

void Solver::activateConstraint(ConstraintPtr cnst)
{
  m_activeConstraintSet.push_back(cnst);
}

void Solver::inactivateConstraint(ConstraintPtr cnst)
{
  std::vector<ConstraintPtr>::iterator it;
  it = find(m_activeConstraintSet.begin(), m_activeConstraintSet.end(), cnst);
  if (it != m_activeConstraintSet.end())
    m_activeConstraintSet.erase(it);
  m_modifiedConstraintSet.erase(std::find(m_modifiedConstraintSet.begin(), m_modifiedConstraintSet.end(), cnst));
}

template <class T>
void vector_remove_first(vector<T> vec, T obj) {
  typename vector<T>::iterator it;
  for (it=vec.begin(); it != vec.end(); ++it) {
    if ((*it) == (obj)) {
      vec.erase(it);
      break;
    }
  }
}

void Solver::print()
{
  //TODO
}

void Solver::solve()
{
  Variable* var = NULL;
  Constraint* cnst = NULL;
  Element* elem = NULL;
  vector<ConstraintPtr>* cnstList = NULL;
  std::vector<ConstraintPtr>::iterator cnstIt;
  vector<VariablePtr>* varList = NULL;
  std::vector<VariablePtr>::iterator varIt;    
  vector<ElementPtr>* elemList = NULL;
  std::vector<ElementPtr>::iterator elemIt;
  vector<ElementPtr>* elemListB = NULL;
  std::vector<ElementPtr>::iterator elemItB;
  double minUsage = -1;
  double minBound = -1;

  if (!m_modified)
    return;

  XBT_IN("(sys=%p)", this);

  /*
   * Compute Usage and store the variables that reach the maximum.
   */
  cnstList = m_selectiveUpdateActive ? &m_modifiedConstraintSet :
                                       &m_activeConstraintSet;

  XBT_DEBUG("Active constraintsSolver : %d", cnstList->size());
  /* Init: Only modified code portions */
  
  for (cnstIt=cnstList->begin(); cnstIt!=cnstList->end(); ++cnstIt) {
    elemList = &((*cnstIt)->m_elementSet);
    for (elemIt=elemList->begin(); elemIt!=elemList->end(); ++elemIt) {
      var = ((*elemIt)->p_variable);
      if (var->m_weight <= 0.0)
	break;
      var->m_value = 0.0;
    }
  }

  vector<ConstraintLightPtr> cnstLightList;
  std::vector<ConstraintLightPtr>::iterator cnstLightIt;

  vector<ConstraintLightPtr> saturatedConstraintSet;
  saturatedConstraintSet.reserve(5);
  
  for (cnstIt=cnstList->begin(); cnstIt!=cnstList->end(); ++cnstIt) {
    /* INIT */
    cnst = (*cnstIt);
    if (cnst->m_remaining == 0)
      continue;
    cnst->m_usage = 0;
    elemList = &(cnst->m_elementSet);
    for (elemIt=elemList->begin(); elemIt!=elemList->end(); ++elemIt) {
      /* 0-weighted elements (ie, sleep actions) are at the end of the swag and we don't want to consider them */
      elem = (*elemIt);
      if (elem->p_variable->m_weight <= 0)
        break;
      if ((elem->m_value > 0)) {
        if (cnst->m_shared)
          cnst->m_usage += elem->m_value / elem->p_variable->m_weight;
        else if (cnst->m_usage < elem->m_value / elem->p_variable->m_weight)
          cnst->m_usage = elem->m_value / elem->p_variable->m_weight;

        elem->activate();
        if (m_keepTrack.size()) //TODO: check good semantic
	  m_keepTrack.push_back(elem->p_variable->p_id);
      }
    }
    XBT_DEBUG("Constraint Usage '%d' : %f", cnst->m_idInt, cnst->m_usage);
    /* Saturated constraints update */
    if(cnst->m_usage > 0) {
      ConstraintLightPtr cnstLight (new ConstraintLight((*cnstIt), cnst->m_remaining / cnst->m_usage));
      cnst->p_cnstLight = cnstLight;
      saturatedConstraintSetUpdate(cnstLight->m_remainingOverUsage, cnstLight, saturatedConstraintSet, &minUsage);
      cnstLightList.push_back(cnstLight);      
    }
  }

  saturatedVariableSetUpdate(cnstLightList, saturatedConstraintSet);

  /* Saturated variables update */
  do {
    /* Fix the variables that have to be */
    varList = &m_saturatedVariableSet;

    for (varIt=varList->begin(); varIt!=varList->end(); ++varIt) {
      var = (*varIt);
      if (var->m_weight <= 0.0) {
        DIE_IMPOSSIBLE;
      }
      /* First check if some of these variables have reach their upper
         bound and update min_usage accordingly. */
      XBT_DEBUG
          ("var=%d, var->bound=%f, var->weight=%f, min_usage=%f, var->bound*var->weight=%f",
           var->m_idInt, var->m_bound, var->m_weight, minUsage,
           var->m_bound * var->m_weight);

      if ((var->m_bound > 0) && (var->m_bound * var->m_weight < minUsage)) {
        if (minBound < 0)
          minBound = var->m_bound;
        else
          minBound = MIN(minBound, var->m_bound);
        XBT_DEBUG("Updated min_bound=%f", minBound);
      }
    }

    while (varList->size()>0) {
      var = varList->front();
      int i;

      if (minBound < 0) {
        var->m_value = minUsage / var->m_weight;
      } else {
        if (minBound == var->m_bound)
          var->m_value = var->m_bound;
        else {
          vector_remove_first(*varList, varList->front());
          continue;
        }
      }
      XBT_DEBUG("Min usage: %f, Var(%d)->weight: %f, Var(%d)->value: %f ",
             minUsage, var->m_idInt, var->m_weight, var->m_idInt, var->m_value);

      /* Update usage */

      for (elemIt=var->m_cnsts.begin(); elemIt!=var->m_cnsts.end(); ++elemIt) {
        elem = (*elemIt);
        cnst = elem->p_constraint;
        if (cnst->m_shared) {
          double_update(&(cnst->m_remaining), elem->m_value * var->m_value);
          double_update(&(cnst->m_usage), elem->m_value / var->m_weight);
          if(cnst->m_usage<=0 || cnst->m_remaining<=0) {
            if (cnst->p_cnstLight) {
	      // TODO: reformat message
              XBT_DEBUG("index: %d \t cnst_light_num: %d \t || \t cnst: %p \t cnst->cnst_light: %p \t cnst_light_tab: %p ",
                  elemIt - var->m_cnsts.begin(), var->m_cnsts.size(), cnst, cnst->p_cnstLight, &cnstLightList);
	      vector_remove_first(cnstLightList, cnst->p_cnstLight);
              cnst->p_cnstLight = ConstraintLightPtr();
            }
          } else {
            cnst->p_cnstLight->m_remainingOverUsage = cnst->m_remaining / cnst->m_usage;
          }
          elem->inactivate();
        } else {
          cnst->m_usage = 0.0;
          elem->inactivate();
          elemListB = &(cnst->m_elementSet);
          for (elemItB=elemListB->begin(); elemItB!=elemListB->end(); ++elemItB) {
            elem = (*elemItB);
            if (elem->p_variable->m_weight <= 0 || elem->p_variable->m_value > 0)
              break;
            if (elem->m_value > 0)
              cnst->m_usage = MAX(cnst->m_usage, elem->m_value / elem->p_variable->m_weight);
          }
          if (cnst->m_usage<=0 || cnst->m_remaining<=0) {
            if(cnst->p_cnstLight) {
	      vector_remove_first(cnstLightList, cnst->p_cnstLight);
	      // TODO: reformat message	      
              XBT_DEBUG("index: %d \t cnst_light_num: %d \t || \t cnst: %p \t cnst->cnst_light: %p \t cnst_light_tab: %p ",
                  elemIt - var->m_cnsts.begin(),  var->m_cnsts.size(), cnst, cnst->p_cnstLight, &cnstLightList);
              cnst->p_cnstLight = ConstraintLightPtr();
            }
          } else {
            cnst->p_cnstLight->m_remainingOverUsage = cnst->m_remaining / cnst->m_usage;
          }
        }
      }
      vector_remove_first(*varList, varList->front());
    }

    /* Find out which variables reach the maximum */
    minUsage = -1;
    minBound = -1;
    m_saturatedConstraintSet.clear();

    for (cnstLightIt=cnstLightList.begin(); cnstLightIt!=cnstLightList.end(); ++cnstLightIt)
      saturatedConstraintSetUpdate((*cnstLightIt)->m_remainingOverUsage, 
	                           (*cnstLightIt),
				    saturatedConstraintSet,
				    &minUsage);
    saturatedVariableSetUpdate(cnstLightList, saturatedConstraintSet);

  } while (cnstLightList.size() > 0);

  m_modified = 0;

  if (m_selectiveUpdateActive)
    removeAllModifiedSet();
  

  if (m_selectiveUpdateActive)
    removeAllModifiedSet();

  if (XBT_LOG_ISENABLED(surf_maxmin, xbt_log_priority_debug)) {
    print();
  }

  /*TODO: xbt_free(saturated_constraint_set->data);
  xbt_free(saturated_constraint_set);
  xbt_free(cnst_light_tab);*/
  XBT_OUT();
}

/* Not a O(1) function */

void Solver::update(ConstraintPtr cnst, VariablePtr var, double value)
{
  std::vector<ElementPtr>::iterator it;
  for (it=var->m_cnsts.begin(); it!=var->m_cnsts.end(); ++it )
    if ((*it)->p_constraint == cnst) {
      (*it)->m_value = value;
      m_modified = true;
      updateModifiedSet(cnst);
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
void Solver::updateVariableBound(VariablePtr var, double bound)
{
  m_modified = 1;
  var->m_bound = bound;

  if (var->m_cnsts.size() > 0)
    updateModifiedSet(var->m_cnsts.front()->p_constraint);
}

void Solver::updateVariableWeight(VariablePtr var, double weight)
{
  int i;

  if (weight == var->m_weight)
    return;
  XBT_IN("(sys=%p, var=%p, weight=%f)", this, var, weight);
  m_modified = 1;
  var->m_weight = weight;
  vector_remove_first(m_variableSet, var);
  if (weight) // TODO: use swap instead
    m_variableSet.insert(m_variableSet.begin(), var);
  else
    m_variableSet.push_back(var);

  vector<ElementPtr> elemList;
  vector<ElementPtr>::iterator it;
  for (it=var->m_cnsts.begin(); it!=var->m_cnsts.end(); ++it ) {
    elemList = (*it)->p_constraint->m_elementSet;
    vector_remove_first(elemList, (*it));
    if (weight)
      elemList.insert(elemList.begin(), *it);
    else
      elemList.push_back(*it);
    if (it == var->m_cnsts.begin())
      updateModifiedSet((*it)->p_constraint);
  }
  if (!weight)
    var->m_value = 0.0;

  XBT_OUT();
}

double Variable::getWeight()
{ return m_weight; }

//XBT_INLINE
void Solver::updateConstraintBound(ConstraintPtr cnst, double bound)
{
  m_modified = 1;
  updateModifiedSet(cnst);
  cnst->m_bound = bound;
}

//XBT_INLINE
bool Solver::constraintUsed(ConstraintPtr cnst)
{
  return std::find(m_activeConstraintSet.begin(),
                   m_activeConstraintSet.end(), cnst)!=m_activeConstraintSet.end();
}

//XBT_INLINE
ConstraintPtr Solver::getFirstActiveConstraint()
{
  return m_activeConstraintSet.front();
}

//XBT_INLINE 
ConstraintPtr Solver::getNextActiveConstraint(ConstraintPtr cnst)
{
  return *(++std::find(m_activeConstraintSet.begin(), m_activeConstraintSet.end(), cnst));
}

#ifdef HAVE_LATENCY_BOUND_TRACKING
//XBT_INLINE 
bool Variable::isLimitedByLatency()
{
  return (double_equals(m_bound, m_value));
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
//static 
void Solver::updateModifiedSetRec(ConstraintPtr cnst)
{
  std::vector<ElementPtr>::iterator elemIt;
  for (elemIt=cnst->m_elementSet.begin(); elemIt!=cnst->m_elementSet.end(); ++elemIt) {
    VariablePtr var = (*elemIt)->p_variable;
    vector<ElementPtr> cnsts = var->m_cnsts;
    std::vector<ElementPtr>::iterator cnstIt;
    for (cnstIt=cnsts.begin(); var->m_visited != m_visitedCounter 
        && cnstIt!=cnsts.end(); ++cnstIt){
      if ((*cnstIt)->p_constraint != cnst
	  && std::find(m_modifiedConstraintSet.begin(),
                   m_modifiedConstraintSet.end(), (*cnstIt)->p_constraint)
	     == m_modifiedConstraintSet.end()) {
	m_modifiedConstraintSet.push_back((*cnstIt)->p_constraint);
        updateModifiedSetRec((*cnstIt)->p_constraint);
      }
    }
    var->m_visited = m_visitedCounter;
  }
}

//static
void Solver::updateModifiedSet(ConstraintPtr cnst)
{
  /* nothing to do if selective update isn't active */
  if (m_selectiveUpdateActive
      && std::find(m_modifiedConstraintSet.begin(),
                   m_modifiedConstraintSet.end(), cnst)
	 == m_modifiedConstraintSet.end()) {
    m_modifiedConstraintSet.push_back(cnst);
    updateModifiedSetRec(cnst);
  }
}

/** \brief Remove all constraints of the modified_constraint_set.
 *
 *  \param sys the lmm_system_t
 */
//static 
void Solver::removeAllModifiedSet()
{
  if (++m_visitedCounter == 1) {
    /* the counter wrapped around, reset each variable->visited */
    std::vector<VariablePtr>::iterator it;
    for (it=m_variableSet.begin(); it!=m_variableSet.end(); ++it)
      (*it)->m_visited = 0;
  }
  m_modifiedConstraintSet.clear();
}

inline void Solver::disableVariable(VariablePtr var)
{
  int i;
  bool m = false;

  ElementPtr elem;

  XBT_IN("(sys=%p, var=%p)", this, var);
  m_modified = 1;
  
  std::vector<ElementPtr>::iterator varIt, elemIt;
  for (varIt=var->m_cnsts.begin(); varIt!=var->m_cnsts.end();  ) {
    vector<ElementPtr> elemSet = (*varIt)->p_constraint->m_elementSet;
    elemSet.erase(std::find(elemSet.begin(), elemSet.end(), *varIt));
    vector<ElementPtr> activeElemSet = (*varIt)->p_constraint->m_activeElementSet;
    activeElemSet.erase(std::find(activeElemSet.begin(), activeElemSet.end(), *varIt));
    if (elemSet.empty()) {
      inactivateConstraint((*varIt)->p_constraint);
      var->m_cnsts.erase(varIt);
    } else {
      ++varIt;
      m = true;
    }
  }
  if (m)
    updateModifiedSet(var->m_cnsts.front()->p_constraint);
  var->m_cnsts.clear();

  XBT_OUT();
}

Constraint::Constraint(void *id, double bound): 
  p_id(id), m_idInt(1), m_bound(bound), m_usage(0), m_shared(1),
  m_elementsZeroWeight(0)
{
  m_idInt = Global_const_debug_id++;
}

void Constraint::addElement(ElementPtr elem)
{
  m_elementSet.push_back(elem);
  if (elem->p_variable->m_weight<0)
    std::swap(m_elementSet[m_elementSet.size()-1], m_elementSet[m_elementsZeroWeight++]);
}

void Constraint::shared() {
  m_shared = 0;
}

bool Constraint::isShared() {
  return m_shared;
}

Variable::Variable(void *id, double weight, double bound, int visited)
{
  int i;

  // TODO: reformat
  XBT_IN("(sys=%p, id=%p, weight=%f, bound=%f, num_cons =%d)",
           0, id, weight, bound, 0);

  p_id = id;
  m_idInt = Global_debug_id++;
  
  // TODO: optimize cache 

  m_weight = weight;
  m_bound = bound;
  m_value = 0.0;
  m_visited = visited;//sys->visited_counter - 1;
  m_mu = 0.0;
  m_newMu = 0.0;
  //m_func_f = func_f_def;
  //m_func_fp = func_fp_def;
  //m_func_fpi = func_fpi_def;


  XBT_OUT(" returns %p", this);
}

double Variable::value()
{
  return m_value;
}

double Variable::bound()
{
  return m_bound;
}

Element::Element(ConstraintPtr cnst, VariablePtr var, double value):
  p_constraint(cnst), p_variable(var), m_value(value)
{}

