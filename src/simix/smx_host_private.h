/* Copyright (c) 2007-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_HOST_PRIVATE_H
#define _SIMIX_HOST_PRIVATE_H

#include <xbt/base.h>

#include "simgrid/simix.h"
#include "popping_private.h"

SG_BEGIN_DECL()

/** @brief Host datatype from SIMIX POV */
typedef struct s_smx_host_priv {
  xbt_swag_t process_list;
  xbt_dynar_t auto_restart_processes;
  xbt_dynar_t boot_processes;
} s_smx_host_priv_t;

XBT_PRIVATE void _SIMIX_host_free_process_arg(void *);
XBT_PRIVATE void SIMIX_host_create(const char *name);
XBT_PRIVATE void SIMIX_host_destroy(void *host);

XBT_PRIVATE void SIMIX_host_add_auto_restart_process(sg_host_t host,
                                         const char *name,
                                         xbt_main_func_t code,
                                         void *data,
                                         const char *hostname,
                                         double kill_time,
                                         int argc, char **argv,
                                         xbt_dict_t properties,
                                         int auto_restart);

XBT_PRIVATE void SIMIX_host_restart_processes(sg_host_t host);
XBT_PRIVATE void SIMIX_host_autorestart(sg_host_t host);
XBT_PRIVATE xbt_dict_t SIMIX_host_get_properties(sg_host_t host);
XBT_PRIVATE xbt_swag_t SIMIX_host_get_process_list(sg_host_t host);
XBT_PRIVATE double SIMIX_host_get_current_power_peak(sg_host_t host);
XBT_PRIVATE double SIMIX_host_get_power_peak_at(sg_host_t host, int pstate_index);
XBT_PRIVATE double SIMIX_host_get_wattmin_at(sg_host_t host,int pstate);
XBT_PRIVATE double SIMIX_host_get_wattmax_at(sg_host_t host,int pstate);
XBT_PRIVATE void SIMIX_host_set_pstate(sg_host_t host, int pstate_index);
XBT_PRIVATE smx_synchro_t SIMIX_process_execute(smx_process_t issuer, const char *name,
    double flops_amount, double priority, double bound, unsigned long affinity_mask);
XBT_PRIVATE smx_synchro_t SIMIX_process_parallel_execute(const char *name,
    int host_nb, sg_host_t *host_list,
    double *flops_amount, double *bytes_amount,
    double amount, double rate);
XBT_PRIVATE void SIMIX_process_execution_destroy(smx_synchro_t synchro);
XBT_PRIVATE void SIMIX_process_execution_cancel(smx_synchro_t synchro);
XBT_PRIVATE double SIMIX_process_execution_get_remains(smx_synchro_t synchro);
XBT_PRIVATE e_smx_state_t SIMIX_process_execution_get_state(smx_synchro_t synchro);
XBT_PRIVATE void SIMIX_process_execution_set_priority(smx_synchro_t synchro, double priority);
XBT_PRIVATE void SIMIX_process_execution_set_bound(smx_synchro_t synchro, double bound);
XBT_PRIVATE void SIMIX_process_execution_set_affinity(smx_synchro_t synchro, sg_host_t host, unsigned long mask);
XBT_PRIVATE xbt_dynar_t SIMIX_host_get_attached_storage_list(sg_host_t host);

XBT_PRIVATE void SIMIX_host_execution_suspend(smx_synchro_t synchro);
XBT_PRIVATE void SIMIX_host_execution_resume(smx_synchro_t synchro);

XBT_PRIVATE void SIMIX_post_host_execute(smx_synchro_t synchro);
XBT_PRIVATE void SIMIX_set_category(smx_synchro_t synchro, const char *category);

/* vm related stuff */
XBT_PRIVATE sg_host_t SIMIX_vm_create(const char *name, sg_host_t ind_phys_host);

XBT_PRIVATE void SIMIX_vm_destroy(sg_host_t ind_vm);
// --
XBT_PRIVATE void SIMIX_vm_resume(sg_host_t ind_vm, smx_process_t issuer);

XBT_PRIVATE void SIMIX_vm_suspend(sg_host_t ind_vm, smx_process_t issuer);
// --
XBT_PRIVATE void SIMIX_vm_save(sg_host_t ind_vm, smx_process_t issuer);

XBT_PRIVATE void SIMIX_vm_restore(sg_host_t ind_vm, smx_process_t issuer);
// --
XBT_PRIVATE void SIMIX_vm_start(sg_host_t ind_vm);

XBT_PRIVATE void SIMIX_vm_shutdown(sg_host_t ind_vm, smx_process_t issuer);
// --

XBT_PRIVATE int SIMIX_vm_get_state(sg_host_t ind_vm);
// --
XBT_PRIVATE void SIMIX_vm_migrate(sg_host_t ind_vm, sg_host_t ind_dst_pm);

XBT_PRIVATE void *SIMIX_vm_get_pm(sg_host_t ind_vm);

XBT_PRIVATE void SIMIX_vm_set_bound(sg_host_t ind_vm, double bound);

XBT_PRIVATE void SIMIX_vm_set_affinity(sg_host_t ind_vm, sg_host_t ind_pm, unsigned long mask);

XBT_PRIVATE void SIMIX_vm_migratefrom_resumeto(sg_host_t vm, sg_host_t src_pm, sg_host_t dst_pm);

XBT_PRIVATE void SIMIX_host_get_params(sg_host_t ind_vm, vm_params_t params);

XBT_PRIVATE void SIMIX_host_set_params(sg_host_t ind_vm, vm_params_t params);

SG_END_DECL()

#endif

