#include <xbt.h>

#ifndef SURF_SOLVER_H_
#define SURF_SOLVER_H_

static double MAXMIN_PRECISION = 0.001;


#ifdef __cplusplus
#include <vector>
#include <boost/smart_ptr.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/bind.hpp>

static void double_update(double *variable, double value)
{
  *variable -= value;
  if (*variable < MAXMIN_PRECISION)
    *variable = 0.0;
}

using namespace std;

class Solver;
typedef boost::shared_ptr<Solver> SolverPtr;

class Element;
typedef boost::shared_ptr<Element> ElementPtr;

class Constraint;
typedef boost::shared_ptr<Constraint> ConstraintPtr;

class ConstraintLight;
typedef boost::shared_ptr<ConstraintLight> ConstraintLightPtr;

class Variable;
typedef boost::shared_ptr<Variable> VariablePtr;

struct ElementPtrOps
{
  bool operator()( const ElementPtr & a, const ElementPtr & b )
    { return true; } //a > b; }
};

#else
  typedef struct Solver Solver;
  typedef struct Element Element;
  typedef struct Constraint Constraint;
  typedef struct ConstraintLight ConstraintLight;
  typedef struct Variable Variable;

#endif

typedef Element *lmm_element_t;
typedef Variable *lmm_variable_t;
typedef Constraint *lmm_constraint_t;
typedef ConstraintLight *lmm_constraint_light_t;
typedef Solver *lmm_system_t;

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__STDC__) || defined(__cplusplus)
  extern lmm_system_t lmm_system_new(int selective_update);

  extern void c_function(Solver*);   /* ANSI C prototypes */
  extern Solver* cplusplus_callback_function(Solver*);

#else
  extern lmm_system_t lmm_system_new(int selective_update);

  extern void c_function();        /* K&R style */
  extern Solver* cplusplus_callback_function();
#endif

#ifdef __cplusplus
}
#endif





#endif /* SURF_SOLVER_H_ */
