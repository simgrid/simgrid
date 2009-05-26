/* $Id$ */
/*  xbt/asserts.h -- assertion mechanism                                     */

/* Copyright (c) 2004,2005 Martin Quinson. All rights reserved.             */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdlib.h>             /* abort */
#include "xbt/log.h"
#include "xbt/asserts.h"

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
 * If you want to pass arguments to the format, you can always write xbt_assert1(0,"fmt",args) or
 * xbt_die(bprintf("fmt", arg))
 */
void xbt_die(const char *msg)
{
  CRITICAL1("%s", msg);
  xbt_abort();
}

/** @brief Kill the program in silence */
void xbt_abort(void)
{
  abort();
}
