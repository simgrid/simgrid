#include "mpi.h"
#include <stdio.h>
int main( int argc, char **argv )
{
  int fsizeof_aint   = ;
  int fsizeof_offset = ;
  int err = 0, rc = 0;

  MPI_Init( &argc, &argv );
  if (sizeof(MPI_Aint) != fsizeof_aint) {
     printf( "Sizeof MPI_Aint is %d but Fortran thinks it is %d\n",
             (int)sizeof(MPI_Aint), fsizeof_aint );
     err++;
  }
  if (sizeof(MPI_Offset) != fsizeof_offset) {
     printf( "Sizeof MPI_Offset is %d but Fortran thinks it is %d\n",
             (int)sizeof(MPI_Offset), fsizeof_offset );
     err++;
  }
  MPI_Finalize( );
  if (err > 0) rc = 1;
  return rc;
}
