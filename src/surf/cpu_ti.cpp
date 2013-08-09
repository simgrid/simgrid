#include "cpu_ti.hpp"
#include "solver.hpp"

#ifndef SURF_MODEL_CPUTI_H_
#define SURF_MODEL_CPUTI_H_

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_cpu_tii, surf,
                                "Logging specific to the SURF CPU TRACE INTEGRATION module");

static std::vector<CpuTiActionPtr>
    cpu_ti_running_action_set_that_does_not_need_being_checked;
static std::vector<CpuTiPtr> cpu_ti_modified_cpu;
static std::vector<CpuTiActionPtr> cpu_ti_action_heap;

/*********
 * Trace *
 *********/
static double surf_cpu_ti_integrate_trace(surf_cpu_ti_tgmr_t trace,
                                          double a, double b);


static double surf_cpu_ti_solve_trace(surf_cpu_ti_tgmr_t trace, double a,
                                      double amount);
static double surf_cpu_ti_solve_trace_somewhat_simple(surf_cpu_ti_tgmr_t
                                                      trace, double a,
                                                      double amount);

static void surf_cpu_ti_free_tmgr(surf_cpu_ti_tgmr_t trace);

static double surf_cpu_ti_integrate_trace_simple(surf_cpu_ti_trace_t trace,
                                                 double a, double b);
static double surf_cpu_ti_integrate_trace_simple_point(surf_cpu_ti_trace_t
                                                       trace, double a);
static double surf_cpu_ti_solve_trace_simple(surf_cpu_ti_trace_t trace,
                                             double a, double amount);
static int surf_cpu_ti_binary_search(double *array, double a, int low,
                                     int high);

static void surf_cpu_ti_free_trace(surf_cpu_ti_trace_t trace)
{
  xbt_free(trace->time_points);
  xbt_free(trace->integral);
  xbt_free(trace);
}

static void surf_cpu_ti_free_tmgr(surf_cpu_ti_tgmr_t trace)
{
  if (trace->trace)
    surf_cpu_ti_free_trace(trace->trace);
  xbt_free(trace);
}

static surf_cpu_ti_trace_t surf_cpu_ti_trace_new(tmgr_trace_t power_trace)
{
  surf_cpu_ti_trace_t trace;
  s_tmgr_event_t val;
  unsigned int cpt;
  double integral = 0;
  double time = 0;
  int i = 0;
  trace = xbt_new0(s_surf_cpu_ti_trace_t, 1);
  trace->time_points = (double*) xbt_malloc0(sizeof(double) *
                  (xbt_dynar_length(power_trace->s_list.event_list) + 1));
  trace->integral = (double*) xbt_malloc0(sizeof(double) *
                  (xbt_dynar_length(power_trace->s_list.event_list) + 1));
  trace->nb_points = xbt_dynar_length(power_trace->s_list.event_list);
  xbt_dynar_foreach(power_trace->s_list.event_list, cpt, val) {
    trace->time_points[i] = time;
    trace->integral[i] = integral;
    integral += val.delta * val.value;
    time += val.delta;
    i++;
  }
  trace->time_points[i] = time;
  trace->integral[i] = integral;
  return trace;
}

/**
* \brief Integrate trace
*
* Wrapper around surf_cpu_integrate_trace_simple() to get
* the cyclic effect.
*
* \param trace Trace structure.
* \param a      Begin of interval
* \param b      End of interval
* \return the integrate value. -1 if an error occurs.
*/
static double surf_cpu_ti_integrate_trace(surf_cpu_ti_tgmr_t trace,
                                          double a, double b)
{
  double first_chunk;
  double middle_chunk;
  double last_chunk;
  int a_index, b_index;

  if ((a < 0.0) || (a > b)) {
    XBT_CRITICAL
        ("Error, invalid integration interval [%.2f,%.2f]. You probably have a task executing with negative computation amount. Check your code.",
         a, b);
    xbt_abort();
  }
  if (a == b)
    return 0.0;

  if (trace->type == TRACE_FIXED) {
    return ((b - a) * trace->value);
  }

  if (ceil(a / trace->last_time) == a / trace->last_time)
    a_index = 1 + (int) (ceil(a / trace->last_time));
  else
    a_index = (int) (ceil(a / trace->last_time));

  b_index = (int) (floor(b / trace->last_time));

  if (a_index > b_index) {      /* Same chunk */
    return surf_cpu_ti_integrate_trace_simple(trace->trace,
                                              a - (a_index -
                                                   1) * trace->last_time,
                                              b -
                                              (b_index) *
                                              trace->last_time);
  }

  first_chunk = surf_cpu_ti_integrate_trace_simple(trace->trace,
                                                   a - (a_index -
                                                        1) *
                                                   trace->last_time,
                                                   trace->last_time);
  middle_chunk = (b_index - a_index) * trace->total;
  last_chunk = surf_cpu_ti_integrate_trace_simple(trace->trace,
                                                  0.0,
                                                  b -
                                                  (b_index) *
                                                  trace->last_time);

  XBT_DEBUG("first_chunk=%.2f  middle_chunk=%.2f  last_chunk=%.2f\n",
         first_chunk, middle_chunk, last_chunk);

  return (first_chunk + middle_chunk + last_chunk);
}

/**
 * \brief Auxiliary function to calculate the integral between a and b.
 *     It simply calculates the integral at point a and b and returns the difference 
 *   between them.
 * \param trace    Trace structure
 * \param a        Initial point
 * \param b  Final point
 * \return  Integral
*/
static double surf_cpu_ti_integrate_trace_simple(surf_cpu_ti_trace_t trace,
                                                 double a, double b)
{
  return surf_cpu_ti_integrate_trace_simple_point(trace,
                                                  b) -
      surf_cpu_ti_integrate_trace_simple_point(trace, a);
}

/**
 * \brief Auxiliary function to calculate the integral at point a.
 * \param trace    Trace structure
 * \param a        point
 * \return  Integral
*/
static double surf_cpu_ti_integrate_trace_simple_point(surf_cpu_ti_trace_t
                                                       trace, double a)
{
  double integral = 0;
  int ind;
  double a_aux = a;
  ind =
      surf_cpu_ti_binary_search(trace->time_points, a, 0,
                                trace->nb_points - 1);
  integral += trace->integral[ind];
  XBT_DEBUG
      ("a %lf ind %d integral %lf ind + 1 %lf ind %lf time +1 %lf time %lf",
       a, ind, integral, trace->integral[ind + 1], trace->integral[ind],
       trace->time_points[ind + 1], trace->time_points[ind]);
  double_update(&a_aux, trace->time_points[ind]);
  if (a_aux > 0)
    integral +=
        ((trace->integral[ind + 1] -
          trace->integral[ind]) / (trace->time_points[ind + 1] -
                                   trace->time_points[ind])) * (a -
                                                                trace->
                                                                time_points
                                                                [ind]);
  XBT_DEBUG("Integral a %lf = %lf", a, integral);

  return integral;
}

/**
* \brief Calculate the time needed to execute "amount" on cpu.
*
* Here, amount can span multiple trace periods
*
* \param trace   CPU trace structure
* \param a        Initial time
* \param amount  Amount to be executed
* \return  End time
*/
static double surf_cpu_ti_solve_trace(surf_cpu_ti_tgmr_t trace, double a,
                                      double amount)
{
  int quotient;
  double reduced_b;
  double reduced_amount;
  double reduced_a;
  double b;

/* Fix very small negative numbers */
  if ((a < 0.0) && (a > -EPSILON)) {
    a = 0.0;
  }
  if ((amount < 0.0) && (amount > -EPSILON)) {
    amount = 0.0;
  }

/* Sanity checks */
  if ((a < 0.0) || (amount < 0.0)) {
    XBT_CRITICAL
        ("Error, invalid parameters [a = %.2f, amount = %.2f]. You probably have a task executing with negative computation amount. Check your code.",
         a, amount);
    xbt_abort();
  }

/* At this point, a and amount are positive */

  if (amount < EPSILON)
    return a;

/* Is the trace fixed ? */
  if (trace->type == TRACE_FIXED) {
    return (a + (amount / trace->value));
  }

  XBT_DEBUG("amount %lf total %lf", amount, trace->total);
/* Reduce the problem to one where amount <= trace_total */
  quotient = (int) (floor(amount / trace->total));
  reduced_amount = (trace->total) * ((amount / trace->total) -
                                     floor(amount / trace->total));
  reduced_a = a - (trace->last_time) * (int) (floor(a / trace->last_time));

  XBT_DEBUG("Quotient: %d reduced_amount: %lf reduced_a: %lf", quotient,
         reduced_amount, reduced_a);

/* Now solve for new_amount which is <= trace_total */
/*
   fprintf(stderr,"reduced_a = %.2f\n",reduced_a);
   fprintf(stderr,"reduced_amount = %.2f\n",reduced_amount);
 */
  reduced_b =
      surf_cpu_ti_solve_trace_somewhat_simple(trace, reduced_a,
                                              reduced_amount);

/* Re-map to the original b and amount */
  b = (trace->last_time) * (int) (floor(a / trace->last_time)) +
      (quotient * trace->last_time) + reduced_b;
  return b;
}

/**
* \brief Auxiliary function to solve integral
*
* Here, amount is <= trace->total
* and a <=trace->last_time
*
*/
static double surf_cpu_ti_solve_trace_somewhat_simple(surf_cpu_ti_tgmr_t
                                                      trace, double a,
                                                      double amount)
{
  double amount_till_end;
  double b;

  XBT_DEBUG("Solve integral: [%.2f, amount=%.2f]", a, amount);
  amount_till_end =
      surf_cpu_ti_integrate_trace(trace, a, trace->last_time);
/*
   fprintf(stderr,"amount_till_end=%.2f\n",amount_till_end);
 */

  if (amount_till_end > amount) {
    b = surf_cpu_ti_solve_trace_simple(trace->trace, a, amount);
  } else {
    b = trace->last_time +
        surf_cpu_ti_solve_trace_simple(trace->trace, 0.0,
                                       amount - amount_till_end);
  }
  return b;
}

/**
 * \brief Auxiliary function to solve integral.
 *  It returns the date when the requested amount of flops is available
 * \param trace    Trace structure
 * \param a        Initial point
 * \param amount  Amount of flops 
 * \return The date when amount is available.
*/
static double surf_cpu_ti_solve_trace_simple(surf_cpu_ti_trace_t trace,
                                             double a, double amount)
{
  double integral_a;
  int ind;
  double time;
  integral_a = surf_cpu_ti_integrate_trace_simple_point(trace, a);
  ind =
      surf_cpu_ti_binary_search(trace->integral, integral_a + amount, 0,
                                trace->nb_points - 1);
  time = trace->time_points[ind];
  time +=
      (integral_a + amount -
       trace->integral[ind]) / ((trace->integral[ind + 1] -
                                 trace->integral[ind]) /
                                (trace->time_points[ind + 1] -
                                 trace->time_points[ind]));

  return time;
}

/**
* \brief Creates a new integration trace from a tmgr_trace_t
*
* \param  power_trace    CPU availability trace
* \param  value          Percentage of CPU power available (useful to fixed tracing)
* \param  spacing        Initial spacing
* \return  Integration trace structure
*/
static surf_cpu_ti_tgmr_t cpu_ti_parse_trace(tmgr_trace_t power_trace,
                                             double value)
{
  surf_cpu_ti_tgmr_t trace;
  double total_time = 0.0;
  s_tmgr_event_t val;
  unsigned int cpt;
  trace = xbt_new0(s_surf_cpu_ti_tgmr_t, 1);

/* no availability file, fixed trace */
  if (!power_trace) {
    trace->type = TRACE_FIXED;
    trace->value = value;
    XBT_DEBUG("No availabily trace. Constant value = %lf", value);
    return trace;
  }

  /* only one point available, fixed trace */
  if (xbt_dynar_length(power_trace->s_list.event_list) == 1) {
    xbt_dynar_get_cpy(power_trace->s_list.event_list, 0, &val);
    trace->type = TRACE_FIXED;
    trace->value = val.value;
    return trace;
  }

  trace->type = TRACE_DYNAMIC;
  trace->power_trace = power_trace;

  /* count the total time of trace file */
  xbt_dynar_foreach(power_trace->s_list.event_list, cpt, val) {
    total_time += val.delta;
  }
  trace->trace = surf_cpu_ti_trace_new(power_trace);
  trace->last_time = total_time;
  trace->total =
      surf_cpu_ti_integrate_trace_simple(trace->trace, 0, total_time);

  XBT_DEBUG("Total integral %lf, last_time %lf ",
         trace->total, trace->last_time);

  return trace;
}

/**
 * \brief Binary search in array.
 *  It returns the first point of the interval in which "a" is. 
 * \param array    Array
 * \param a        Value to search
 * \param low     Low bound to search in array
 * \param high    Upper bound to search in array
 * \return Index of point
*/
static int surf_cpu_ti_binary_search(double *array, double a, int low,
                                     int high)
{
  xbt_assert(low < high, "Wrong parameters: low (%d) should be smaller than"
      " high (%d)", low, high);

  int mid;
  do {
    mid = low + (high - low) / 2;
    XBT_DEBUG("a %lf low %d high %d mid %d value %lf", a, low, high, mid,
        array[mid]);

    if (array[mid] > a)
      high = mid;
    else
      low = mid;
  }
  while (low < high - 1);

  return low;
}

/*********
 * Model *
 *********/

CpuTiModel::CpuTiModel() : CpuModel("cpu_ti")
{
  
}

CpuTiPtr CpuTiModel::createResource(string name,
	                   double powerPeak,
                           double powerScale,
                           tmgr_trace_t powerTrace,
                           int core,
                           e_surf_resource_state_t stateInitial,
                           tmgr_trace_t stateTrace,
                           xbt_dict_t cpuProperties)
{
  tmgr_trace_t empty_trace;
  s_tmgr_event_t val;
  CpuTiActionPtr cpuAction;
  xbt_assert(core==1,"Multi-core not handled with this model yet");
  /*TOREPAIR: xbt_assert(!surf_cpu_resource_priv(surf_cpu_resource_by_name(name)),
              "Host '%s' declared several times in the platform file",
              name);*/
  CpuTiPtr cpu = new CpuTi(this, name, powerPeak, powerScale, powerTrace,
		           core, stateInitial, stateTrace, cpuProperties);
  xbt_lib_set(host_lib, name.c_str(), SURF_CPU_LEVEL, cpu);
  return (CpuTiPtr) xbt_lib_get_elm_or_null(host_lib, name.c_str());
}

CpuTiActionPtr CpuTiModel::createAction(double cost, bool failed)
{
  return NULL;//new CpuTiAction(this, cost, failed);
}

void CpuTi::printCpuTiModel()
{
  std::cout << getModel()->getName() << "<<plop"<< std::endl;
};

/************
 * Resource *
 ************/
CpuTi::CpuTi(CpuTiModelPtr model, string name, double powerPeak,
        double powerScale, tmgr_trace_t powerTrace, int core,
        e_surf_resource_state_t stateInitial, tmgr_trace_t stateTrace,
	xbt_dict_t properties) :
	Cpu(model, name, properties), m_powerPeak(powerPeak), m_powerScale(powerScale),
        m_stateCurrent(stateInitial) {
  tmgr_trace_t empty_trace;		
  s_tmgr_event_t val;		
  xbt_assert(core==1,"Multi-core not handled with this model yet");
  XBT_DEBUG("power scale %lf", powerScale);
  m_availTrace = cpu_ti_parse_trace(powerTrace, powerScale);
  if (stateTrace)
    /*TOREPAIR:m_stateEvent = tmgr_history_add_trace(history, stateTrace, 0.0, 0, this);*/
  if (powerTrace && xbt_dynar_length(powerTrace->s_list.event_list) > 1) {
    // add a fake trace event if periodicity == 0 
    xbt_dynar_get_cpy(powerTrace->s_list.event_list,
                      xbt_dynar_length(powerTrace->s_list.event_list) - 1, &val);
    /*TOREPAIR:if (val.delta == 0) {
      empty_trace = tmgr_empty_trace_new();
       m_powerEvent =
        tmgr_history_add_trace(history, empty_trace,
                               m_availTrace->last_time, 0, this);
    }*/
  }
};

CpuTiModelPtr CpuTi::getModel() {
  return static_cast<CpuTiModelPtr>(p_model);
}; 

double CpuTi::getSpeed(double load)
{
  return load * m_powerPeak;
}

double CpuTi::getAvailableSpeed()
{
  return 0;
}

/**********
 * Action *
 **********/

double CpuTiAction::getRemains()
{
  return 0;
}

double CpuTiAction::getStartTime()
{
  return 0;
}

double CpuTiAction::getFinishTime()
{
  return 0;
}

#endif /* SURF_MODEL_CPUTI_H_ */

