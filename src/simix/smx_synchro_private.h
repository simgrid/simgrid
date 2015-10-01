/* Copyright (c) 2012, 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_SYNCHRO_PRIVATE_H
#define _SIMIX_SYNCHRO_PRIVATE_H

#include "xbt/base.h"
#include "xbt/swag.h"
#include "xbt/xbt_os_thread.h"

typedef struct s_smx_mutex {
  unsigned int locked;
  smx_process_t owner;
  xbt_swag_t sleeping;          /* list of sleeping process */
} s_smx_mutex_t;

typedef struct s_smx_cond {
  smx_mutex_t mutex;
  xbt_swag_t sleeping;          /* list of sleeping process */
} s_smx_cond_t;

typedef struct s_smx_sem {
  unsigned int value;
  xbt_swag_t sleeping;          /* list of sleeping process */
} s_smx_sem_t;

XBT_PRIVATE void SIMIX_post_synchro(smx_synchro_t synchro);
XBT_PRIVATE void SIMIX_synchro_stop_waiting(smx_process_t process, smx_simcall_t simcall);
XBT_PRIVATE void SIMIX_synchro_destroy(smx_synchro_t synchro);

XBT_PRIVATE smx_mutex_t SIMIX_mutex_init(void);
XBT_PRIVATE void SIMIX_mutex_destroy(smx_mutex_t mutex);
XBT_PRIVATE int SIMIX_mutex_trylock(smx_mutex_t mutex, smx_process_t issuer);
XBT_PRIVATE void SIMIX_mutex_unlock(smx_mutex_t mutex, smx_process_t issuer);

XBT_PRIVATE smx_cond_t SIMIX_cond_init(void);
XBT_PRIVATE void SIMIX_cond_destroy(smx_cond_t cond);
XBT_PRIVATE void SIMIX_cond_broadcast(smx_cond_t cond);
XBT_PRIVATE void SIMIX_cond_signal(smx_cond_t cond);

XBT_PRIVATE void SIMIX_sem_destroy(smx_sem_t sem);
XBT_PRIVATE XBT_PRIVATE smx_sem_t SIMIX_sem_init(unsigned int value);
XBT_PRIVATE void SIMIX_sem_release(smx_sem_t sem);
XBT_PRIVATE int SIMIX_sem_would_block(smx_sem_t sem);
XBT_PRIVATE int SIMIX_sem_get_capacity(smx_sem_t sem);

#endif
