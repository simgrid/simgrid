#include <xbt.h>
#include <math.h>

#ifndef SURF_SOLVER_H_
#define SURF_SOLVER_H_

static double MAXMIN_PRECISION = 0.001;
extern double sg_maxmin_precision;

static XBT_INLINE int double_equals(double value1, double value2)
{
  return (fabs(value1 - value2) < MAXMIN_PRECISION);
}

static XBT_INLINE void double_update(double *variable, double value)
{
  *variable -= value;
  if (*variable < MAXMIN_PRECISION)
    *variable = 0.0;
}

#ifdef __cplusplus
#include <vector>
#include <boost/smart_ptr.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/bind.hpp>

using namespace std;

static XBT_INLINE int double_positive(double value)
{
  return (value > MAXMIN_PRECISION);
}

class Solver;
class Element;
class Constraint;
class ConstraintLight;
class Variable;

/*struct ElementPtrOps
{
  bool operator()( const ElementPtr & a, const ElementPtr & b )
    { return true; } //a > b; }
};*/

#else
  typedef struct Solver Solver;
  typedef struct Element Element;
  typedef struct Constraint Constraint;
  typedef struct ConstraintLight ConstraintLight;
  typedef struct Variable Variable;

#endif
typedef Element *ElementPtr;
typedef Variable *VariablePtr;
typedef Constraint *ConstraintPtr;
typedef ConstraintLight *ConstraintLightPtr;
typedef Solver *SolverPtr;

typedef ElementPtr lmm_element_t;
typedef VariablePtr lmm_variable_t;
typedef ConstraintPtr lmm_constraint_t;
typedef ConstraintLightPtr lmm_constraint_light_t;
typedef SolverPtr lmm_system_t;

extern double (*func_f_def) (lmm_variable_t, double);
extern double (*func_fp_def) (lmm_variable_t, double);
extern double (*func_fpi_def) (lmm_variable_t, double);

#ifdef __cplusplus
extern "C" {
#endif

  extern void _simgrid_log_category__surf_lagrange__constructor__(void);
  extern void _simgrid_log_category__surf_maxmin__constructor__(void);
  extern void _simgrid_log_category__surf_lagrange_dichotomy__constructor__(void);

  extern void lmm_set_default_protocol_function(double (*func_f)(lmm_variable_t var, double x),
	                                        double (*func_fp) (lmm_variable_t var, double x),
						double (*func_fpi) (lmm_variable_t var, double x));
  extern double func_reno_f(lmm_variable_t var, double x);
  extern double func_reno_fp(lmm_variable_t var, double x);
  extern double func_reno_fpi(lmm_variable_t var, double x);
  extern double func_reno2_f(lmm_variable_t var, double x);
  extern double func_reno2_fp(lmm_variable_t var, double x);
  extern double func_reno2_fpi(lmm_variable_t var, double x);
  extern double func_vegas_f(lmm_variable_t var, double x);
  extern double func_vegas_fp(lmm_variable_t var, double x);
  extern double func_vegas_fpi(lmm_variable_t var, double x);


  //extern int double_equals(double value1, double value2);
  //extern void double_update(double *variable, double value);

  extern lmm_variable_t lmm_get_var_from_cnst(lmm_system_t sys, lmm_constraint_t cnst, lmm_element_t * elem);
  extern lmm_constraint_t lmm_get_cnst_from_var(lmm_system_t sys, lmm_variable_t var, int num);
  extern double lmm_get_cnst_weight_from_var(lmm_system_t sys, lmm_variable_t var, int num);
  extern int lmm_get_number_of_cnst_from_var(lmm_system_t sys, lmm_variable_t var);

  extern lmm_variable_t lmm_variable_new(lmm_system_t sys, void *id,
                                            double weight_value,
                                            double bound,
                                            int number_of_constraints); 
  extern void *lmm_variable_id(lmm_variable_t var);	  
  extern double lmm_variable_getvalue(lmm_variable_t var);
  extern double lmm_get_variable_weight(lmm_variable_t var);
  extern void lmm_variable_free(lmm_system_t sys, lmm_variable_t var);
  
  extern lmm_constraint_t lmm_constraint_new(lmm_system_t sys, void *id,
                                    double bound_value);	  
  extern void *lmm_constraint_id(lmm_constraint_t cnst);
  extern void lmm_constraint_shared(lmm_constraint_t cnst);
  extern int lmm_constraint_is_shared(lmm_constraint_t cnst);
  extern int lmm_constraint_used(lmm_system_t sys, lmm_constraint_t cnst);
  extern void lmm_constraint_free(lmm_system_t sys, lmm_constraint_t cnst);

  extern lmm_system_t lmm_system_new(int selective_update);	  
  extern int lmm_system_modified(lmm_system_t solver);	  
  extern void lmm_expand(lmm_system_t sys, lmm_constraint_t cnst, lmm_variable_t var, double value);
  extern void lmm_expand_add(lmm_system_t sys, lmm_constraint_t cnst,
                    lmm_variable_t var, double value);
  extern void lmm_update_variable_bound(lmm_system_t sys, lmm_variable_t var,
                               double bound);
  extern void lmm_update_variable_weight(lmm_system_t sys,
                                            lmm_variable_t var,
                                            double weight);	  
  extern void lmm_update_constraint_bound(lmm_system_t sys,
                                             lmm_constraint_t cnst,
                                             double bound);
  extern void lmm_solve(lmm_system_t solver);
  extern void lagrange_solve(lmm_system_t solver);
  extern void bottleneck_solve(lmm_system_t solver);
  extern void lmm_system_free(lmm_system_t solver);

  extern void c_function(lmm_system_t);   /* ANSI C prototypes */
  extern lmm_system_t cplusplus_callback_function(lmm_system_t);
  extern void lmm_print(lmm_system_t sys);
  
  /*********
   * FIXES *
   *********/
  extern int fix_constraint_is_active(lmm_system_t sys, lmm_constraint_t cnst);

#ifdef __cplusplus
}
#endif





#endif /* SURF_SOLVER_H_ */
