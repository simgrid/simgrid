/* Copyright (c) 2007-2010, 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_SMURF_PRIVATE_H
#define _SIMIX_SMURF_PRIVATE_H

SG_BEGIN_DECL()

/********************************* Simcalls *********************************/

/* we want to build the e_smx_simcall_t enumeration, the table of the
 * corresponding simcalls string names, and the simcall handlers table
 * automatically, using macros.
 * To add a new simcall follow the following syntax:
 *
 * */

#include "simcalls_generated_enum.h" /* All possible simcalls (generated) */

typedef int (*simix_match_func_t)(void *, void *, smx_action_t);
typedef void (*simix_copy_data_func_t)(smx_action_t, void*, size_t);
typedef void (*simix_clean_func_t)(void *);
typedef void (*FPtr)(void); // Hide the ugliness

/* Pack all possible scalar types in an union */
union u_smx_scalar {
  char            c;
  const char*     cc;
  short           s;
  int             i;
  long            l;
  unsigned char   uc;
  unsigned short  us;
  unsigned int    ui;
  unsigned long   ul;
  float           f;
  double          d;
  size_t          sz;
  sg_size_t       sgsz;
  sg_offset_t     sgoff;
  void*           dp;
  FPtr            fp;
  const void*     cp;
};

/**
 * \brief Represents a simcall to the kernel.
 */
typedef struct s_smx_simcall {
  e_smx_simcall_t call;
  smx_process_t issuer;
#ifdef HAVE_MC
  int mc_value;
#endif
  union u_smx_scalar args[11];
  union u_smx_scalar result;
  //FIXME: union u_smx_scalar retval;
  union {
    struct {
      const char* param1;
      double param2;
      int result;
    } new_api;

  };
} s_smx_simcall_t, *smx_simcall_t;

#if HAVE_MC
#define SIMCALL_SET_MC_VALUE(simcall, value) ((simcall)->mc_value = (value))
#define SIMCALL_GET_MC_VALUE(simcall) ((simcall)->mc_value)
#else
#define SIMCALL_SET_MC_VALUE(simcall, value) ((void)0)
#define SIMCALL_GET_MC_VALUE(simcall) 0
#endif

#include "simcalls_generated_res_getter_setter.h"
#include "simcalls_generated_args_getter_setter.h"

/******************************** General *************************************/

void SIMIX_simcall_push(smx_process_t self);
void SIMIX_simcall_answer(smx_simcall_t);
void SIMIX_simcall_enter(smx_simcall_t, int);
void SIMIX_simcall_exit(smx_action_t);
smx_simcall_t SIMIX_simcall_mine(void);
const char *SIMIX_simcall_name(e_smx_simcall_t kind);
//TOFIX put it in a better place
xbt_dict_t SIMIX_pre_asr_get_properties(smx_simcall_t simcall, const char *name);

/*************************** New simcall interface ****************************/

typedef smx_action_t (*simcall_handler_t)(u_smx_scalar_t *);

extern const char *simcall_types[];
extern simcall_handler_t simcall_table[];

SG_END_DECL()

#endif
