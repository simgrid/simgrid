/* $Id$ */

/* virtu[alization] - speciafic parts for each OS and for SG                */

/* module's private interface.                                              */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#ifndef GRAS_VIRTU_PRIVATE_H
#define GRAS_VIRTU_PRIVATE_H

#include "gras/Virtu/virtu_interface.h"

/** @brief Data for each process */
typedef struct {
  /* globals of the process */
  void *userdata;

  /* data specific to each process for each module. 
     Registered with gras_procdata_add(), retrieved with gras_libdata_get() */
  xbt_dict_t libdata;
} gras_procdata_t;

gras_procdata_t *gras_procdata_get(void);

#endif  /* GRAS_VIRTU_PRIVATE_H */
