/* $Id$ */

/* error - Error handling functions                                         */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2001,2002,2003,2004 the OURAGAN project.                   */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/error.h"

/**
 * \brief Usefull to do nice error repporting messages.
 * \ingroup XBT_error
 * \param errcode 
 * \return the printable name of an error code
 *
 */
const char *xbt_error_name(xbt_error_t errcode)  {

   switch (errcode) {
      
    case no_error: return "success";
    case mismatch_error: return "mismatch";
    case system_error: return "system";
    case network_error: return "network";
    case timeout_error: return "timeout";
    case thread_error: return "thread";
    case unknown_error: return "unclassified";
    default:
      return "SEVERE ERROR in error repporting module";
   }
}

XBT_LOG_EXTERNAL_CATEGORY(xbt);
XBT_LOG_DEFAULT_CATEGORY(xbt);
  
/**
 * \ingroup XBT_error
 * \param msg 
 *
 * Things are so messed up that the only thing to do now, is to stop the program.
 */
void xbt_die (const char *msg) {
   CRITICAL1("%s",msg);
   xbt_abort();
}

