/****************************************************************************

 MESSAGE PASSING INTERFACE TEST CASE SUITE

 Copyright IBM Corp. 1995

 IBM Corp. hereby grants a non-exclusive license to use, copy, modify, and
 distribute this software for any purpose and without fee provided that the
 above copyright notice and the following paragraphs appear in all copies.

 IBM Corp. makes no representation that the test cases comprising this
 suite are correct or are an accurate representation of any standard.

 In no event shall IBM be liable to any party for direct, indirect, special
 incidental, or consequential damage arising out of the use of this software
 even if IBM Corp. has been advised of the possibility of such damage.

 IBM CORP. SPECIFICALLY DISCLAIMS ANY WARRANTIES INCLUDING, BUT NOT LIMITED
 TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS" BASIS AND IBM
 CORP. HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES,
 ENHANCEMENTS, OR MODIFICATIONS.

****************************************************************************

 These test cases reflect an interpretation of the MPI Standard.  They are
 are, in most cases, unit tests of specific MPI behaviors.  If a user of any
 test case from this set believes that the MPI Standard requires behavior
 different than that implied by the test case we would appreciate feedback.

 Comments may be sent to:
    Richard Treumann
    treumann@kgn.ibm.com

****************************************************************************
*/
#include "mpi.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


#define MAXLEN 10000
 
int main( int argc, char *argv[] )
{
#define N 1000000
   int *out, *in,i,j,k;
   int myself,tasks;
 
   out = malloc(N*sizeof(int));
   in  = malloc(N*sizeof(int));
   if ((out==NULL) || (in==NULL)) {
           printf("Error: cannot allocate N bytes for in or out arrays\n");
           exit(1);
   }
   MPI_Init( &argc,&argv );
   MPI_Comm_rank(MPI_COMM_WORLD,&myself);
   MPI_Comm_size(MPI_COMM_WORLD,&tasks);
   for(j=1;j<=MAXLEN;j*=10)  {
           if ( 0 == myself ) {
                   printf("* calling MPI_Alltoall with buffers of %d ints\n",j);
           }
           for(i=0;i<j*tasks;i++)  
                   out[i] = myself;

           MPI_Alltoall(out,j,MPI_INT,in,j,MPI_INT,MPI_COMM_WORLD);

           for(i=0;i<tasks;i++)  {
                   for(k=0;k<j;k++) {
                           if(in[k+i*j] != i) {  
                                   printf("<%d> bad answer (%d) at index %d of %d (should be %d)\n",myself,in[k+i*j],k+i*j,j*tasks,i); 
                                   break; 
                           }
                   }
           }
   }
   MPI_Barrier(MPI_COMM_WORLD);
   if(myself==0)  printf("TEST COMPLETE\n");
   MPI_Finalize();
}
