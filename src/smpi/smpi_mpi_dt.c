/* $Id: $tag */

/* smpi_mpi_dt.c -- MPI primitives to handle datatypes                        */
 
/* Note: a very incomplete implementation                                     */

/* Copyright (c) 2009 Stephane Genaud.                                        */
/* All rights reserved.                                                       */

/* This program is free software; you can redistribute it and/or modify it
 *  * under the terms of the license (GNU LGPL) which comes with this package. */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "private.h"
#include "smpi_mpi_dt_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_mpi_dt, smpi,
                                "Logging specific to SMPI (datatype)");


/**
 * Get the lower bound and extent for a Datatype 
 * The  extent of a datatype is defined to be the span from the first byte to the last byte 
 * occupied by entries in this datatype, rounded up to satisfy alignment requirements (epsilon).
 *
 * For typemap T = {(t_0,disp_0), ...,  (t_n-1,disp_n-1)}
 * lb(T)     = min_j disp_j
 * ub(T)     = max_j (disp_j+sizeof(t_j)) + epsilon
 * extent(T) = ub(T) - lb(T)
 *
 * FIXME: this an incomplete implementation as we do not support yet MPI_Type_commit.
 * Hence, this can be called only for primitive type MPI_INT, MPI_DOUBLE, ...
 *
 * remark: MPI-1 has also the deprecated 
 * int MPI_Type_extent(MPI_Datatype datatype, *MPI_Aint *extent);
 *
 **/
int smpi_mpi_type_get_extent(MPI_Datatype datatype, MPI_Aint *lb, MPI_Aint *extent) {
        
        if ( DT_FLAG_COMMITED != (datatype-> flags & DT_FLAG_COMMITED) )
                return( MPI_ERR_TYPE );
        *lb =  datatype->lb;
        *extent =  datatype->ub - datatype->lb;
        return( MPI_SUCCESS );
}
/**
 * query the size of the type
 **/
int SMPI_MPI_Type_size(MPI_Datatype datatype, size_t * size)
{
  int retval = MPI_SUCCESS;

  smpi_bench_end();

  if (NULL == datatype) {
    retval = MPI_ERR_TYPE;
  } else if (NULL == size) {
    retval = MPI_ERR_ARG;
  } else {
    *size = datatype->size;
  }

  smpi_bench_begin();

  return retval;
}



/**
 * query extent and lower bound of the type 
 **/
int SMPI_MPI_Type_get_extent( MPI_Datatype datatype, int *lb, int *extent) 
{
        return( smpi_mpi_type_get_extent( datatype, lb, extent));
}

/* Deprecated Functions. 
 * The MPI-2 standard deprecated a number of routines because MPI-2 provides better versions. 
 * This routine is one of those that was deprecated. The routine may continue to be used, but 
 * new code should use the replacement routine. The replacement for this routine is MPI_Type_Get_extent.
 **/
int SMPI_MPI_Type_ub( MPI_Datatype datatype, MPI_Aint *displacement) 
{
        if ( DT_FLAG_COMMITED != (datatype->flags & DT_FLAG_COMMITED) )
                return( MPI_ERR_TYPE );
        *displacement = datatype->ub;
        return( MPI_SUCCESS );
}
int SMPI_MPI_Type_lb( MPI_Datatype datatype, MPI_Aint *displacement) 
{
        if ( DT_FLAG_COMMITED != (datatype->flags & DT_FLAG_COMMITED) )
                return( MPI_ERR_TYPE );
        *displacement = datatype->lb;
        return( MPI_SUCCESS );
}
