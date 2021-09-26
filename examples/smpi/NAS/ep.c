#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "smpi/mpi.h"
#include "nas_common.h"
#include "simgrid/instr.h" //TRACE_

char class;
int nprocs;

#define TRUE 1
#define FALSE 0

int main(int argc, char **argv) {
  double dum[3] = {1.,1.,1.};
  double x1, x2, sx, sy, tm, an, gc;
  double Mops;
  double epsilon=1.0E-8, a = 1220703125., s=271828183.;
  double t1, t2, t3, t4;
  double sx_verify_value, sy_verify_value, sx_err, sy_err;

  int    m = 24;
  int    mk=16;
  int    nk = (int)(pow(2,mk)),
         nq=10,
         np, node, no_nodes, i, ik, kk, l, k, nit, no_large_nodes, np_add, k_offset;
  int    verified;
  char   size[500]; // mind the size of the string to represent a big number

  double *x = (double *) SMPI_SHARED_MALLOC (2*nk*sizeof(double));
  double *q = (double *) SMPI_SHARED_MALLOC (nq*sizeof(double));

  MPI_Init( &argc, &argv );
  MPI_Comm_size( MPI_COMM_WORLD, &no_nodes);
  MPI_Comm_rank( MPI_COMM_WORLD, &node);

  TRACE_smpi_set_category ("start");

  get_info(argc, argv, &nprocs, &class);
  check_info(EP, nprocs, class);

  if      (class == 'S')
    { m = 24; }
  else if (class == 'W')
    { m = 25; }
  else if (class == 'A')
    { m = 28; }
  else if (class == 'B')
    { m = 30; }
  else if (class == 'C')
    { m = 32; }
  else if (class == 'D')
    { m = 36; }
  else if (class == 'E')
    { m = 40; }
  else {
    printf("EP: Internal error: invalid class type %c\n", class);
    exit(1);
  }

  int mm = m -mk;
  int nn = (int)(pow(2,mm));

  int root = 0;
  if (node == root ) {
    /* Because the size of the problem is too large to store in a 32-bit integer for some classes, we put it into a
     * string (for printing). Have to strip off the decimal point put in there by the floating point print statement
     * (internal file) */
    fprintf(stdout," NAS Parallel Benchmarks 3.2 -- EP Benchmark");
    snprintf(size,500,"%lu",(unsigned long)pow(2,m+1));
    fprintf(stdout," Number of random numbers generated: %s\n",size);
    fprintf(stdout," Number of active processes: %d\n",no_nodes);
  }

  /* Compute the number of "batches" of random number pairs generated per processor. Adjust if the number of processors
   * does not evenly divide the total number  */
  np = nn / no_nodes;
  no_large_nodes = nn % no_nodes;
  if (node < no_large_nodes)
    np_add = 1;
  else
    np_add = 0;
  np = np + np_add;

  if (np == 0) {
    fprintf(stdout,"Too many nodes: %d  %d",no_nodes,nn);
    MPI_Abort(MPI_COMM_WORLD,1);
    exit(0);
  }

  /* Call the random number generator functions and initialize the x-array to reduce the effects of paging the timings.
   Also, call all mathematical functions that are used. Make sure initializations cannot be eliminated as dead code. */
  vranlc(0, dum[0], dum[1], &(dum[2]));

  dum[0] = randlc(&(dum[1]),&(dum[2]));
  for (i=0;i<2*nk;i++) {
    x[i] = -1e99;
  }

  /* Synchronize before placing time stamp */
  MPI_Barrier( MPI_COMM_WORLD );

  TRACE_smpi_set_category ("ep");

  timer_clear(1);
  timer_clear(2);
  timer_clear(3);
  timer_start(1);

  t1 = a;
  t1 = vranlc(0, t1, a, x);

  /* Compute AN = A ^ (2 * NK) (mod 2^46). */
  t1 = a;
  for (i=1; i <= mk+1; i++) {
    t2 = randlc(&t1, &t1);
  }
  an = t1;
  gc = 0;
  sx = 0.;
  sy = 0.;
  for (i=0; i < nq ; i++) {
    q[i] = 0.;
  }

/* Each instance of this loop may be performed independently. We compute the k offsets separately to take into account
 * the fact that some nodes have more numbers to generate than others */
  if (np_add == 1)
    k_offset = node * np -1;
  else
    k_offset = no_large_nodes*(np+1) + (node-no_large_nodes)*np -1;

  int stop = FALSE;
  SMPI_SAMPLE_GLOBAL(k = 1, k <= np, k++, 0.25 * np, 0.03){
    stop = FALSE;
    kk = k_offset + k ;
    t1 = s;
    t2 = an;

    //       Find starting seed t1 for this kk.
    for (i=1;i<=100 && !stop;i++) {
      ik = kk / 2;
      if (2 * ik != kk)  {
        randlc(&t1, &t2);
      }
      if (ik==0)
        stop = TRUE;
      else {
        randlc(&t2, &t2);
        kk = ik;
      }
    }
    //       Compute uniform pseudorandom numbers.

    timer_start(3);

    t1 = vranlc(2 * nk, t1, a, x);

    timer_stop(3);

    /* Compute Gaussian deviates by acceptance-rejection method and tally counts in concentric square annuli.
     * This loop is not vectorizable. */
    timer_start(2);

    for(i=1; i<=nk;i++) {
      x1 = 2. * x[2*i-2] -1.0;
      x2 = 2. * x[2*i-1] - 1.0;
      t1 = x1*x1 + x2*x2;
      if (t1 <= 1.) {
        t2   = sqrt(-2. * log(t1) / t1);
        t3   = (x1 * t2);
        t4   = (x2 * t2);
        l    = (int)(fabs(t3) > fabs(t4) ? fabs(t3) : fabs(t4));
        q[l] = q[l] + 1.;
        sx   = sx + t3;
        sy   = sy + t4;
      }
    }
    timer_stop(2);
  }

  TRACE_smpi_set_category ("finalize");

  MPI_Allreduce(&sx, x, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  sx = x[0];
  MPI_Allreduce(&sy, x, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  sy = x[0];
  MPI_Allreduce(q, x, nq, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

  for(i = 0; i < nq; i++) {
    q[i] = x[i];
  }
  for(i = 0; i < nq; i++) {
    gc += q[i];
  }

  timer_stop(1);
  tm = timer_read(1);
  MPI_Allreduce(&tm, x, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
  tm = x[0];

  SMPI_SHARED_FREE(x);

  if(node == root) {
    nit = 0;
    verified = TRUE;

    if(m == 24) {
      sx_verify_value = -3.247834652034740E3;
      sy_verify_value = -6.958407078382297E3;
    } else if(m == 25) {
      sx_verify_value = -2.863319731645753E3;
      sy_verify_value = -6.320053679109499E3;
    } else if(m == 28) {
      sx_verify_value = -4.295875165629892E3;
      sy_verify_value = -1.580732573678431E4;
    } else if(m == 30) {
      sx_verify_value =  4.033815542441498E4;
      sy_verify_value = -2.660669192809235E4;
    } else if(m == 32) {
      sx_verify_value =  4.764367927995374E4;
      sy_verify_value = -8.084072988043731E4;
    } else if(m == 36) {
      sx_verify_value =  1.982481200946593E5;
      sy_verify_value = -1.020596636361769E5;
    } else {
      verified = FALSE;
    }

    if(verified) {
      sx_err = fabs((sx - sx_verify_value)/sx_verify_value);
      sy_err = fabs((sy - sy_verify_value)/sy_verify_value);
      verified = ((sx_err < epsilon) && (sy_err < epsilon));
    }

    Mops = (pow(2.0, m+1))/tm/1000;

    fprintf(stdout,"EP Benchmark Results:\n");
    fprintf(stdout,"CPU Time=%d\n",(int) tm);
    fprintf(stdout,"N = 2^%d\n",m);
    fprintf(stdout,"No. Gaussain Pairs =%d\n",(int) gc);
    fprintf(stdout,"Sum = %f %ld\n",sx,(long) sy);
    fprintf(stdout,"Count:");
    for(i = 0; i < nq; i++) {
      fprintf(stdout,"%d\t %ld\n",i,(long) q[i]);
    }
    c_print_results("EP", class, m+1, 0, 0, nit, nprocs, no_nodes, tm, Mops, "Random number generated",verified);

    fprintf(stdout,"Total time:     %f\n",(timer_read(1)/1000));
    fprintf(stdout,"Gaussian pairs: %f\n",(timer_read(2)/1000));
    fprintf(stdout,"Random numbers: %f\n",(timer_read(3)/1000));
  }

  SMPI_SHARED_FREE(q);

  MPI_Finalize();
  return 0;
}
