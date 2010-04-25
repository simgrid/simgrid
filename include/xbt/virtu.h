/* virtu - virtualization layer for XBT to choose between GRAS and MSG implementation */

/*  Copyright (c) 2007 Martin Quinson                                       */
/*  All rights reserved.                                                    */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef __XBT_VIRTU_H__
#define __XBT_VIRTU_H__

#include "xbt/misc.h"
#include "xbt/sysdep.h"
#include "xbt/function_types.h"

SG_BEGIN_DECL()

  /* Get the PID of the current process */
  XBT_PUBLIC_DATA(int_f_void_t) xbt_getpid;

SG_END_DECL()
#endif /* __XBT_VIRTU_H__ */
