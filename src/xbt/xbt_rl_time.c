/* time - time related syscal wrappers                                      */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

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
