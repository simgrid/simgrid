/* time - time related syscal wrappers                                      */

/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <math.h>               /* floor */

#include "portable.h"

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "gras/virtu.h"
#include "xbt/xbt_os_time.h"    /* private */

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(gras_virtu);
double xbt_time(void)
{
  return xbt_os_time();
}

void xbt_sleep(double sec)
{
  xbt_os_sleep(sec);
}

/* This horribly badly placed. I just need to get that symbol with that value into libgras so that the mallocators are happy.
 * EPR will save us all. One day.
 */
int _surf_do_model_check = 0;
