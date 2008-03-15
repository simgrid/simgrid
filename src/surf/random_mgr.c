
#include "surf/random_mgr.h"
#include "xbt/sysdep.h"

#ifdef WIN32
static double drand48(void)
{
   THROW_UNIMPLEMENTED;
   return -1;
}
#endif

static double custom_random(Generator generator, long int *seed){
   switch(generator) {
   case DRAND48:
     return drand48(); 	
   case RAND: 
     return (double)rand_r((unsigned int*)seed)/RAND_MAX; 
   default: return drand48();
   }
}

/* Generate numbers between min and max with a given mean and standard deviation */
double random_generate(random_data_t random){  
  double x1, x2, w, y;
  
  if (random == NULL) return 0.0f;  

  do {
    /* Apply the polar form of the Box-Muller Transform to map the two uniform random numbers to a pair of numbers from a normal distribution.
       It is good for speed because it does not call math functions many times. Another way would be to simply:
         y1 = sqrt( - 2 * log(x1) ) * cos( 2 * pi * x2 )
    */ 
    do {
      x1 = 2.0 * custom_random(random->generator,&(random->seed)) - 1.0;
      x2 = 2.0 * custom_random(random->generator,&(random->seed)) - 1.0;
      w = x1 * x1 + x2 * x2;
    } while ( w >= 1.0 );

    w = sqrt( (-2.0 * log( w ) ) / w );
    y = x1 * w;

    /* Multiply the Box-Muller value by the standard deviation and add the mean */
    y = y * random->stdDeviation + random->mean;
  } while (!(random->min <= y && y <= random->max));

  return y;
}

random_data_t random_new(Generator generator, long int seed, 
			 double min, double max, double mean, 
			 double stdDeviation){
  random_data_t random = xbt_new0(s_random_data_t, 1);
  random->generator = generator;
  random->seed = seed;
  random->min = min;
  random->max = max;
  random->mean = mean;
  random->stdDeviation = stdDeviation;
  return random;
}

