/* error - Error handling functions                                         */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2001,2002,2003,2004 the OURAGAN project.                   */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"

/**
 * \brief Usefull to do nice error repporting messages.
 *
 * \param errcode
 * \return the printable name of an error code
 *
 */
const char *xbt_error_name(xbt_error_t errcode)  {

  switch (errcode) {

    case no_error: return "success";
    case old_mismatch_error: return "mismatch";
    case old_system_error: return "system";
    case old_network_error: return "network";
    case old_timeout_error: return "timeout";
    case old_thread_error: return "thread";
    case old_unknown_error: return "unclassified";
    default:
      return "SEVERE ERROR in error repporting module";
  }
}

XBT_LOG_EXTERNAL_CATEGORY(xbt);
XBT_LOG_DEFAULT_CATEGORY(xbt);

/**
 * @brief Kill the program with an error message
 * \param msg
 *
 * Things are so messed up that the only thing to do now, is to stop the program.
 *
 * The message is handled by a CRITICAL logging request
 *
 * If you want to pass arguments to the format, you can always write xbt_assert1(0,"fmt",args)
 */
void xbt_die (const char *msg) {
  CRITICAL1("%s",msg);
  xbt_abort();
}

/** @brief Kill the program in silence */
void xbt_abort(void) {
  abort();
}
