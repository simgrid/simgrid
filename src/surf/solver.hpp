#include <xbt.h>
#include <vector>
#include "solver.h"
#include <boost/pool/object_pool.hpp>
#include <boost/bind.hpp>

#ifndef SURF_SOLVER_HPP_
#define SURF_SOLVER_HPP_
using namespace std;

/*static void double_update(double *variable, double value)
{
  *variable -= value;
  if (*variable < MAXMIN_PRECISION)
    *variable = 0.0;
}*/

class Solver;
class Element;
class Constraint;
class ConstraintLight;
class Variable;

struct ElementOps
{
  bool operator()( const Element & a, const Element & b )
    { return true; } //a > b; }
};

class Element {
public:
  Element(ConstraintPtr cnst, VariablePtr var, double value);
  ConstraintPtr p_constraint;
  VariablePtr p_variable;
  double m_value;

  void activate();
  void inactivate();
};

class ConstraintLight {
public:
  ConstraintLight(ConstraintPtr cnst, double remainingOverUsage):
    m_remainingOverUsage(remainingOverUsage), p_cnst(cnst) {};
  double m_remainingOverUsage;
  ConstraintPtr p_cnst;
};

class Constraint {
public:
  Constraint(void *id, double bound);
  ~Constraint() {
  std::cout << "Del Const:" << m_bound << std::endl;    
  };
  
  void shared();
  bool isShared();
  void* id();
  VariablePtr getVar(ElementPtr elem);
  void addElement(ElementPtr elem);

  vector<ElementPtr> m_elementSet;     /* a list of lmm_element_t */
  int m_elementsZeroWeight;
  vector<ElementPtr> m_activeElementSet;      /* a list of lmm_element_t */
  double m_remaining;
  double m_usage;
  double m_bound;
  int m_shared;
  void *p_id;
  int m_idInt;
  double m_lambda;
  double m_newLambda;
  ConstraintLightPtr p_cnstLight;
};

class Variable {
public:
  Variable(void *id, double weight, double bound, int visited);
  ~Variable() {
  std::cout << "Del Variable" << std::endl;
  };

  double value();
  double bound();
  void* id();
  double getWeight();
  int getNumberOfCnst();
  ConstraintPtr getCnst(int num);
  double getCnstWeight(int num);
  double isLimitedByLatency();

  /* \begin{For Lagrange only} */
  double (*p_funcF) (VariablePtr var, double x);       /* (f)    */
  double (*p_funcFP) (VariablePtr var, double x);      /* (f')    */
  double (*p_funcFPI) (VariablePtr var, double x);     /* (f')^{-1}    */
  /* \end{For Lagrange only} */
  vector<ElementPtr> m_cnsts;

  unsigned m_visited;             /* used by lmm_update_modified_set */
  double m_weight;
  double m_bound;
  double m_value;
  void *p_id;
  int m_idInt;
  double m_mu;
  double m_newMu;

protected:
  /* \begin{For Lagrange only} */
  /* \end{For Lagrange only} */
};

class Solver {
public:
  Solver(int selective_update);
  ~Solver() {
  std::cout << "Del Solver" << std::endl;    
  }	  

  inline void disableVariable(VariablePtr var);
  ConstraintPtr createConstraint(void *id, double bound_value);
  VariablePtr createVariable(void *id, double weight, double bound);
  void expand(ConstraintPtr cnst, VariablePtr var,  double value);
  void expandAdd(ConstraintPtr cnst, VariablePtr var,  double value);
  void elementSetValue(ConstraintPtr cnst, VariablePtr var, double value);
  void saturatedConstraintSetUpdate(double usage, ConstraintLightPtr cnstLight,
                                  vector<ConstraintLightPtr> saturatedConstraintSet,
                                  double *minUsage);
  void saturatedVariableSetUpdate(vector<ConstraintLightPtr> cnstLightList,
    vector<ConstraintLightPtr> saturatedConstraintSet);
  void solve();
  void update(ConstraintPtr cnst, VariablePtr var, double value);
  void updateVariableBound(VariablePtr var, double bound);
  void updateVariableWeight(VariablePtr var, double weight);
  void updateConstraintBound(ConstraintPtr cnst, double bound);
  bool constraintUsed(ConstraintPtr cnst);
  ConstraintPtr getFirstActiveConstraint();
  ConstraintPtr getNextActiveConstraint(ConstraintPtr cnst);
  void updateModifiedSetRec(ConstraintPtr cnst);
  void updateModifiedSet(ConstraintPtr cnst);
  void removeAllModifiedSet();
  void activateConstraint(ConstraintPtr cnst);
  void inactivateConstraint(ConstraintPtr cnst);
  void print();

  vector<VariablePtr>   m_variableSet;    /* a list of lmm_variable_t */
  vector<VariablePtr> m_saturatedVariableSet;  /* a list of lmm_variable_t */
  vector<ConstraintPtr> m_activeConstraintSet;   /* a list of lmm_constraint_t */
  vector<ConstraintPtr> m_constraintSet;  /* a list of lmm_constraint_t */
  vector<ConstraintPtr> m_modifiedConstraintSet; /* a list of modified lmm_constraint_t */
  vector<ConstraintPtr> m_saturatedConstraintSet;        /* a list of lmm_constraint_t_t */

  bool m_modified;
private:


protected:
  bool m_selectiveUpdateActive;  /* flag to update partially the system only selecting changed portions */
  unsigned m_visitedCounter;     /* used by lmm_update_modified_set */
  

  boost::object_pool<Constraint> m_constraintAllocator;
  boost::object_pool<Variable> m_variableAllocator;
  boost::object_pool<Element> m_elementAllocator;  
  void destroyConstraint(Constraint* cnst);
  void destroyVariable(Variable* var);
  void destroyElement(Element* elem);

  vector<void*> m_keepTrack;

  //xbt_mallocator_t variable_mallocator;
};

#endif /* SURF_SOLVER_H_ */
