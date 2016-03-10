/*************************************************************************
 *                                                                       * 
 *        N  A  S     P A R A L L E L     B E N C H M A R K S  3.3       *
 *                                                                       * 
 *                                  I S                                  * 
 *                                                                       * 
 ************************************************************************* 
 *                                                                       * 
 *   This benchmark is part of the NAS Parallel Benchmark 3.3 suite.     *
 *   It is described in NAS Technical Report 95-020.                     * 
 *                                                                       * 
 *   Permission to use, copy, distribute and modify this software        * 
 *   for any purpose with or without fee is hereby granted.  We          * 
 *   request, however, that all derived work reference the NAS           * 
 *   Parallel Benchmarks 3.3. This software is provided "as is"          *
 *   without express or implied warranty.                                * 
 *                                                                       * 
 *   Information on NPB 3.3, including the technical report, the         *
 *   original specifications, source code, results and information       * 
 *   on how to submit new results, is available at:                      * 
 *                                                                       * 
 *          http://www.nas.nasa.gov/Software/NPB                         * 
 *                                                                       * 
 *   Send comments or suggestions to  npb@nas.nasa.gov                   * 
 *   Send bug reports to              npb-bugs@nas.nasa.gov              * 
 *                                                                       * 
 *         NAS Parallel Benchmarks Group                                 * 
 *         NASA Ames Research Center                                     * 
 *         Mail Stop: T27A-1                                             * 
 *         Moffett Field, CA   94035-1000                                * 
 *                                                                       * 
 *         E-mail:  npb@nas.nasa.gov                                     * 
 *         Fax:     (650) 604-3957                                       * 
 *                                                                       * 
 ************************************************************************* 
 *                                                                       * 
 *   Author: M. Yarrow                                                   * 
 *           H. Jin                                                      * 
 *                                                                       * 
 *************************************************************************/

#include "smpi/mpi.h"
#include "nas_common.h"
#include <stdlib.h>
#include <stdio.h>

#include "simgrid/instr.h" //TRACE_

char class;
int nprocs;
int total_keys_log2;
int max_key_log_2;
int num_bucket_log_2;
int min_procs=1;
/* NOTE: THIS CODE CANNOT BE RUN ON ARBITRARILY LARGE NUMBERS OF PROCESSORS. THE LARGEST VERIFIED NUMBER IS 1024.
 * INCREASE max_procs AT YOUR PERIL */
int max_procs=1024;

int total_keys;
int max_key;
int num_buckets;
int num_keys;
long size_of_buffers;

#define  MAX_ITERATIONS      10
#define  TEST_ARRAY_SIZE     5

/* Typedef: if necessary, change the size of int here by changing the  int type to, say, long */
typedef  int  INT_TYPE;
typedef  long INT_TYPE2;
#define MP_KEY_TYPE MPI_INT

typedef struct {
/* MPI properties:  */
int      my_rank, comm_size;
/* Some global info */
INT_TYPE *key_buff_ptr_global,         /* used by full_verify to get */
         total_local_keys,             /* copies of rank info        */
         total_lesser_keys;

int      passed_verification;
/* These are the three main arrays. See SIZE_OF_BUFFERS def above    */
INT_TYPE *key_array, *key_buff1, *key_buff2,
         *bucket_size,     /* Top 5 elements for */
         *bucket_size_totals, /* part. ver. vals */
         *bucket_ptrs, *process_bucket_distrib_ptr1, *process_bucket_distrib_ptr2;
int      send_count[1024], recv_count[1024], send_displ[1024], recv_displ[1024];

/* Partial verif info */
INT_TYPE2 test_index_array[TEST_ARRAY_SIZE],
         test_rank_array[TEST_ARRAY_SIZE];
} global_data;

const INT_TYPE2
         S_test_index_array[TEST_ARRAY_SIZE] = {48427,17148,23627,62548,4431},
         S_test_rank_array[TEST_ARRAY_SIZE] =  {0,18,346,64917,65463},
         W_test_index_array[TEST_ARRAY_SIZE] = {357773,934767,875723,898999,404505},
         W_test_rank_array[TEST_ARRAY_SIZE] =  {1249,11698,1039987,1043896,1048018},

         A_test_index_array[TEST_ARRAY_SIZE] = {2112377,662041,5336171,3642833,4250760},
         A_test_rank_array[TEST_ARRAY_SIZE] =  {104,17523,123928,8288932,8388264},

         B_test_index_array[TEST_ARRAY_SIZE] = {41869,812306,5102857,18232239,26860214},
         B_test_rank_array[TEST_ARRAY_SIZE] =  {33422937,10244,59149,33135281,99},

         C_test_index_array[TEST_ARRAY_SIZE] = {44172927,72999161,74326391,129606274,21736814},
         C_test_rank_array[TEST_ARRAY_SIZE] =  {61147,882988,266290,133997595,133525895},

         D_test_index_array[TEST_ARRAY_SIZE] = {1317351170,995930646,1157283250,1503301535,1453734525},
         D_test_rank_array[TEST_ARRAY_SIZE] =  {1,36538729,1978098519,2145192618,2147425337};

void full_verify( global_data* gd );

/************ returns parallel random number seq seed ************/
/*
 * Create a random number sequence of total length nn residing on np number of processors.  Each processor will
 * therefore have a subsequence of length nn/np.  This routine returns that random number which is the first random
 * number for the subsequence belonging to processor rank kn, and which is used as seed for proc kn ran # gen.
 */
static double  find_my_seed( int  kn,       /* my processor rank, 0<=kn<=num procs */
                             int  np,       /* np = num procs                      */
                             long nn,       /* total num of ran numbers, all procs */
                             double s,      /* Ran num seed, for ex.: 314159265.00 */
                             double a )     /* Ran num gen mult, try 1220703125.00 */
{
  long   i;
  double t1,t2,t3,an;
  long   mq,nq,kk,ik;

  nq = nn / np;

  for( mq=0; nq>1; mq++,nq/=2);

  t1 = a;

  for( i=1; i<=mq; i++ )
    t2 = randlc( &t1, &t1 );

  an = t1;

  kk = kn;
  t1 = s;
  t2 = an;

  for( i=1; i<=100; i++ ){
    ik = kk / 2;
    if( 2 * ik !=  kk )
      t3 = randlc( &t1, &t2 );
    if( ik == 0 )
      break;
    t3 = randlc( &t2, &t2 );
    kk = ik;
  }
  an=t3;//added to silence paranoid compilers

  return t1;
}

static void create_seq( global_data* gd, double seed, double a )
{
  double x;
  int    i, k;

  k = max_key/4;

  for (i=0; i<num_keys; i++){
    x = randlc(&seed, &a);
    x += randlc(&seed, &a);
    x += randlc(&seed, &a);
    x += randlc(&seed, &a);

    gd->key_array[i] = k*x;
  }
}

void full_verify( global_data* gd )
{
  MPI_Status  status;
  MPI_Request request;

  INT_TYPE    i, j;
  INT_TYPE    k, last_local_key;

/*  Now, finally, sort the keys:  */
  for( i=0; i<gd->total_local_keys; i++ )
    gd->key_array[--gd->key_buff_ptr_global[gd->key_buff2[i]]- gd->total_lesser_keys] = gd->key_buff2[i];
  last_local_key = (gd->total_local_keys<1)? 0 : (gd->total_local_keys-1);

/*  Send largest key value to next processor  */
  if( gd->my_rank > 0 )
    MPI_Irecv( &k, 1, MP_KEY_TYPE, gd->my_rank-1, 1000, MPI_COMM_WORLD, &request );
  if( gd->my_rank < gd->comm_size-1 )
    MPI_Send( &gd->key_array[last_local_key], 1, MP_KEY_TYPE, gd->my_rank+1, 1000, MPI_COMM_WORLD );
  if( gd->my_rank > 0 )
    MPI_Wait( &request, &status );

/*  Confirm that neighbor's greatest key value is not greater than my least key value */
  j = 0;
  if( gd->my_rank > 0 && gd->total_local_keys > 0 )
    if( k > gd->key_array[0] )
      j++;

/*  Confirm keys correctly sorted: count incorrectly sorted keys, if any */
  for( i=1; i<gd->total_local_keys; i++ )
    if( gd->key_array[i-1] > gd->key_array[i] )
      j++;

  if( j != 0 ) {
    printf( "Processor %d:  Full_verify: number of keys out of sort: %d\n", gd->my_rank, j );
  } else
    gd->passed_verification++;
}

static void rank( global_data* gd, int iteration )
{
  INT_TYPE    i, k;
  INT_TYPE    shift = max_key_log_2 - num_bucket_log_2;
  INT_TYPE    key;
  INT_TYPE2   bucket_sum_accumulator, j, m;
  INT_TYPE    local_bucket_sum_accumulator;
  INT_TYPE    min_key_val, max_key_val;
  INT_TYPE    *key_buff_ptr;

/*  Iteration alteration of keys */  
  if(gd->my_rank == 0){
    gd->key_array[iteration] = iteration;
    gd->key_array[iteration+MAX_ITERATIONS] = max_key - iteration;
  }

/*  Initialize */
  for( i=0; i<num_buckets+TEST_ARRAY_SIZE; i++ ){
    gd->bucket_size[i] = 0;
    gd->bucket_size_totals[i] = 0;
    gd->process_bucket_distrib_ptr1[i] = 0;
    gd->process_bucket_distrib_ptr2[i] = 0;
  }

/*  Determine where the partial verify test keys are, load into top of array bucket_size */
  for( i=0; i<TEST_ARRAY_SIZE; i++ )
    if( (gd->test_index_array[i]/num_keys) == gd->my_rank )
      gd->bucket_size[num_buckets+i] = gd->key_array[gd->test_index_array[i] % num_keys];

/*  Determine the number of keys in each bucket */
  for( i=0; i<num_keys; i++ )
    gd->bucket_size[gd->key_array[i] >> shift]++;

/*  Accumulative bucket sizes are the bucket pointers */
  gd->bucket_ptrs[0] = 0;
  for( i=1; i< num_buckets; i++ )
    gd->bucket_ptrs[i] = gd->bucket_ptrs[i-1] + gd->bucket_size[i-1];

/*  Sort into appropriate bucket */
  for( i=0; i<num_keys; i++ ) {
    key = gd->key_array[i];
    gd->key_buff1[gd->bucket_ptrs[key >> shift]++] = key;
  }

/*  Get the bucket size totals for the entire problem. These will be used to determine the redistribution of keys */
  MPI_Allreduce(gd->bucket_size, gd->bucket_size_totals, num_buckets+TEST_ARRAY_SIZE, MP_KEY_TYPE, MPI_SUM,
                MPI_COMM_WORLD);

/* Determine Redistibution of keys: accumulate the bucket size totals till this number surpasses num_keys (which the
 * average number of keys per processor).  Then all keys in these buckets go to processor 0.
   Continue accumulating again until supassing 2*num_keys. All keys in these buckets go to processor 1, etc.  This
   algorithm guarantees that all processors have work ranking; no processors are left idle.
   The optimum number of buckets, however, does not result in as high a degree of load balancing (as even a distribution
   of keys as is possible) as is obtained from increasing the number of buckets, but more buckets results in more
   computation per processor so that the optimum number of buckets turns out to be 1024 for machines tested.
   Note that process_bucket_distrib_ptr1 and ..._ptr2 hold the bucket number of first and last bucket which each
   processor will have after the redistribution is done.
*/

  bucket_sum_accumulator = 0;
  local_bucket_sum_accumulator = 0;
  gd->send_displ[0] = 0;
  gd->process_bucket_distrib_ptr1[0] = 0;
  for( i=0, j=0; i<num_buckets; i++ ) {
    bucket_sum_accumulator       += gd->bucket_size_totals[i];
    local_bucket_sum_accumulator += gd->bucket_size[i];
    if( bucket_sum_accumulator >= (j+1)*num_keys ) {
      gd->send_count[j] = local_bucket_sum_accumulator;
      if( j != 0 ){
         gd->send_displ[j] = gd->send_displ[j-1] + gd->send_count[j-1];
         gd->process_bucket_distrib_ptr1[j] = gd->process_bucket_distrib_ptr2[j-1]+1;
      }
      gd->process_bucket_distrib_ptr2[j++] = i;
      local_bucket_sum_accumulator = 0;
    }
  }

/*  When nprocs approaching num_buckets, it is highly possible that the last few processors don't get any buckets.
 *  So, we need to set counts properly in this case to avoid any fallouts.    */
  while( j < gd->comm_size ) {
    gd->send_count[j] = 0;
    gd->process_bucket_distrib_ptr1[j] = 1;
    j++;
  }

/*  This is the redistribution section:  first find out how many keys
    each processor will send to every other processor:                 */
  MPI_Alltoall( gd->send_count, 1, MPI_INT, gd->recv_count, 1, MPI_INT, MPI_COMM_WORLD );

/*  Determine the receive array displacements for the buckets */
  gd->recv_displ[0] = 0;
  for( i=1; i<gd->comm_size; i++ )
    gd->recv_displ[i] = gd->recv_displ[i-1] + gd->recv_count[i-1];

  /*  Now send the keys to respective processors  */
  MPI_Alltoallv(gd->key_buff1, gd->send_count, gd->send_displ, MP_KEY_TYPE, gd->key_buff2, gd->recv_count,
                gd->recv_displ, MP_KEY_TYPE, MPI_COMM_WORLD );

/* The starting and ending bucket numbers on each processor are multiplied by the interval size of the buckets to
 * obtain the smallest possible min and greatest possible max value of any key on each processor
 */
  min_key_val = gd->process_bucket_distrib_ptr1[gd->my_rank] << shift;
  max_key_val = ((gd->process_bucket_distrib_ptr2[gd->my_rank] + 1) << shift)-1;

/*  Clear the work array */
  for( i=0; i<max_key_val-min_key_val+1; i++ )
    gd->key_buff1[i] = 0;

/*  Determine the total number of keys on all other processors holding keys of lesser value         */
  m = 0;
  for( k=0; k<gd->my_rank; k++ )
    for( i= gd->process_bucket_distrib_ptr1[k]; i<=gd->process_bucket_distrib_ptr2[k]; i++ )
      m += gd->bucket_size_totals[i]; /*  m has total # of lesser keys */

/*  Determine total number of keys on this processor */
  j = 0;
  for( i= gd->process_bucket_distrib_ptr1[gd->my_rank]; i<=gd->process_bucket_distrib_ptr2[gd->my_rank]; i++ )
    j += gd->bucket_size_totals[i];     /* j has total # of local keys   */

/*  Ranking of all keys occurs in this section:                 */
/*  shift it backwards so no subtractions are necessary in loop */
  key_buff_ptr = gd->key_buff1 - min_key_val;

/*  In this section, the keys themselves are used as their own indexes to determine how many of each there are: their
    individual population                                       */
  for( i=0; i<j; i++ )
    key_buff_ptr[gd->key_buff2[i]]++;  /* Now they have individual key  population                     */

/*  To obtain ranks of each key, successively add the individual key population, not forgetting the total of lesser
 *  keys, m.
    NOTE: Since the total of lesser keys would be subtracted later in verification, it is no longer added to the first
    key population here, but still needed during the partial verify test.  This is to ensure that 32-bit key_buff can
    still be used for class D.           */
/*    key_buff_ptr[min_key_val] += m;    */
  for( i=min_key_val; i<max_key_val; i++ )
    key_buff_ptr[i+1] += key_buff_ptr[i];

/* This is the partial verify test section */
/* Observe that test_rank_array vals are shifted differently for different cases */
  for( i=0; i<TEST_ARRAY_SIZE; i++ ){
    k = gd->bucket_size_totals[i+num_buckets];    /* Keys were hidden here */
    if( min_key_val <= k  &&  k <= max_key_val ){
      /* Add the total of lesser keys, m, here */
      INT_TYPE2 key_rank = key_buff_ptr[k-1] + m;
      int failed = 0;

      switch( class ){
        case 'S':
          if( i <= 2 ) {
            if( key_rank != gd->test_rank_array[i]+iteration )
              failed = 1;
            else
              gd->passed_verification++;
          } else {
            if( key_rank != gd->test_rank_array[i]-iteration )
              failed = 1;
            else
              gd->passed_verification++;
          }
          break;
        case 'W':
          if( i < 2 ){
            if( key_rank != gd->test_rank_array[i]+(iteration-2) )
              failed = 1;
            else
              gd->passed_verification++;
          } else {
              if( key_rank != gd->test_rank_array[i]-iteration )
                failed = 1;
              else
                gd->passed_verification++;
          }
          break;
        case 'A':
          if( i <= 2 ){
            if( key_rank != gd->test_rank_array[i]+(iteration-1) )
              failed = 1;
            else
              gd->passed_verification++;
          } else {
              if( key_rank !=  gd->test_rank_array[i]-(iteration-1) )
                failed = 1;
              else
                gd->passed_verification++;
          }
          break;
        case 'B':
          if( i == 1 || i == 2 || i == 4 ) {
            if( key_rank != gd->test_rank_array[i]+iteration )
              failed = 1;
            else
              gd->passed_verification++;
          } else {
              if( key_rank != gd->test_rank_array[i]-iteration )
                failed = 1;
              else
                gd->passed_verification++;
          }
          break;
        case 'C':
          if( i <= 2 ){
            if( key_rank != gd->test_rank_array[i]+iteration )
              failed = 1;
            else
              gd->passed_verification++;
          } else {
              if( key_rank != gd->test_rank_array[i]-iteration )
                failed = 1;
              else
                gd->passed_verification++;
          }
          break;
        case 'D':
          if( i < 2 ) {
            if( key_rank != gd->test_rank_array[i]+iteration )
              failed = 1;
            else
              gd->passed_verification++;
           } else {
              if( key_rank != gd->test_rank_array[i]-iteration )
                failed = 1;
              else
                gd->passed_verification++;
           }
         break;
      }
      if( failed == 1 )
        printf( "Failed partial verification: iteration %d, processor %d, test key %d\n",
               iteration, gd->my_rank, (int)i );
    }
  }

/*  Make copies of rank info for use by full_verify: these variables in rank are local; making them global slows down
 *  the code, probably since they cannot be made register by compiler                        */

  if( iteration == MAX_ITERATIONS ) {
    gd->key_buff_ptr_global = key_buff_ptr;
    gd->total_local_keys    = j;
    gd->total_lesser_keys   = 0;  /* no longer set to 'm', see note above */
  }
}

int main( int argc, char **argv )
{
  int             i, iteration, itemp;
  double          timecounter, maxtime;

  global_data* gd = malloc(sizeof(global_data));
/*  Initialize MPI */
  MPI_Init( &argc, &argv );
  MPI_Comm_rank( MPI_COMM_WORLD, &gd->my_rank );
  MPI_Comm_size( MPI_COMM_WORLD, &gd->comm_size );

  get_info(argc, argv, &nprocs, &class);
  check_info(IS, nprocs, class);
/*  Initialize the verification arrays if a valid class */
  for( i=0; i<TEST_ARRAY_SIZE; i++ )

    switch( class ) {
      case 'S':
         total_keys_log2 = 16;
         max_key_log_2 = 11;
         num_bucket_log_2 = 9;
         max_procs = 128;
         gd->test_index_array[i] = S_test_index_array[i];
         gd->test_rank_array[i]  = S_test_rank_array[i];
         break;
      case 'A':
         total_keys_log2 = 23;
         max_key_log_2 = 19;
         num_bucket_log_2 = 10;
         gd->test_index_array[i] = A_test_index_array[i];
         gd->test_rank_array[i]  = A_test_rank_array[i];
         break;
      case 'W':
          total_keys_log2 = 20;
          max_key_log_2 = 16;
          num_bucket_log_2 = 10;
         gd->test_index_array[i] = W_test_index_array[i];
         gd->test_rank_array[i]  = W_test_rank_array[i];
         break;
      case 'B':
          total_keys_log2 = 25;
          max_key_log_2 = 21;
          num_bucket_log_2 = 10;
         gd->test_index_array[i] = B_test_index_array[i];
         gd->test_rank_array[i]  = B_test_rank_array[i];
         break;
      case 'C':
          total_keys_log2 = 27;
          max_key_log_2 = 23;
          num_bucket_log_2 = 10;
         gd->test_index_array[i] = C_test_index_array[i];
         gd->test_rank_array[i]  = C_test_rank_array[i];
         break;
      case 'D':
          total_keys_log2 = 29;
          max_key_log_2 = 27;
          num_bucket_log_2 = 10;
         min_procs = 4;
         gd->test_index_array[i] = D_test_index_array[i];
         gd->test_rank_array[i]  = D_test_rank_array[i];
         break;
    };

  total_keys  = (1 << total_keys_log2);
  max_key     = (1 << max_key_log_2);
  num_buckets = (1 << num_bucket_log_2);
  num_keys    = (total_keys/nprocs*min_procs);

  /* On larger number of processors, since the keys are (roughly)  gaussian distributed, the first and last processor
   * sort keys in a large interval, requiring array sizes to be larger. Note that for large NUM_PROCS, num_keys is,
   * however, a small number The required array size also depends on the bucket size used. The following values are
   * validated for the 1024-bucket setup. */
  if (nprocs < 256)
    size_of_buffers = 3*num_keys/2;
  else if (nprocs < 512)
    size_of_buffers = 5*num_keys/2;
  else if (nprocs < 1024)
    size_of_buffers = 4*num_keys/2;
  else
    size_of_buffers = 13*num_keys/2;

  gd->key_array = (INT_TYPE*)malloc(size_of_buffers*sizeof(INT_TYPE));
  gd->key_buff1 = (INT_TYPE*)malloc(size_of_buffers*sizeof(INT_TYPE));
  gd->key_buff2 = (INT_TYPE*)malloc(size_of_buffers*sizeof(INT_TYPE));
  gd->bucket_size = (INT_TYPE*)malloc((num_buckets+TEST_ARRAY_SIZE)*sizeof(INT_TYPE));     /* Top 5 elements for */
  gd->bucket_size_totals = (INT_TYPE*)malloc((num_buckets+TEST_ARRAY_SIZE)*sizeof(INT_TYPE)); /* part. ver. vals */
  gd->bucket_ptrs = (INT_TYPE*)malloc(num_buckets*sizeof(INT_TYPE));
  gd->process_bucket_distrib_ptr1 = (INT_TYPE*)malloc((num_buckets+TEST_ARRAY_SIZE)*sizeof(INT_TYPE));
  gd->process_bucket_distrib_ptr2 = (INT_TYPE*)malloc((num_buckets+TEST_ARRAY_SIZE)*sizeof(INT_TYPE));
//  int      send_count[max_procs], recv_count[max_procs],
//           send_displ[max_procs], recv_displ[max_procs];

/*  Printout initial NPB info */
  if( gd->my_rank == 0 ){
     printf( "\n\n NAS Parallel Benchmarks 3.3 -- IS Benchmark\n\n" );
     printf( " Size:  %ld  (class %c)\n", (long)total_keys*min_procs, class);
     printf( " Iterations:   %d\n", MAX_ITERATIONS );
     printf( " Number of processes:     %d\n",gd->comm_size );
  }

/*  Check that actual and compiled number of processors agree */
  if( gd->comm_size != nprocs) {
    if( gd->my_rank == 0 )
       printf( "\n ERROR: compiled for %d processes\n"
               " Number of active processes: %d\n"
               " Exiting program!\n\n", nprocs, gd->comm_size );
    MPI_Finalize();
    exit( 1 );
  }

/*  Check to see whether total number of processes is within bounds.
    This could in principle be checked in setparams.c, but it is more convenient to do it here */
  if( gd->comm_size < min_procs || gd->comm_size > max_procs){
    if( gd->my_rank == 0 )
      printf( "\n ERROR: number of processes %d not within range %d-%d"
              "\n Exiting program!\n\n", gd->comm_size, min_procs, max_procs);
    MPI_Finalize();
    exit( 1 );
  }

/*  Generate random number sequence and subsequent keys on all procs */
  create_seq(gd,  find_my_seed( gd->my_rank, gd->comm_size, 4*(long)total_keys*min_procs,
             314159265.00,      /* Random number gen seed */
             1220703125.00 ),   /* Random number gen mult */
             1220703125.00 );   /* Random number gen mult */

/*  Do one interation for free (i.e., untimed) to guarantee initialization of  
    all data and code pages and respective tables */
  rank(gd, 1 );

/*  Start verification counter */
  gd->passed_verification = 0;

  if( gd->my_rank == 0 && class != 'S' ) printf( "\n   iteration\n" );

/*  Initialize timer  */
  timer_clear(0);

/*  Start timer */
  timer_start(0);

  char smpi_category[100];
  snprintf (smpi_category, 100, "%d", gd->my_rank);
  TRACE_smpi_set_category (smpi_category);

/*  This is the main iteration */
  for( iteration=1; iteration<=MAX_ITERATIONS; iteration++ ) {
    if( gd->my_rank == 0 && class != 'S' ) printf( "        %d\n", iteration );
    rank(gd,  iteration );
  }
  TRACE_smpi_set_category (NULL);

/*  Stop timer, obtain time for processors */
  timer_stop(0);

  timecounter = timer_read(0);

/*  End of timing, obtain maximum time of all processors */
  MPI_Reduce( &timecounter, &maxtime, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD );

/*  This tests that keys are in sequence: sorting of last ranked key seq occurs here, but is an untimed operation */
  full_verify(gd);

/*  Obtain verification counter sum */
  itemp =gd->passed_verification;
  MPI_Reduce( &itemp, &gd->passed_verification, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD );

/*  The final printout  */
  if( gd->my_rank == 0 ) {
    if( gd->passed_verification != 5*MAX_ITERATIONS + gd->comm_size )
      gd->passed_verification = 0;
    c_print_results("IS", class, (int)(total_keys), min_procs, 0, MAX_ITERATIONS, nprocs, gd->comm_size, maxtime,
                    ((double) (MAX_ITERATIONS)*total_keys*min_procs)/maxtime/1000000., "keys ranked",
                    gd->passed_verification);
  }

  MPI_Finalize();
  free(gd);

  return 0;
}
