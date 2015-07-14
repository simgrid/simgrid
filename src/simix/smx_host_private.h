/* Copyright (c) 2007-2010, 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_HOST_PRIVATE_H
#define _SIMIX_HOST_PRIVATE_H

#include "simgrid/simix.h"
#include "popping_private.h"

SG_BEGIN_DECL()

/** @brief Host datatype from SIMIX POV */
typedef struct s_smx_host_priv {
  xbt_swag_t process_list;
  xbt_dynar_t auto_restart_processes;
  xbt_dynar_t boot_processes;
} s_smx_host_priv_t;

void _SIMIX_host_free_process_arg(void *);
void SIMIX_host_create(const char *name);
void SIMIX_host_destroy(void *host);

void SIMIX_host_add_auto_restart_process(sg_host_t host,
                                         const char *name,
                                         xbt_main_func_t code,
                                         void *data,
                                         const char *hostname,
                                         double kill_time,
                                         int argc, char **argv,
                                         xbt_dict_t properties,
                                         int auto_restart);

void SIMIX_host_restart_processes(sg_host_t host);
void SIMIX_host_autorestart(sg_host_t host);
xbt_dict_t SIMIX_host_get_properties(sg_host_t host);
int SIMIX_host_get_core(sg_host_t host);
xbt_swag_t SIMIX_host_get_process_list(sg_host_t host);
double SIMIX_host_get_speed(sg_host_t host);
double SIMIX_host_get_available_speed(sg_host_t host);
int SIMIX_host_get_state(sg_host_t host);
void SIMIX_host_on(sg_host_t host);
void SIMIX_host_off(sg_host_t host, smx_process_t issuer);
double SIMIX_host_get_current_power_peak(sg_host_t host);
double SIMIX_host_get_power_peak_at(sg_host_t host, int pstate_index);
int SIMIX_host_get_nb_pstates(sg_host_t host);
double SIMIX_host_get_consumed_energy(sg_host_t host);
double SIMIX_host_get_wattmin_at(sg_host_t host,int pstate);
double SIMIX_host_get_wattmax_at(sg_host_t host,int pstate);
void SIMIX_host_set_pstate(sg_host_t host, int pstate_index);
int SIMIX_host_get_pstate(sg_host_t host);
smx_synchro_t SIMIX_host_execute(const char *name,
    sg_host_t host, double flops_amount, double priority, double bound, unsigned long affinity_mask);
smx_synchro_t SIMIX_host_parallel_execute(const char *name,
    int host_nb, sg_host_t *host_list,
    double *flops_amount, double *bytes_amount,
    double amount, double rate);
void SIMIX_host_execution_destroy(smx_synchro_t synchro);
void SIMIX_host_execution_cancel(smx_synchro_t synchro);
double SIMIX_host_execution_get_remains(smx_synchro_t synchro);
e_smx_state_t SIMIX_host_execution_get_state(smx_synchro_t synchro);
void SIMIX_host_execution_set_priority(smx_synchro_t synchro, double priority);
void SIMIX_host_execution_set_bound(smx_synchro_t synchro, double bound);
void SIMIX_host_execution_set_affinity(smx_synchro_t synchro, sg_host_t host, unsigned long mask);
xbt_dict_t SIMIX_host_get_mounted_storage_list(sg_host_t host);
xbt_dynar_t SIMIX_host_get_attached_storage_list(sg_host_t host);

void SIMIX_host_execution_suspend(smx_synchro_t synchro);
void SIMIX_host_execution_resume(smx_synchro_t synchro);

void SIMIX_post_host_execute(smx_synchro_t synchro);
void SIMIX_set_category(smx_synchro_t synchro, const char *category);

/* vm related stuff */
sg_host_t SIMIX_vm_create(const char *name, sg_host_t ind_phys_host);

void SIMIX_vm_destroy(sg_host_t ind_vm);
// --
void SIMIX_vm_resume(sg_host_t ind_vm, smx_process_t issuer);

void SIMIX_vm_suspend(sg_host_t ind_vm, smx_process_t issuer);
// --
void SIMIX_vm_save(sg_host_t ind_vm, smx_process_t issuer);

void SIMIX_vm_restore(sg_host_t ind_vm, smx_process_t issuer);
// --
void SIMIX_vm_start(sg_host_t ind_vm);

void SIMIX_vm_shutdown(sg_host_t ind_vm, smx_process_t issuer);
// --

int SIMIX_vm_get_state(sg_host_t ind_vm);
// --
void SIMIX_vm_migrate(sg_host_t ind_vm, sg_host_t ind_dst_pm);

void *SIMIX_vm_get_pm(sg_host_t ind_vm);

void SIMIX_vm_set_bound(sg_host_t ind_vm, double bound);

void SIMIX_vm_set_affinity(sg_host_t ind_vm, sg_host_t ind_pm, unsigned long mask);

void SIMIX_vm_migratefrom_resumeto(sg_host_t vm, sg_host_t src_pm, sg_host_t dst_pm);

void SIMIX_host_get_params(sg_host_t ind_vm, ws_params_t params);

void SIMIX_host_set_params(sg_host_t ind_vm, ws_params_t params);

SG_END_DECL()

#endif

