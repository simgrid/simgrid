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

#include "mpi.h"
#include "npbparams.h"
#include <stdlib.h>
#include <stdio.h>

/******************/
/* default values */
/******************/
#ifndef CLASS
#define CLASS 'S'
#define NUM_PROCS            1                 
#endif
#define MIN_PROCS            1


/*************/
/*  CLASS S  */
/*************/
#if CLASS == 'S'
#define  TOTAL_KEYS_LOG_2    16
#define  MAX_KEY_LOG_2       11
#define  NUM_BUCKETS_LOG_2   9
#endif


/*************/
/*  CLASS W  */
/*************/
#if CLASS == 'W'
#define  TOTAL_KEYS_LOG_2    20
#define  MAX_KEY_LOG_2       16
#define  NUM_BUCKETS_LOG_2   10
#endif

/*************/
/*  CLASS A  */
/*************/
#if CLASS == 'A'
#define  TOTAL_KEYS_LOG_2    23
#define  MAX_KEY_LOG_2       19
#define  NUM_BUCKETS_LOG_2   10
#endif


/*************/
/*  CLASS B  */
/*************/
#if CLASS == 'B'
#define  TOTAL_KEYS_LOG_2    25
#define  MAX_KEY_LOG_2       21
#define  NUM_BUCKETS_LOG_2   10
#endif


/*************/
/*  CLASS C  */
/*************/
#if CLASS == 'C'
#define  TOTAL_KEYS_LOG_2    27
#define  MAX_KEY_LOG_2       23
#define  NUM_BUCKETS_LOG_2   10
#endif


/*************/
/*  CLASS D  */
/*************/
#if CLASS == 'D'
#define  TOTAL_KEYS_LOG_2    29
#define  MAX_KEY_LOG_2       27
#define  NUM_BUCKETS_LOG_2   10
#undef   MIN_PROCS
#define  MIN_PROCS           4
#endif


#define  TOTAL_KEYS          (1 << TOTAL_KEYS_LOG_2)
#define  MAX_KEY             (1 << MAX_KEY_LOG_2)
#define  NUM_BUCKETS         (1 << NUM_BUCKETS_LOG_2)
#define  NUM_KEYS            (TOTAL_KEYS/NUM_PROCS*MIN_PROCS)

/*****************************************************************/
/* On larger number of processors, since the keys are (roughly)  */ 
/* gaussian distributed, the first and last processor sort keys  */ 
/* in a large interval, requiring array sizes to be larger. Note */
/* that for large NUM_PROCS, NUM_KEYS is, however, a small number*/
/* The required array size also depends on the bucket size used. */
/* The following values are validated for the 1024-bucket setup. */
/*****************************************************************/
#if   NUM_PROCS < 256
#define  SIZE_OF_BUFFERS     3*NUM_KEYS/2
#elif NUM_PROCS < 512
#define  SIZE_OF_BUFFERS     5*NUM_KEYS/2
#elif NUM_PROCS < 1024
#define  SIZE_OF_BUFFERS     4*NUM_KEYS
#else
#define  SIZE_OF_BUFFERS     13*NUM_KEYS/2
#endif

/*****************************************************************/
/* NOTE: THIS CODE CANNOT BE RUN ON ARBITRARILY LARGE NUMBERS OF */
/* PROCESSORS. THE LARGEST VERIFIED NUMBER IS 1024. INCREASE     */
/* MAX_PROCS AT YOUR PERIL                                       */
/*****************************************************************/
#if CLASS == 'S'
#define  MAX_PROCS           128
#else
#define  MAX_PROCS           1024
#endif

#define  MAX_ITERATIONS      10
#define  TEST_ARRAY_SIZE     5


/***********************************/
/* Enable separate communication,  */
/* computation timing and printout */
/***********************************/
/* #define  TIMING_ENABLED         */


/*************************************/
/* Typedef: if necessary, change the */
/* size of int here by changing the  */
/* int type to, say, long            */
/*************************************/
typedef  int  INT_TYPE;
typedef  long INT_TYPE2;
#define MP_KEY_TYPE MPI_INT


typedef struct {

/********************/
/* MPI properties:  */
/********************/
int      my_rank,
         comm_size;


/********************/
/* Some global info */
/********************/
INT_TYPE *key_buff_ptr_global,         /* used by full_verify to get */
         total_local_keys,             /* copies of rank info        */
         total_lesser_keys;


int      passed_verification;
                                 


/************************************/
/* These are the three main arrays. */
/* See SIZE_OF_BUFFERS def above    */
/************************************/
INT_TYPE key_array[SIZE_OF_BUFFERS],    
         key_buff1[SIZE_OF_BUFFERS],    
         key_buff2[SIZE_OF_BUFFERS],
         bucket_size[NUM_BUCKETS+TEST_ARRAY_SIZE],     /* Top 5 elements for */
         bucket_size_totals[NUM_BUCKETS+TEST_ARRAY_SIZE], /* part. ver. vals */
         bucket_ptrs[NUM_BUCKETS],
         process_bucket_distrib_ptr1[NUM_BUCKETS+TEST_ARRAY_SIZE],   
         process_bucket_distrib_ptr2[NUM_BUCKETS+TEST_ARRAY_SIZE];   
int      send_count[MAX_PROCS], recv_count[MAX_PROCS],
         send_displ[MAX_PROCS], recv_displ[MAX_PROCS];


/**********************/
/* Partial verif info */
/**********************/
INT_TYPE2 test_index_array[TEST_ARRAY_SIZE],
         test_rank_array[TEST_ARRAY_SIZE];

/**********/
/* Timers */
/**********/
double start[64], elapsed[64];

} global_data;


const INT_TYPE2
         S_test_index_array[TEST_ARRAY_SIZE] = 
                             {48427,17148,23627,62548,4431},
         S_test_rank_array[TEST_ARRAY_SIZE] = 
                             {0,18,346,64917,65463},

         W_test_index_array[TEST_ARRAY_SIZE] = 
                             {357773,934767,875723,898999,404505},
         W_test_rank_array[TEST_ARRAY_SIZE] = 
                             {1249,11698,1039987,1043896,1048018},

         A_test_index_array[TEST_ARRAY_SIZE] = 
                             {2112377,662041,5336171,3642833,4250760},
         A_test_rank_array[TEST_ARRAY_SIZE] = 
                             {104,17523,123928,8288932,8388264},

         B_test_index_array[TEST_ARRAY_SIZE] = 
                             {41869,812306,5102857,18232239,26860214},
         B_test_rank_array[TEST_ARRAY_SIZE] = 
                             {33422937,10244,59149,33135281,99}, 

         C_test_index_array[TEST_ARRAY_SIZE] = 
                             {44172927,72999161,74326391,129606274,21736814},
         C_test_rank_array[TEST_ARRAY_SIZE] = 
                             {61147,882988,266290,133997595,133525895},

         D_test_index_array[TEST_ARRAY_SIZE] = 
                             {1317351170,995930646,1157283250,1503301535,1453734525},
         D_test_rank_array[TEST_ARRAY_SIZE] = 
                             {1,36538729,1978098519,2145192618,2147425337};



/***********************/
/* function prototypes */
/***********************/
double	randlc( double *X, double *A );

void full_verify( global_data* gd );

void c_print_results( char   *name,
                      char   class,
                      int    n1, 
                      int    n2,
                      int    n3,
                      int    niter,
                      int    nprocs_compiled,
                      int    nprocs_total,
                      double t,
                      double mops,
		      char   *optype,
                      int    passed_verification,
                      char   *npbversion,
                      char   *compiletime,
                      char   *mpicc,
                      char   *clink,
                      char   *cmpi_lib,
                      char   *cmpi_inc,
                      char   *cflags,
                      char   *clinkflags );

void    timer_clear(global_data* gd, int n );
void    timer_start(global_data* gd, int n );
void    timer_stop(global_data* gd, int n );
double  timer_read(global_data* gd, int n );

void    timer_clear(global_data* gd, int n ) {
   gd->elapsed[n] = 0.0;
}

void    timer_start(global_data* gd, int n ) {
   gd->start[n] = MPI_Wtime();
}

void    timer_stop(global_data* gd, int n ) {
   gd->elapsed[n] += MPI_Wtime() - gd->start[n];
}

double  timer_read(global_data* gd, int n ) {
   return gd->elapsed[n];
}


/*
 *    FUNCTION RANDLC (X, A)
 *
 *  This routine returns a uniform pseudorandom double precision number in the
 *  range (0, 1) by using the linear congruential generator
 *
 *  x_{k+1} = a x_k  (mod 2^46)
 *
 *  where 0 < x_k < 2^46 and 0 < a < 2^46.  This scheme generates 2^44 numbers
 *  before repeating.  The argument A is the same as 'a' in the above formula,
 *  and X is the same as x_0.  A and X must be odd double precision integers
 *  in the range (1, 2^46).  The returned value RANDLC is normalized to be
 *  between 0 and 1, i.e. RANDLC = 2^(-46) * x_1.  X is updated to contain
 *  the new seed x_1, so that subsequent calls to RANDLC using the same
 *  arguments will generate a continuous sequence.
 *
 *  This routine should produce the same results on any computer with at least
 *  48 mantissa bits in double precision floating point data.  On Cray systems,
 *  double precision should be disabled.
 *
 *  David H. Bailey     October 26, 1990
 *
 *     IMPLICIT DOUBLE PRECISION (A-H, O-Z)
 *     SAVE KS, R23, R46, T23, T46
 *     DATA KS/0/
 *
 *  If this is the first call to RANDLC, compute R23 = 2 ^ -23, R46 = 2 ^ -46,
 *  T23 = 2 ^ 23, and T46 = 2 ^ 46.  These are computed in loops, rather than
 *  by merely using the ** operator, in order to insure that the results are
 *  exact on all systems.  This code assumes that 0.5D0 is represented exactly.
 */


/*****************************************************************/
/*************           R  A  N  D  L  C             ************/
/*************                                        ************/
/*************    portable random number generator    ************/
/*****************************************************************/

double	randlc( double *X, double *A )
{
      static int        KS=0;
      static double	R23, R46, T23, T46;
      double		T1, T2, T3, T4;
      double		A1;
      double		A2;
      double		X1;
      double		X2;
      double		Z;
      int     		i, j;

      if (KS == 0) 
      {
        R23 = 1.0;
        R46 = 1.0;
        T23 = 1.0;
        T46 = 1.0;
    
        for (i=1; i<=23; i++)
        {
          R23 = 0.50 * R23;
          T23 = 2.0 * T23;
        }
        for (i=1; i<=46; i++)
        {
          R46 = 0.50 * R46;
          T46 = 2.0 * T46;
        }
        KS = 1;
      }

/*  Break A into two parts such that A = 2^23 * A1 + A2 and set X = N.  */

      T1 = R23 * *A;
      j  = T1;
      A1 = j;
      A2 = *A - T23 * A1;

/*  Break X into two parts such that X = 2^23 * X1 + X2, compute
    Z = A1 * X2 + A2 * X1  (mod 2^23), and then
    X = 2^23 * Z + A2 * X2  (mod 2^46).                            */

      T1 = R23 * *X;
      j  = T1;
      X1 = j;
      X2 = *X - T23 * X1;
      T1 = A1 * X2 + A2 * X1;
      
      j  = R23 * T1;
      T2 = j;
      Z = T1 - T23 * T2;
      T3 = T23 * Z + A2 * X2;
      j  = R46 * T3;
      T4 = j;
      *X = T3 - T46 * T4;
      return(R46 * *X);
} 



/*****************************************************************/
/************   F  I  N  D  _  M  Y  _  S  E  E  D    ************/
/************                                         ************/
/************ returns parallel random number seq seed ************/
/*****************************************************************/

/*
 * Create a random number sequence of total length nn residing
 * on np number of processors.  Each processor will therefore have a 
 * subsequence of length nn/np.  This routine returns that random 
 * number which is the first random number for the subsequence belonging
 * to processor rank kn, and which is used as seed for proc kn ran # gen.
 */

double   find_my_seed( int  kn,       /* my processor rank, 0<=kn<=num procs */
                       int  np,       /* np = num procs                      */
                       long nn,       /* total num of ran numbers, all procs */
                       double s,      /* Ran num seed, for ex.: 314159265.00 */
                       double a )     /* Ran num gen mult, try 1220703125.00 */
{

  long   i;

  double t1,t2,t3,an;
  long   mq,nq,kk,ik;



      nq = nn / np;

      for( mq=0; nq>1; mq++,nq/=2 )
          ;

      t1 = a;

      for( i=1; i<=mq; i++ )
        t2 = randlc( &t1, &t1 );

      an = t1;

      kk = kn;
      t1 = s;
      t2 = an;

      for( i=1; i<=100; i++ )
      {
        ik = kk / 2;
        if( 2 * ik !=  kk ) 
            t3 = randlc( &t1, &t2 );
        if( ik == 0 ) 
            break;
        t3 = randlc( &t2, &t2 );
        kk = ik;
      }

      return( t1 );

}




/*****************************************************************/
/*************      C  R  E  A  T  E  _  S  E  Q      ************/
/*****************************************************************/

void	create_seq( global_data* gd, double seed, double a )
{
	double x;
	int    i, k;

        k = MAX_KEY/4;

	for (i=0; i<NUM_KEYS; i++)
	{
	    x = randlc(&seed, &a);
	    x += randlc(&seed, &a);
    	    x += randlc(&seed, &a);
	    x += randlc(&seed, &a);  

            gd->key_array[i] = k*x;
	}
}




/*****************************************************************/
/*************    F  U  L  L  _  V  E  R  I  F  Y     ************/
/*****************************************************************/


void full_verify( global_data* gd )
{
    MPI_Status  status;
    MPI_Request request;
    
    INT_TYPE    i, j;
    INT_TYPE    k, last_local_key;

    
/*  Now, finally, sort the keys:  */
    for( i=0; i<gd->total_local_keys; i++ )
        gd->key_array[--gd->key_buff_ptr_global[gd->key_buff2[i]]-
                                 gd->total_lesser_keys] = gd->key_buff2[i];
    last_local_key = (gd->total_local_keys<1)? 0 : (gd->total_local_keys-1);

/*  Send largest key value to next processor  */
    if( gd->my_rank > 0 )
        MPI_Irecv( &k,
                   1,
                   MP_KEY_TYPE,
                   gd->my_rank-1,
                   1000,
                   MPI_COMM_WORLD,
                   &request );                   
    if( gd->my_rank < gd->comm_size-1 )
        MPI_Send( &gd->key_array[last_local_key],
                  1,
                  MP_KEY_TYPE,
                  gd->my_rank+1,
                  1000,
                  MPI_COMM_WORLD );
    if( gd->my_rank > 0 )
        MPI_Wait( &request, &status );

/*  Confirm that neighbor's greatest key value 
    is not greater than my least key value       */              
    j = 0;
    if( gd->my_rank > 0 && gd->total_local_keys > 0 )
        if( k > gd->key_array[0] )
            j++;


/*  Confirm keys correctly sorted: count incorrectly sorted keys, if any */
    for( i=1; i<gd->total_local_keys; i++ )
        if( gd->key_array[i-1] > gd->key_array[i] )
            j++;


    if( j != 0 )
    {
        printf( "Processor %d:  Full_verify: number of keys out of sort: %d\n",
                gd->my_rank, j );
    }
    else
        gd->passed_verification++;
           

}




/*****************************************************************/
/*************             R  A  N  K             ****************/
/*****************************************************************/


void rank( global_data* gd, int iteration )
{

    INT_TYPE    i, k;

    INT_TYPE    shift = MAX_KEY_LOG_2 - NUM_BUCKETS_LOG_2;
    INT_TYPE    key;
    INT_TYPE2   bucket_sum_accumulator, j, m;
    INT_TYPE    local_bucket_sum_accumulator;
    INT_TYPE    min_key_val, max_key_val;
    INT_TYPE    *key_buff_ptr;




/*  Iteration alteration of keys */  
    if(gd->my_rank == 0 )                    
    {
      gd->key_array[iteration] = iteration;
      gd->key_array[iteration+MAX_ITERATIONS] = MAX_KEY - iteration;
    }


/*  Initialize */
    for( i=0; i<NUM_BUCKETS+TEST_ARRAY_SIZE; i++ )  
    {
        gd->bucket_size[i] = 0;
        gd->bucket_size_totals[i] = 0;
        gd->process_bucket_distrib_ptr1[i] = 0;
        gd->process_bucket_distrib_ptr2[i] = 0;
    }


/*  Determine where the partial verify test keys are, load into  */
/*  top of array bucket_size                                     */
    for( i=0; i<TEST_ARRAY_SIZE; i++ )
        if( (gd->test_index_array[i]/NUM_KEYS) == gd->my_rank )
            gd->bucket_size[NUM_BUCKETS+i] = 
                          gd->key_array[gd->test_index_array[i] % NUM_KEYS];


/*  Determine the number of keys in each bucket */
    for( i=0; i<NUM_KEYS; i++ )
        gd->bucket_size[gd->key_array[i] >> shift]++;


/*  Accumulative bucket sizes are the bucket pointers */
    gd->bucket_ptrs[0] = 0;
    for( i=1; i< NUM_BUCKETS; i++ )  
        gd->bucket_ptrs[i] = gd->bucket_ptrs[i-1] + gd->bucket_size[i-1];


/*  Sort into appropriate bucket */
    for( i=0; i<NUM_KEYS; i++ )  
    {
        key = gd->key_array[i];
        gd->key_buff1[gd->bucket_ptrs[key >> shift]++] = key;
    }

#ifdef  TIMING_ENABLED
    timer_stop(gd, 2 );
    timer_start(gd, 3 );
#endif

/*  Get the bucket size totals for the entire problem. These 
    will be used to determine the redistribution of keys      */
    MPI_Allreduce( gd->bucket_size, 
                   gd->bucket_size_totals, 
                   NUM_BUCKETS+TEST_ARRAY_SIZE, 
                   MP_KEY_TYPE,
                   MPI_SUM,
                   MPI_COMM_WORLD );

#ifdef  TIMING_ENABLED
    timer_stop(gd, 3 );
    timer_start(gd, 2 );
#endif

/*  Determine Redistibution of keys: accumulate the bucket size totals 
    till this number surpasses NUM_KEYS (which the average number of keys
    per processor).  Then all keys in these buckets go to processor 0.
    Continue accumulating again until supassing 2*NUM_KEYS. All keys
    in these buckets go to processor 1, etc.  This algorithm guarantees
    that all processors have work ranking; no processors are left idle.
    The optimum number of buckets, however, does not result in as high
    a degree of load balancing (as even a distribution of keys as is
    possible) as is obtained from increasing the number of buckets, but
    more buckets results in more computation per processor so that the
    optimum number of buckets turns out to be 1024 for machines tested.
    Note that process_bucket_distrib_ptr1 and ..._ptr2 hold the bucket
    number of first and last bucket which each processor will have after   
    the redistribution is done.                                          */

    bucket_sum_accumulator = 0;
    local_bucket_sum_accumulator = 0;
    gd->send_displ[0] = 0;
    gd->process_bucket_distrib_ptr1[0] = 0;
    for( i=0, j=0; i<NUM_BUCKETS; i++ )  
    {
        bucket_sum_accumulator       += gd->bucket_size_totals[i];
        local_bucket_sum_accumulator += gd->bucket_size[i];
        if( bucket_sum_accumulator >= (j+1)*NUM_KEYS )  
        {
            gd->send_count[j] = local_bucket_sum_accumulator;
            if( j != 0 )
            {
                gd->send_displ[j] = gd->send_displ[j-1] + gd->send_count[j-1];
                gd->process_bucket_distrib_ptr1[j] = 
                                        gd->process_bucket_distrib_ptr2[j-1]+1;
            }
            gd->process_bucket_distrib_ptr2[j++] = i;
            local_bucket_sum_accumulator = 0;
        }
    }

/*  When NUM_PROCS approaching NUM_BUCKETS, it is highly possible
    that the last few processors don't get any buckets.  So, we
    need to set counts properly in this case to avoid any fallouts.    */
    while( j < gd->comm_size )
    {
        gd->send_count[j] = 0;
        gd->process_bucket_distrib_ptr1[j] = 1;
        j++;
    }

#ifdef  TIMING_ENABLED
    timer_stop(gd, 2 );
    timer_start(gd, 3 ); 
#endif

/*  This is the redistribution section:  first find out how many keys
    each processor will send to every other processor:                 */
    MPI_Alltoall( gd->send_count,
                  1,
                  MPI_INT,
                  gd->recv_count,
                  1,
                  MPI_INT,
                  MPI_COMM_WORLD );

/*  Determine the receive array displacements for the buckets */    
    gd->recv_displ[0] = 0;
    for( i=1; i<gd->comm_size; i++ )
        gd->recv_displ[i] = gd->recv_displ[i-1] + gd->recv_count[i-1];


/*  Now send the keys to respective processors  */    
    MPI_Alltoallv( gd->key_buff1,
                   gd->send_count,
                   gd->send_displ,
                   MP_KEY_TYPE,
                   gd->key_buff2,
                   gd->recv_count,
                   gd->recv_displ,
                   MP_KEY_TYPE,
                   MPI_COMM_WORLD );

#ifdef  TIMING_ENABLED
    timer_stop(gd, 3 ); 
    timer_start(gd, 2 );
#endif

/*  The starting and ending bucket numbers on each processor are
    multiplied by the interval size of the buckets to obtain the 
    smallest possible min and greatest possible max value of any 
    key on each processor                                          */
    min_key_val = gd->process_bucket_distrib_ptr1[gd->my_rank] << shift;
    max_key_val = ((gd->process_bucket_distrib_ptr2[gd->my_rank] + 1) << shift)-1;

/*  Clear the work array */
    for( i=0; i<max_key_val-min_key_val+1; i++ )
        gd->key_buff1[i] = 0;

/*  Determine the total number of keys on all other 
    processors holding keys of lesser value         */
    m = 0;
    for( k=0; k<gd->my_rank; k++ )
        for( i= gd->process_bucket_distrib_ptr1[k];
             i<=gd->process_bucket_distrib_ptr2[k];
             i++ )  
            m += gd->bucket_size_totals[i]; /*  m has total # of lesser keys */

/*  Determine total number of keys on this processor */
    j = 0;                                 
    for( i= gd->process_bucket_distrib_ptr1[gd->my_rank];
         i<=gd->process_bucket_distrib_ptr2[gd->my_rank];
         i++ )  
        j += gd->bucket_size_totals[i];     /* j has total # of local keys   */


/*  Ranking of all keys occurs in this section:                 */
/*  shift it backwards so no subtractions are necessary in loop */
    key_buff_ptr = gd->key_buff1 - min_key_val;

/*  In this section, the keys themselves are used as their 
    own indexes to determine how many of each there are: their
    individual population                                       */
    for( i=0; i<j; i++ )
        key_buff_ptr[gd->key_buff2[i]]++;  /* Now they have individual key   */
                                       /* population                     */

/*  To obtain ranks of each key, successively add the individual key
    population, not forgetting the total of lesser keys, m.
    NOTE: Since the total of lesser keys would be subtracted later 
    in verification, it is no longer added to the first key population 
    here, but still needed during the partial verify test.  This is to 
    ensure that 32-bit key_buff can still be used for class D.           */
/*    key_buff_ptr[min_key_val] += m;    */
    for( i=min_key_val; i<max_key_val; i++ )   
        key_buff_ptr[i+1] += key_buff_ptr[i];  


/* This is the partial verify test section */
/* Observe that test_rank_array vals are   */
/* shifted differently for different cases */
    for( i=0; i<TEST_ARRAY_SIZE; i++ )
    {                                             
        k = gd->bucket_size_totals[i+NUM_BUCKETS];    /* Keys were hidden here */
        if( min_key_val <= k  &&  k <= max_key_val )
        {
            /* Add the total of lesser keys, m, here */
            INT_TYPE2 key_rank = key_buff_ptr[k-1] + m;
            int failed = 0;

            switch( CLASS )
            {
                case 'S':
                    if( i <= 2 )
                    {
                        if( key_rank != gd->test_rank_array[i]+iteration )
                            failed = 1;
                        else
                            gd->passed_verification++;
                    }
                    else
                    {
                        if( key_rank != gd->test_rank_array[i]-iteration )
                            failed = 1;
                        else
                            gd->passed_verification++;
                    }
                    break;
                case 'W':
                    if( i < 2 )
                    {
                        if( key_rank != gd->test_rank_array[i]+(iteration-2) )
                            failed = 1;
                        else
                            gd->passed_verification++;
                    }
                    else
                    {
                        if( key_rank != gd->test_rank_array[i]-iteration )
                            failed = 1;
                        else
                            gd->passed_verification++;
                    }
                    break;
                case 'A':
                    if( i <= 2 )
        	    {
                        if( key_rank != gd->test_rank_array[i]+(iteration-1) )
                            failed = 1;
                        else
                          gd->passed_verification++;
        	    }
                    else
                    {
                        if( key_rank !=  gd->test_rank_array[i]-(iteration-1) )
                            failed = 1;
                        else
                            gd->passed_verification++;
                    }
                    break;
                case 'B':
                    if( i == 1 || i == 2 || i == 4 )
        	    {
                        if( key_rank != gd->test_rank_array[i]+iteration )
                            failed = 1;
                        else
                            gd->passed_verification++;
        	    }
                    else
                    {
                        if( key_rank != gd->test_rank_array[i]-iteration )
                            failed = 1;
                        else
                            gd->passed_verification++;
                    }
                    break;
                case 'C':
                    if( i <= 2 )
        	    {
                        if( key_rank != gd->test_rank_array[i]+iteration )
                            failed = 1;
                        else
                            gd->passed_verification++;
        	    }
                    else
                    {
                        if( key_rank != gd->test_rank_array[i]-iteration )
                            failed = 1;
                        else
                            gd->passed_verification++;
                    }
                    break;
                case 'D':
                    if( i < 2 )
        	    {
                        if( key_rank != gd->test_rank_array[i]+iteration )
                            failed = 1;
                        else
                            gd->passed_verification++;
        	    }
                    else
                    {
                        if( key_rank != gd->test_rank_array[i]-iteration )
                            failed = 1;
                        else
                            gd->passed_verification++;
                    }
                    break;
            }
            if( failed == 1 )
                printf( "Failed partial verification: "
                        "iteration %d, processor %d, test key %d\n", 
                         iteration, gd->my_rank, (int)i );
        }
    }




/*  Make copies of rank info for use by full_verify: these variables
    in rank are local; making them global slows down the code, probably
    since they cannot be made register by compiler                        */

    if( iteration == MAX_ITERATIONS ) 
    {
        gd->key_buff_ptr_global = key_buff_ptr;
        gd->total_local_keys    = j;
        gd->total_lesser_keys   = 0;  /* no longer set to 'm', see note above */
    }

}      


/*****************************************************************/
/*************             M  A  I  N             ****************/
/*****************************************************************/

int main( int argc, char **argv )
{

    int             i, iteration, itemp;

    double          timecounter, maxtime;

    global_data* gd = malloc(sizeof(global_data));
/*  Initialize MPI */
    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &gd->my_rank );
    MPI_Comm_size( MPI_COMM_WORLD, &gd->comm_size );

/*  Initialize the verification arrays if a valid class */
    for( i=0; i<TEST_ARRAY_SIZE; i++ )
        switch( CLASS )
        {
            case 'S':
                gd->test_index_array[i] = S_test_index_array[i];
                gd->test_rank_array[i]  = S_test_rank_array[i];
                break;
            case 'A':
                gd->test_index_array[i] = A_test_index_array[i];
                gd->test_rank_array[i]  = A_test_rank_array[i];
                break;
            case 'W':
                gd->test_index_array[i] = W_test_index_array[i];
                gd->test_rank_array[i]  = W_test_rank_array[i];
                break;
            case 'B':
                gd->test_index_array[i] = B_test_index_array[i];
                gd->test_rank_array[i]  = B_test_rank_array[i];
                break;
            case 'C':
                gd->test_index_array[i] = C_test_index_array[i];
                gd->test_rank_array[i]  = C_test_rank_array[i];
                break;
            case 'D':
                gd->test_index_array[i] = D_test_index_array[i];
                gd->test_rank_array[i]  = D_test_rank_array[i];
                break;
        };

        

/*  Printout initial NPB info */
    if( gd->my_rank == 0 )
    {
        printf( "\n\n NAS Parallel Benchmarks 3.3 -- IS Benchmark\n\n" );
        printf( " Size:  %ld  (class %c)\n", (long)TOTAL_KEYS*MIN_PROCS, CLASS );
        printf( " Iterations:   %d\n", MAX_ITERATIONS );
        printf( " Number of processes:     %d\n",gd->comm_size );
    }

/*  Check that actual and compiled number of processors agree */
    if( gd->comm_size != NUM_PROCS )
    {
        if( gd->my_rank == 0 )
            printf( "\n ERROR: compiled for %d processes\n"
                    " Number of active processes: %d\n"
                    " Exiting program!\n\n", NUM_PROCS, gd->comm_size );
        MPI_Finalize();
        exit( 1 );
    }

/*  Check to see whether total number of processes is within bounds.
    This could in principle be checked in setparams.c, but it is more
    convenient to do it here                                               */
    if( gd->comm_size < MIN_PROCS || gd->comm_size > MAX_PROCS)
    {
       if( gd->my_rank == 0 )
           printf( "\n ERROR: number of processes %d not within range %d-%d"
                   "\n Exiting program!\n\n", gd->comm_size, MIN_PROCS, MAX_PROCS);
       MPI_Finalize();
       exit( 1 );
    }


/*  Generate random number sequence and subsequent keys on all procs */
    create_seq(gd,  find_my_seed( gd->my_rank, 
                              gd->comm_size, 
                              4*(long)TOTAL_KEYS*MIN_PROCS,
                              314159265.00,      /* Random number gen seed */
                              1220703125.00 ),   /* Random number gen mult */
                1220703125.00 );                 /* Random number gen mult */

/*  Do one interation for free (i.e., untimed) to guarantee initialization of  
    all data and code pages and respective tables */
    rank(gd, 1 );  

/*  Start verification counter */
    gd->passed_verification = 0;

    if( gd->my_rank == 0 && CLASS != 'S' ) printf( "\n   iteration\n" );

/*  Initialize timer  */             
    timer_clear(gd, 0 );

/*  Initialize separate communication, computation timing */
#ifdef  TIMING_ENABLED 
    for( i=1; i<=3; i++ ) timer_clear(gd, i );
#endif

/*  Start timer  */             
    timer_start(gd, 0 );

#ifdef  TIMING_ENABLED
    timer_start(gd, 1 );
    timer_start(gd, 2 );
#endif

/*  This is the main iteration */
    for( iteration=1; iteration<=MAX_ITERATIONS; iteration++ )
    {
        if( gd->my_rank == 0 && CLASS != 'S' ) printf( "        %d\n", iteration );
        rank(gd,  iteration );
    }


#ifdef  TIMING_ENABLED
    timer_stop(gd, 2 );
    timer_stop(gd, 1 );
#endif

/*  Stop timer, obtain time for processors */
    timer_stop(gd, 0 );

    timecounter = timer_read(gd, 0 );

/*  End of timing, obtain maximum time of all processors */
    MPI_Reduce( &timecounter,
                &maxtime,
                1,
                MPI_DOUBLE,
                MPI_MAX,
                0,
                MPI_COMM_WORLD );

#ifdef  TIMING_ENABLED
    {
        double    tmin, tsum, tmax;
    
        if( my_rank == 0 )
        {
            printf( "\ntimer 1/2/3 = total/computation/communication time\n");
            printf( "              min                avg                max\n" );
        }
        for( i=1; i<=3; i++ )
        {
            timecounter = timer_read(gd, i );
            MPI_Reduce( &timecounter,
                        &tmin,
                        1,
                        MPI_DOUBLE,
                        MPI_MIN,
                        0,
                        MPI_COMM_WORLD );
            MPI_Reduce( &timecounter,
                        &tsum,
                        1,
                        MPI_DOUBLE,
                        MPI_SUM,
                        0,
                        MPI_COMM_WORLD );
            MPI_Reduce( &timecounter,
                        &tmax,
                        1,
                        MPI_DOUBLE,
                        MPI_MAX,
                        0,
                        MPI_COMM_WORLD );
            if( my_rank == 0 )
                printf( "timer %d:    %f           %f            %f\n",
                        i, tmin, tsum/((double) comm_size), tmax );
        }
        if( my_rank == 0 )
            printf( "\n" );
    }
#endif

/*  This tests that keys are in sequence: sorting of last ranked key seq
    occurs here, but is an untimed operation                             */
    full_verify(gd);


/*  Obtain verification counter sum */
    itemp =gd->passed_verification;
    MPI_Reduce( &itemp,
                &gd->passed_verification,
                1,
                MPI_INT,
                MPI_SUM,
                0,
                MPI_COMM_WORLD );



/*  The final printout  */
    if( gd->my_rank == 0 )
    {
        if( gd->passed_verification != 5*MAX_ITERATIONS + gd->comm_size )
            gd->passed_verification = 0;
        c_print_results( "IS",
                         CLASS,
                         (int)(TOTAL_KEYS),
                         MIN_PROCS,
                         0,
                         MAX_ITERATIONS,
                         NUM_PROCS,
                         gd->comm_size,
                         maxtime,
                         ((double) (MAX_ITERATIONS)*TOTAL_KEYS*MIN_PROCS)
                                                      /maxtime/1000000.,
                         "keys ranked", 
                         gd->passed_verification,
                         NPBVERSION,
                         COMPILETIME,
                         MPICC,
                         CLINK,
                         CMPI_LIB,
                         CMPI_INC,
                         CFLAGS,
                         CLINKFLAGS );
    }
                    
    MPI_Finalize();
    free(gd);

    return 0;
         /**************************/
}        /*  E N D  P R O G R A M  */
         /**************************/
