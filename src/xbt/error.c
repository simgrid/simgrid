/* $Id$ */

/* error - Error handling functions                                         */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2001,2002,2003,2004 the OURAGAN project.                   */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras_private.h"

/**
 * gras_error_name:
 * @errcode: 
 * @Returns: the printable name of an error code
 *
 * usefull to do nice error repporting messages
 */

const char *gras_error_name(gras_error_t errcode)  {

   switch (errcode) {
      
    case no_error: return "success";
    case malloc_error: return "malloc";
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

