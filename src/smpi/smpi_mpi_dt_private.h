/**
 * $Id: $tag 
 *
 * smpi_mpi_dt_private.h -- functions of smpi_mpi_dt.c that are exported to other SMPI modules.
 *
 **/
#include "private.h"

/* flags for the datatypes. */
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



int smpi_mpi_type_get_extent(MPI_Datatype datatype, MPI_Aint *lb, MPI_Aint *extent);

/* Deprecated Functions. 
 * The MPI-2 standard deprecated a number of routines because MPI-2 provides better versions. 
 * This routine is one of those that was deprecated. The routine may continue to be used, but 
 * new code should use the replacement routine. The replacement for this routine is MPI_Type_Get_extent.
 **/
int SMPI_MPI_Type_ub( MPI_Datatype datatype, MPI_Aint *displacement);
int SMPI_MPI_Type_lb( MPI_Datatype datatype, MPI_Aint *displacement);

