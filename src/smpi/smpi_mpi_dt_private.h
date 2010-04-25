/* smpi_mpi_dt_private.h -- functions of smpi_mpi_dt.c that are exported to other SMPI modules. */

/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */


#include "private.h"

#define DT_FLAG_DESTROYED     0x0001  /**< user destroyed but some other layers still have a reference */
#define DT_FLAG_COMMITED      0x0002  /**< ready to be used for a send/recv operation */
#define DT_FLAG_CONTIGUOUS    0x0004  /**< contiguous datatype */
#define DT_FLAG_OVERLAP       0x0008  /**< datatype is unpropper for a recv operation */
#define DT_FLAG_USER_LB       0x0010  /**< has a user defined LB */
#define DT_FLAG_USER_UB       0x0020  /**< has a user defined UB */
#define DT_FLAG_PREDEFINED    0x0040  /**< cannot be removed: initial and predefined datatypes */
#define DT_FLAG_NO_GAPS       0x0080  /**< no gaps around the datatype */
#define DT_FLAG_DATA          0x0100  /**< data or control structure */
#define DT_FLAG_ONE_SIDED     0x0200  /**< datatype can be used for one sided operations */
#define DT_FLAG_UNAVAILABLE   0x0400  /**< datatypes unavailable on the build (OS or compiler dependant) */
#define DT_FLAG_VECTOR        0x0800  /**< valid only for loops. The loop contain only one element
                                       **< without extent. It correspond to the vector type. */
/*
 * We should make the difference here between the predefined contiguous and non contiguous
 * datatypes. The DT_FLAG_BASIC is held by all predefined contiguous datatypes.
 */
#define DT_FLAG_BASIC         (DT_FLAG_PREDEFINED | DT_FLAG_CONTIGUOUS | DT_FLAG_NO_GAPS | DT_FLAG_DATA | DT_FLAG_COMMITED)
