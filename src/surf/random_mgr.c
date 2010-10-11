/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf/random_mgr.h"
#include "xbt/sysdep.h"
#include "simgrid_config.h" /*_XBT_WIN32*/

#ifdef _XBT_WIN32

static unsigned int _seed = 2147483647;

#ifdef __VISUALC__
typedef unsigned __int64 uint64_t;
typedef unsigned int uint32_t;
#endif

struct drand48_data {
  unsigned short int __x[3];    /* Current state.  */
  unsigned short int __old_x[3];        /* Old state.  */
  unsigned short int __c;       /* Additive const. in congruential formula.  */
  unsigned short int __init;    /* Flag for initializing.  */
  unsigned long long int __a;   /* Factor in congruential formula.  */
};

static struct drand48_data __libc_drand48_data = { 0 };

union ieee754_double {
  double d;

  /* This is the IEEE 754 double-precision format.  */
  struct {
    /* Together these comprise the mantissa.  */
    unsigned int mantissa1:32;
    unsigned int mantissa0:20;
    unsigned int exponent:11;
    unsigned int negative:1;
    /* Little endian.  */
  } ieee;

  /* This format makes it easier to see if a NaN is a signalling NaN.  */
  struct {
    /* Together these comprise the mantissa.  */
    unsigned int mantissa1:32;
    unsigned int mantissa0:19;
    unsigned int quiet_nan:1;
    unsigned int exponent:11;
    unsigned int negative:1;

  } ieee_nan;
};

#define IEEE754_DOUBLE_BIAS	0x3ff   /* Added to exponent.  */

double drand48(void);

int
_drand48_iterate(unsigned short int xsubi[3], struct drand48_data *buffer);

int
_erand48_r(unsigned short int xsubi[3], struct drand48_data *buffer,
           double *result);


int
_erand48_r(unsigned short int xsubi[3], struct drand48_data *buffer,
           double *result)
{
  union ieee754_double temp;

  /* Compute next state.  */
  if (_drand48_iterate(xsubi, buffer) < 0)
    return -1;

  /* Construct a positive double with the 48 random bits distributed over
     its fractional part so the resulting FP number is [0.0,1.0).  */

  temp.ieee.negative = 0;
  temp.ieee.exponent = IEEE754_DOUBLE_BIAS;
  temp.ieee.mantissa0 = (xsubi[2] << 4) | (xsubi[1] >> 12);
  temp.ieee.mantissa1 = ((xsubi[1] & 0xfff) << 20) | (xsubi[0] << 4);

  /* Please note the lower 4 bits of mantissa1 are always 0.  */
  *result = temp.d - 1.0;

  return 0;
}

int _drand48_iterate(unsigned short int xsubi[3],
                     struct drand48_data *buffer)
{
  uint64_t X;
  uint64_t result;

  /* Initialize buffer, if not yet done.  */

  if (buffer->__init == 0) {
    buffer->__a = 0x5deece66dull;
    buffer->__c = 0xb;
    buffer->__init = 1;
  }

  /* Do the real work.  We choose a data type which contains at least
     48 bits.  Because we compute the modulus it does not care how
     many bits really are computed.  */

  X = (uint64_t) xsubi[2] << 32 | (uint32_t) xsubi[1] << 16 | xsubi[0];

  result = X * buffer->__a + buffer->__c;


  xsubi[0] = result & 0xffff;
  xsubi[1] = (result >> 16) & 0xffff;
  xsubi[2] = (result >> 32) & 0xffff;

  return 0;
}


double _drand48(void)
{
  double result;

  (void) _erand48_r(__libc_drand48_data.__x, &__libc_drand48_data,
                    &result);

  return result;
}

void _srand(unsigned int seed)
{
  _seed = seed;
}

int _rand(void)
{
  const long a = 16807;
  const long m = 2147483647;
  const long q = 127773;        /* (m/a) */
  const long r = 2836;          /* (m%a) */

  long lo, k, s;

  s = (long) _seed;

  k = (long) (s / q);

  lo = (s - q * k);

  s = a * lo - r * k;

  if (s <= 0)
    s += m;

  _seed = (int) (s & RAND_MAX);

  return _seed;
}

int _rand_r(unsigned int *pseed)
{
  const long a = 16807;
  const long m = 2147483647;
  const long q = 127773;        /* (m/a) */
  const long r = 2836;          /* (m%a) */

  long lo, k, s;

  s = (long) *pseed;

  k = (long) (s / q);

  lo = (s - q * k);

  s = a * lo - r * k;

  if (s <= 0)
    s += m;

  return (int) (s & RAND_MAX);

}


#define rand_r _rand_r
#define drand48 _drand48

#endif

static double custom_random(Generator generator, long int *seed)
{
  switch (generator) {

  case DRAND48:
    return drand48();
  case RAND:
    return (double) rand_r((unsigned int *) seed) / RAND_MAX;
  default:
    return drand48();
  }
}

/* Generate numbers between min and max with a given mean and standard deviation */
double random_generate(random_data_t random)
{
  double a, b;
  double alpha, beta, gamma;
  double U1, U2, V, W, X;

  if (random == NULL)
    return 0.0f;

  if (random->std == 0)
    return random->mean * (random->max - random->min) + random->min;

  a = random->mean * (random->mean * (1 - random->mean) /
                      (random->std * random->std) - 1);
  b = (1 -
       random->mean) * (random->mean * (1 -
                                        random->mean) / (random->std *
                                                         random->std) - 1);

  alpha = a + b;
  if (a <= 1. || b <= 1.)
    beta = ((1. / a) > (1. / b)) ? (1. / a) : (1. / b);
  else
    beta = sqrt((alpha - 2.) / (2. * a * b - alpha));
  gamma = a + 1. / beta;

  do {
    /* Random generation for the Beta distribution based on
     *   R. C. H. Cheng (1978). Generating beta variates with nonintegral shape parameters. _Communications of the ACM_, *21*, 317-322.
     *   It is good for speed because it does not call math functions many times and respect the 4 given constraints
     */
    U1 = custom_random(random->generator, &(random->seed));
    U2 = custom_random(random->generator, &(random->seed));

    V = beta * log(U1 / (1 - U1));
    W = a * exp(V);
  } while (alpha * log(alpha / (b + W)) + gamma * V - log(4) <
           log(U1 * U1 * U2));

  X = W / (b + W);

  return X * (random->max - random->min) + random->min;
}

random_data_t random_new(Generator generator, long int seed,
                         double min, double max, double mean, double std)
{
  random_data_t random = xbt_new0(s_random_data_t, 1);

  random->generator = generator;
  random->seed = seed;
  random->min = min;
  random->max = max;

  /* Check user stupidities */
  if (max < min)
    THROW2(arg_error, 0, "random->max < random->min (%f < %f)", max, min);
  if (mean < min)
    THROW2(arg_error, 0, "random->mean < random->min (%f < %f)", mean,
           min);
  if (mean > max)
    THROW2(arg_error, 0, "random->mean > random->max (%f > %f)", mean,
           max);

  /* normalize the mean and standard deviation before storing */
  random->mean = (mean - min) / (max - min);
  random->std = std / (max - min);

  if (random->mean * (1 - random->mean) < random->std * random->std)
    THROW2(arg_error, 0, "Invalid mean and standard deviation (%f and %f)",
           random->mean, random->std);

  return random;
}
