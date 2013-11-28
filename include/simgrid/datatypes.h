/*
 * datatypes.h
 *
 *  Created on: Nov 28, 2013
 *      Author: bedaride
 */

#ifndef SIMGRID_DATATYPES_H_
#define SIMGRID_DATATYPES_H_

typedef struct ws_params {
  int ncpus;
  sg_size_t ramsize;
  int overcommit;

  /* The size of other states than memory pages, which is out-of-scope of dirty
   * page tracking. */
  long devsize;
  int skip_stage1;
  int skip_stage2;
  double max_downtime;

  double dp_rate;
  double dp_cap; /* bytes per 1 flop execution */

  double xfer_cpu_overhead;
  double dpt_cpu_overhead;

  /* set migration speed */
  double mig_speed;
} s_ws_params_t, *ws_params_t;

#endif /* SIMGRID_DATATYPES_H_ */
