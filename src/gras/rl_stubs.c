/* $Id$ */

/* rl_stubs.c -- empty body of functions used in SG, but harmful in RL       */

/* Copyright (c) 2007 Martin Quinson.                                       */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt_modinter.h"
#include "xbt/sysdep.h"
#include "simix/simix.h"

XBT_LOG_EXTERNAL_CATEGORY(xbt);
XBT_LOG_EXTERNAL_CATEGORY(xbt_sync_rl);
XBT_LOG_EXTERNAL_CATEGORY(gras_trp);
XBT_LOG_EXTERNAL_CATEGORY(gras_trp_file);
XBT_LOG_EXTERNAL_CATEGORY(gras_trp_tcp);

/*void xbt_context_mod_init(void)
{
  XBT_LOG_CONNECT(xbt_sync_rl, xbt);
  XBT_LOG_CONNECT(gras_trp_file, gras_trp);
  XBT_LOG_CONNECT(gras_trp_tcp, gras_trp);
}*/

/*void xbt_context_mod_exit(void)
{
}*/

void SIMIX_display_process_status(void)
{
}
