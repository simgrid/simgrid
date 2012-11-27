/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012. The SimGrid Team.
 * All rights reserved.                                                                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package.              */

#include "msg_private.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_new_API, msg,
                                "Logging specific to MSG (new_API)");


/* ****************************************************************************************** */
/* TUTORIAL: New API                                                                        */
/* All functions for the API                                                                  */
/* ****************************************************************************************** */
int MSG_new_API_fct(const char* param1, double param2)
{
  int result = simcall_new_api_fct(param1, param2);
  return result;
}
