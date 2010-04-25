/* base - several addons to do specific stuff not in GRAS itself            */

/* Copyright (c) 2006, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras.h"
#include "amok/amok_modinter.h"

XBT_LOG_NEW_SUBCATEGORY(amok, XBT_LOG_ROOT_CAT, "All AMOK categories");

void amok_init(void)
{

  /* Create all the modules */
  amok_pm_modulecreate();
}

void amok_exit(void)
{
  /* FIXME: No real module mechanism in GRAS so far, nothing to do. */
}
