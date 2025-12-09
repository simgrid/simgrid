/* File:     pth_pi_mutex.c
 * Purpose:  Estimate pi using the Maclaurin series for arctan(x):
 *
 *              arctan(x) = Sum_n=0 ^infty (-1)^n x^(2n+1)/(2n+1), |x| <= 1
 *
 *           Since arctan(1) = pi/4, we can compute
 *
 *              pi = 4*[1 - 1/3 + 1/5 - 1/7 + 1/9 - . . . ]
 *
 * Compile:  gcc -g -Wall -o pth_pi_mutex pth_pi_mutex.c timer.c -lm -lpthread
 * Run:      pth_pi_mutex <number of threads> <n>
 *           n is the number of terms of the Maclaurin series to use
 *           n should be evenly divisible by the number of threads
 * 
 * Input:    none            
 * Output:   The estimate of pi using multiple threads, one thread, and the 
 *           value computed by the math library arctan function
 *           Also elapsed times for the multithreaded and singlethreaded
 *           computations.
 *
 * Notes:
 *    1.  The radius of convergence for the series is only 1.  So the 
 *        series converges quite slowly.
 *    2.  This version uses a local sum on each thread and a mutex to
 *        control access to the global sum.
 */        

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <sys/time.h>

#define GET_TIME(now) { \
   struct timeval t; \
   gettimeofday(&t, NULL); \
   now = t.tv_sec + t.tv_usec/1000000.0; \
}

long thread_count;
long long n;
double sum;
pthread_mutex_t mutex;

void* Thread_sum(void* rank);

/* Only executed by main thread */
void Get_args(int argc, char* argv[]);
void Usage(char* prog_name);
double Serial_pi(long long n);

int main(int argc, char* argv[]) {
   long       thread;  /* Use long in case of a 64-bit system */
   pthread_t* thread_handles;
//   double start, finish;

   /* Get number of threads from command line */
   Get_args(argc, argv);

   thread_handles = (pthread_t*) malloc (thread_count*sizeof(pthread_t)); 
   pthread_mutex_init(&mutex, NULL);

//   GET_TIME(start);
   sum = 0.0;
   for (thread = 0; thread < thread_count; thread++)  
      pthread_create(&thread_handles[thread], NULL,
          Thread_sum, (void*)thread);  

   for (thread = 0; thread < thread_count; thread++) 
      pthread_join(thread_handles[thread], NULL); 
   sum = 4.0*sum;
//   GET_TIME(finish);

//   printf("With n = %lld terms,\n", n);
//   printf("   Our estimate of pi = %.15f\n", sum);
//   printf("The elapsed time is %e seconds\n", finish - start);
//   GET_TIME(start);
   sum = Serial_pi(n);
//   GET_TIME(finish);
//   printf("   Single thread est  = %.15f\n", sum);
//   printf("The elapsed time is %e seconds\n", finish - start);
//   printf("                   pi = %.15f\n", 4.0*atan(1.0));
   
   pthread_mutex_destroy(&mutex);
   free(thread_handles);
   return 0;
}  /* main */

/*------------------------------------------------------------------*/
void* Thread_sum(void* rank) {
   long my_rank = (long) rank;
   double factor;
   long long i;
   long long my_n = n/thread_count;
   long long my_first_i = my_n*my_rank;
   long long my_last_i = my_first_i + my_n;
   double my_sum = 0.0;

   if (my_first_i % 2 == 0)
      factor = 1.0;
   else
      factor = -1.0;

   for (i = my_first_i; i < my_last_i; i++, factor = -factor) {
      my_sum += factor/(2*i+1);
   }
   pthread_mutex_lock(&mutex);
#  ifdef VERBOSE
   printf("Thread %ld > Obtained mutex\n", my_rank);
   fflush(stdout);
#  endif
   sum += my_sum;
   pthread_mutex_unlock(&mutex);

   return NULL;
}  /* Thread_sum */

/*------------------------------------------------------------------
 * Function:   Serial_pi
 * Purpose:    Estimate pi using 1 thread
 * In arg:     n
 * Return val: Estimate of pi using n terms of Maclaurin series
 */
double Serial_pi(long long n) {
   double sum = 0.0;
   long long i;
   double factor = 1.0;

   for (i = 0; i < n; i++, factor = -factor) {
      sum += factor/(2*i+1);
   }
   return 4.0*sum;

}  /* Serial_pi */

/*------------------------------------------------------------------
 * Function:    Get_args
 * Purpose:     Get the command line args
 * In args:     argc, argv
 * Globals out: thread_count, n
 */
void Get_args(int argc, char* argv[]) {

#ifdef THREADS
   thread_count = THREADS;
   n = ITERS;
   //printf ("WARNING: fixing thread_count %ld and n %lld\n", thread_count, n);
   // this is because nidhugg does not support commandline arguments
#else
   if (argc != 3) Usage(argv[0]);
   thread_count = strtol(argv[1], NULL, 10);  
   if (thread_count <= 0) Usage(argv[0]);
   n = strtoll(argv[2], NULL, 10);
   if (n <= 0) Usage(argv[0]);
#endif
}  /* Get_args */

/*------------------------------------------------------------------
 * Function:  Usage
 * Purpose:   Print a message explaining how to run the program
 * In arg:    prog_name
 */
void Usage(char* prog_name) {
   fprintf(stderr, "usage: %s <number of threads> <n>\n", prog_name);
   fprintf(stderr, "   n is the number of terms and should be >= 1\n");
   fprintf(stderr, "   n should be evenly divisible by the number of threads\n");
   exit(0);
}  /* Usage */
