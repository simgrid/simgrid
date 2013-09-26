/* Copyright (c) 2013. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg_private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_synchro, msg,
                                "Logging specific to MSG (synchro)");


/** @addtogroup msg_synchro
 *
 *  @{
 */

/********************************* Host **************************************/

/** @brief creates a semaphore object of the given initial capacity */
msg_sem_t MSG_sem_init(int initial_value) {
  return simcall_sem_init(initial_value);
}

/** @brief locks on a semaphore object */
void MSG_sem_acquire(msg_sem_t sem) {
  simcall_sem_acquire(sem);
}
/** @brief locks on a semaphore object up until the provided timeout expires */
msg_error_t MSG_sem_acquire_timeout(msg_sem_t sem, double timeout) {
  xbt_ex_t e;
  msg_error_t res = MSG_OK;
  TRY {
    simcall_sem_acquire_timeout(sem,timeout);
  } CATCH(e) {
    if (e.category == timeout_error) {
      res = MSG_TIMEOUT;
      xbt_ex_free(e);
    } else {
      RETHROW;
    }
  }
  return res;
}
/** @brief releases the semaphore object */
void MSG_sem_release(msg_sem_t sem) {
  simcall_sem_release(sem);
}
void MSG_sem_get_capacity(msg_sem_t sem) {
  simcall_sem_get_capacity(sem);
}

void MSG_sem_destroy(msg_sem_t sem) {
  simcall_sem_destroy(sem);
}
/** @brief returns a boolean indicating it this semaphore would block at this very specific time
 *
 * Note that the returned value may be wrong right after the function call, when you try to use it...
 * But that's a classical semaphore issue, and SimGrid's semaphore are not different to usual ones here.
 */
int MSG_sem_would_block(msg_sem_t sem) {
  return simcall_sem_would_block(sem);
}

/**@}*/
