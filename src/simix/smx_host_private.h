/* Copyright (c) 2007-2010, 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_HOST_PRIVATE_H
#define _SIMIX_HOST_PRIVATE_H

#include "simgrid/simix.h"
#include "popping_private.h"

SG_BEGIN_DECL()

/** @brief Host datatype */
typedef struct s_smx_host_priv {
  xbt_swag_t process_list;
  xbt_dynar_t auto_restart_processes;
  xbt_dynar_t boot_processes; 
} s_smx_host_priv_t;

static inline smx_host_priv_t SIMIX_host_priv(smx_host_t host){
  return (smx_host_priv_t) xbt_lib_get_level(host, SIMIX_HOST_LEVEL);
}

void _SIMIX_host_free_process_arg(void *);
smx_host_t SIMIX_host_create(const char *name, void *data);
void SIMIX_host_destroy(void *host);

void SIMIX_host_add_auto_restart_process(smx_host_t host,
                                         const char *name,
                                         xbt_main_func_t code,
                                         void *data,
                                         const char *hostname,
                                         double kill_time,
                                         int argc, char **argv,
                                         xbt_dict_t properties,
                                         int auto_restart);

void SIMIX_host_restart_processes(smx_host_t host);
void SIMIX_host_autorestart(smx_host_t host);
xbt_dict_t SIMIX_host_get_properties(smx_host_t host);
int SIMIX_host_get_core(smx_host_t host);
xbt_swag_t SIMIX_host_get_process_list(smx_host_t host);
double SIMIX_host_get_speed(smx_host_t host);
double SIMIX_host_get_available_speed(smx_host_t host);
int SIMIX_host_get_state(smx_host_t host);
void SIMIX_host_on(smx_host_t host);
void SIMIX_host_off(smx_host_t host, smx_process_t issuer);
double SIMIX_host_get_current_power_peak(smx_host_t host);
double SIMIX_host_get_power_peak_at(smx_host_t host, int pstate_index);
int SIMIX_host_get_nb_pstates(smx_host_t host);
double SIMIX_host_get_consumed_energy(smx_host_t host);
double SIMIX_host_get_wattmin_at(smx_host_t host,int pstate);
double SIMIX_host_get_wattmax_at(smx_host_t host,int pstate);
void SIMIX_host_set_pstate(smx_host_t host, int pstate_index);
int SIMIX_host_get_pstate(smx_host_t host);
smx_synchro_t SIMIX_host_execute(const char *name,
    smx_host_t host, double flops_amount, double priority, double bound, unsigned long affinity_mask);
smx_synchro_t SIMIX_host_parallel_execute(const char *name,
    int host_nb, smx_host_t *host_list,
    double *flops_amount, double *bytes_amount,
    double amount, double rate);
void SIMIX_host_execution_destroy(smx_synchro_t synchro);
void SIMIX_host_execution_cancel(smx_synchro_t synchro);
double SIMIX_host_execution_get_remains(smx_synchro_t synchro);
e_smx_state_t SIMIX_host_execution_get_state(smx_synchro_t synchro);
void SIMIX_host_execution_set_priority(smx_synchro_t synchro, double priority);
void SIMIX_host_execution_set_bound(smx_synchro_t synchro, double bound);
void SIMIX_host_execution_set_affinity(smx_synchro_t synchro, smx_host_t host, unsigned long mask);
xbt_dict_t SIMIX_host_get_mounted_storage_list(smx_host_t host);
xbt_dynar_t SIMIX_host_get_attached_storage_list(smx_host_t host);

void SIMIX_host_execution_suspend(smx_synchro_t synchro);
void SIMIX_host_execution_resume(smx_synchro_t synchro);

void SIMIX_post_host_execute(smx_synchro_t synchro);
void SIMIX_set_category(smx_synchro_t synchro, const char *category);

/* vm related stuff */
smx_host_t SIMIX_vm_create(const char *name, smx_host_t ind_phys_host);

void SIMIX_vm_destroy(smx_host_t ind_vm);
// --
void SIMIX_vm_resume(smx_host_t ind_vm, smx_process_t issuer);

void SIMIX_vm_suspend(smx_host_t ind_vm, smx_process_t issuer);
// --
void SIMIX_vm_save(smx_host_t ind_vm, smx_process_t issuer);

void SIMIX_vm_restore(smx_host_t ind_vm, smx_process_t issuer);
// --
void SIMIX_vm_start(smx_host_t ind_vm);

void SIMIX_vm_shutdown(smx_host_t ind_vm, smx_process_t issuer);
// --

int SIMIX_vm_get_state(smx_host_t ind_vm);
// --
void SIMIX_vm_migrate(smx_host_t ind_vm, smx_host_t ind_dst_pm);

void *SIMIX_vm_get_pm(smx_host_t ind_vm);

void SIMIX_vm_set_bound(smx_host_t ind_vm, double bound);

void SIMIX_vm_set_affinity(smx_host_t ind_vm, smx_host_t ind_pm, unsigned long mask);

void SIMIX_vm_migratefrom_resumeto(smx_host_t vm, smx_host_t src_pm, smx_host_t dst_pm);

void SIMIX_host_get_params(smx_host_t ind_vm, ws_params_t params);

void SIMIX_host_set_params(smx_host_t ind_vm, ws_params_t params);

SG_END_DECL()

#endif

