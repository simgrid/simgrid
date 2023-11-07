#ifdef _REENTRANT
#undef _REENTRANT
#endif
#define _REENTRANT

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// The maximum number of customer threads.
#define MAX_CUSTOMERS 25

// Define the semaphores.

// waitingRoom Limits the # of customers allowed
// to enter the waiting room at one time.
sem_t waitingRoom;

// barberChair ensures mutually exclusive access to
// the barber chair.
sem_t barberChair;

// barberPillow is used to allow the barber to sleep
// until a customer arrives.
sem_t barberPillow;

// seatBelt is used to make the customer to wait until
// the barber is done cutting his/her hair.
sem_t seatBelt;

// Flag to stop the barber thread when all customers
// have been serviced.
int allDone = 0;
int DEBUG   = 0;

static void randwait(int secs)
{
  int len;

  // Generate a random number...
  len = (int)((drand48() * secs) + 1);
  sleep(len);
}

static void* customer(void* number)
{
  int num = *(int*)number;

  // Leave for the shop and take some random amount of
  // time to arrive.
  if (DEBUG)
    printf("Customer %d leaving for barber shop.\n", num);
  randwait(5);
  if (DEBUG)
    printf("Customer %d arrived at barber shop.\n", num);

  // Wait for space to open up in the waiting room...
  sem_wait(&waitingRoom);
  if (DEBUG)
    printf("Customer %d entering waiting room.\n", num);

  // Wait for the barber chair to become free.
  sem_wait(&barberChair);

  // The chair is free so give up your spot in the
  // waiting room.
  sem_post(&waitingRoom);

  // Wake up the barber...
  if (DEBUG)
    printf("Customer %d waking the barber.\n", num);
  sem_post(&barberPillow);

  // Wait for the barber to finish cutting your hair.
  sem_wait(&seatBelt);

  // Give up the chair.
  sem_post(&barberChair);
  if (DEBUG)
    printf("Customer %d leaving barber shop.\n", num);
  return NULL;
}

static void* barber(void* junk)
{
  // While there are still customers to be serviced...
  // Our barber is omnicient and can tell if there are
  // customers still on the way to his shop.
  while (!allDone) {

    // Sleep until someone arrives and wakes you..
    if (DEBUG)
      printf("The barber is sleeping\n");
    sem_wait(&barberPillow);

    // Skip this stuff at the end...
    if (!allDone) {

      // Take a random amount of time to cut the
      // customer's hair.
      if (DEBUG)
        printf("The barber is cutting hair\n");
      randwait(3);
      if (DEBUG)
        printf("The barber has finished cutting hair.\n");

      // Release the customer when done cutting...
      sem_post(&seatBelt);
    } else {
      if (DEBUG)
        printf("The barber is going home for the day.\n");
    }
  }
  return NULL;
}

int main(int argc, char* argv[])
{
  pthread_t btid;
  pthread_t tid[MAX_CUSTOMERS];
  long RandSeed;
  int i, numCustomers, numChairs;
  int Number[MAX_CUSTOMERS];

  // Check to make sure there are the right number of
  // command line arguments.
  if (argc != 5) {
    printf("Use: SleepBarber <Num Customers> <Num Chairs> <rand seed> <DEBUG>\n");
    exit(-1);
  }

  // Get the command line arguments and convert them
  // into integers.
  numCustomers = atoi(argv[1]);
  numChairs    = atoi(argv[2]);
  RandSeed     = atol(argv[3]);
  DEBUG        = atoi(argv[4]);

  // Make sure the number of threads is less than the number of
  // customers we can support.
  if (numCustomers > MAX_CUSTOMERS) {
    printf("The maximum number of Customers is %d.\n", MAX_CUSTOMERS);
    exit(-1);
  }

  if (DEBUG) {
    printf("\nSleepBarber.c\n\n");
    printf("A solution to the sleeping barber problem using semaphores.\n");
  }

  // Initialize the random number generator with a new seed.
  srand48(RandSeed);

  // Initialize the numbers array.
  for (i = 0; i < MAX_CUSTOMERS; i++) {
    Number[i] = i;
  }

  // Initialize the semaphores with initial values...
  sem_init(&waitingRoom, 0, numChairs);
  sem_init(&barberChair, 0, 1);
  sem_init(&barberPillow, 0, 0);
  sem_init(&seatBelt, 0, 0);

  // Create the barber.
  pthread_create(&btid, NULL, barber, NULL);

  // Create the customers.
  for (i = 0; i < numCustomers; i++) {
    pthread_create(&tid[i], NULL, customer, (void*)&Number[i]);
  }

  // Join each of the threads to wait for them to finish.
  for (i = 0; i < numCustomers; i++) {
    pthread_join(tid[i], NULL);
  }

  // When all of the customers are finished, kill the
  // barber thread.
  allDone = 1;
  sem_post(&barberPillow); // Wake the barber so he will exit.
  pthread_join(btid, NULL);
}
