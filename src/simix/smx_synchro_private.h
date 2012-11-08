#ifndef _SIMIX_SYNCHRO_PRIVATE_H
#define _SIMIX_SYNCHRO_PRIVATE_H

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

void SIMIX_post_synchro(smx_action_t action);
void SIMIX_synchro_stop_waiting(smx_process_t process, smx_simcall_t simcall);
void SIMIX_synchro_destroy(smx_action_t action);

smx_mutex_t SIMIX_mutex_init(void);
void SIMIX_mutex_destroy(smx_mutex_t mutex);
void SIMIX_pre_mutex_lock(smx_simcall_t simcall, smx_mutex_t mutex);
int SIMIX_mutex_trylock(smx_mutex_t mutex, smx_process_t issuer);
void SIMIX_mutex_unlock(smx_mutex_t mutex, smx_process_t issuer);

smx_cond_t SIMIX_cond_init(void);
void SIMIX_cond_destroy(smx_cond_t cond);
void SIMIX_cond_signal(smx_cond_t cond);
void SIMIX_pre_cond_wait(smx_simcall_t simcall, smx_cond_t cond, smx_mutex_t mutex);
void SIMIX_pre_cond_wait_timeout(smx_simcall_t simcall, smx_cond_t cond,
		                 smx_mutex_t mutex, double timeout);
void SIMIX_cond_broadcast(smx_cond_t cond);

smx_sem_t SIMIX_sem_init(unsigned int value);
void SIMIX_sem_destroy(smx_sem_t sem);
void SIMIX_sem_release(smx_sem_t sem);
int SIMIX_sem_would_block(smx_sem_t sem);
void SIMIX_pre_sem_acquire(smx_simcall_t simcall, smx_sem_t sem);
void SIMIX_pre_sem_acquire_timeout(smx_simcall_t simcall, smx_sem_t sem, double timeout);
int SIMIX_sem_get_capacity(smx_sem_t sem);

// pre prototypes
smx_mutex_t SIMIX_pre_mutex_init(smx_simcall_t simcall);
void SIMIX_pre_mutex_destroy(smx_simcall_t simcall, smx_mutex_t mutex);
int SIMIX_pre_mutex_trylock(smx_simcall_t simcall, smx_mutex_t mutex);
void SIMIX_pre_mutex_unlock(smx_simcall_t simcall, smx_mutex_t mutex);
smx_cond_t SIMIX_pre_cond_init(smx_simcall_t simcall);
void SIMIX_pre_cond_destroy(smx_simcall_t simcall, smx_cond_t cond);
void SIMIX_pre_cond_signal(smx_simcall_t simcall, smx_cond_t cond);
void SIMIX_pre_cond_broadcast(smx_simcall_t simcall, smx_cond_t cond);
smx_sem_t SIMIX_pre_sem_init(smx_simcall_t simcall, unsigned int value);
void SIMIX_pre_sem_destroy(smx_simcall_t simcall, smx_sem_t sem);
void SIMIX_pre_sem_release(smx_simcall_t simcall, smx_sem_t sem);
XBT_INLINE int SIMIX_pre_sem_would_block(smx_simcall_t simcall, smx_sem_t sem);
int SIMIX_pre_sem_get_capacity(smx_simcall_t simcall, smx_sem_t sem);
#endif
