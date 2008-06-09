#include "surf/random_mgr.h"
#include "xbt/sysdep.h"

#ifdef WIN32
static double drand48(void)
{
   THROW_UNIMPLEMENTED;
   return -1;
}

static double rand_r(unsigned int* seed)
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
    default: 
      return drand48();
   }
}

/* Generate numbers between min and max with a given mean and standard deviation */
double random_generate(random_data_t random) {
  double a, b;
  double alpha, beta, gamma;
  double U1, U2, V, W, X;

  if (random == NULL) return 0.0f;

  a = random->mean * ( random->mean * (1 - random->mean) / (random->std*random->std) - 1 );
  b = (1 - random->mean) * ( random->mean * (1 - random->mean) / (random->std*random->std) - 1 );

  alpha = a + b;
  if (a <= 1. || b <= 1.)
    beta = ((1./a)>(1./b))?(1./a):(1./b);
  else
    beta = sqrt ((alpha-2.) / (2.*a*b - alpha));
  gamma = a + 1./beta;

  do {
    /* Random generation for the Beta distribution based on
     *   R. C. H. Cheng (1978). Generating beta variates with nonintegral shape parameters. _Communications of the ACM_, *21*, 317-322.
     *   It is good for speed because it does not call math functions many times and respect the 4 given constraints
     */
    U1 = custom_random(random->generator,&(random->seed));
    U2 = custom_random(random->generator,&(random->seed));

    V = beta * log(U1/(1-U1));
    W = a * exp(V);
  } while (alpha * log(alpha/(b + W)) + gamma*V - log(4) < log(U1*U1*U2));

  X = W / (b + W);

  return X * (random->max - random->min) + random->min;
}

random_data_t random_new(Generator generator, long int seed,
			 double min, double max, 
			 double mean, double std){
  random_data_t random = xbt_new0(s_random_data_t, 1);
   
  random->generator = generator;
  random->seed = seed;   
  random->min = min;
  random->max = max;

  /* Check user stupidities */
  if (max < min)
     THROW2(arg_error,0,"random->max < random->min (%f < %f)",max, min);
  if (mean < min)
     THROW2(arg_error,0,"random->mean < random->min (%f < %f)",mean, min);
  if (mean > max)
     THROW2(arg_error,0,"random->mean > random->max (%f > %f)",mean, max);

  /* normalize the mean and standard deviation before storing */
  random->mean = (mean - min) / (max - min);
  random->std = std / (max - min);

  if (random->mean * (1-random->mean) < random->std*random->std) 
     THROW2(arg_error,0,"Invalid mean and standard deviation (%f and %f)",random->mean, random->std);
   
  return random;
}
