/* $Id$ */

/* xbt_synchro -- Synchronization virtualized depending on whether we are   */
/*                in simulation or real life (act on simulated processes)   */

/* This is the simulation implementation, using simix.                      */

/* Copyright 2006,2007 Malek Cherier, Martin Quinson          
 * All right reserved.                                                      */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"

#include "xbt/synchro.h"     /* This module */

#include "simix/simix.h"     /* used implementation */
#include "simix/datatypes.h"

/* the implementation would be cleaner (and faster) with ELF symbol aliasing */

typedef struct s_xbt_thread_ {
   /* KEEP IT IN SYNC WITH s_smx_process_ from src/include/simix/datatypes.h */
   char *name;			/**< @brief process name if any */
   smx_simdata_process_t simdata;	/**< @brief simulator data */
   s_xbt_swag_hookup_t process_hookup;
   s_xbt_swag_hookup_t synchro_hookup;
   s_xbt_swag_hookup_t host_proc_hookup;
   void *data;			/**< @brief user data */
   /* KEEP IT IN SYNC WITH s_smx_process_ from src/include/simix/datatypes.h */
} s_xbt_thread_t;

xbt_thread_t xbt_thread_create(pvoid_f_pvoid_t start_routine, void* param)  {
   THROW_UNIMPLEMENTED; /* FIXME */
}

void 
xbt_thread_join(xbt_thread_t thread,void ** thread_return) {
   THROW_UNIMPLEMENTED; /* FIXME */
}		       

void xbt_thread_exit(int *retval) {
   THROW_UNIMPLEMENTED; /* FIXME */
}

xbt_thread_t xbt_thread_self(void) {
   THROW_UNIMPLEMENTED; /* FIXME */   
}

void xbt_thread_yield(void) {
   THROW_UNIMPLEMENTED; /* FIXME */   
}
/****** mutex related functions ******/
struct s_xbt_mutex_ {
   
   /* KEEP IT IN SYNC WITH src/simix/private.h::struct s_smx_mutex */
   xbt_swag_t sleeping;			/* list of sleeping process */
   int using;
   /* KEEP IT IN SYNC WITH src/simix/private.h::struct s_smx_mutex */
   
};

xbt_mutex_t xbt_mutex_init(void) {
   return (xbt_mutex_t)SIMIX_mutex_init();
}

void xbt_mutex_lock(xbt_mutex_t mutex) {
   SIMIX_mutex_lock( (smx_mutex_t)mutex) ;
}

void xbt_mutex_unlock(xbt_mutex_t mutex) {
   SIMIX_mutex_unlock( (smx_mutex_t)mutex );
}

void xbt_mutex_destroy(xbt_mutex_t mutex) {
   SIMIX_mutex_destroy( (smx_mutex_t)mutex );
}

/***** condition related functions *****/
struct s_xbt_cond_ {
   
   /* KEEP IT IN SYNC WITH src/simix/private.h::struct s_smx_cond */
   xbt_swag_t sleeping; 			/* list of sleeping process */
   smx_mutex_t  mutex;
   xbt_fifo_t actions;			/* list of actions */
   /* KEEP IT IN SYNC WITH src/simix/private.h::struct s_smx_cond */

};

xbt_cond_t xbt_cond_init(void) {
   return (xbt_cond_t)SIMIX_cond_init();
}

void xbt_cond_wait(xbt_cond_t cond, xbt_mutex_t mutex) {
   SIMIX_cond_wait( (smx_cond_t)cond , (smx_mutex_t)mutex );
}

void xbt_cond_signal(xbt_cond_t cond) {
   SIMIX_cond_signal( (smx_cond_t)cond );
}
	 
void xbt_cond_broadcast(xbt_cond_t cond){
   SIMIX_cond_broadcast( (smx_cond_t)cond );
}
void xbt_cond_destroy(xbt_cond_t cond){
   SIMIX_cond_destroy( (smx_cond_t)cond );
}
