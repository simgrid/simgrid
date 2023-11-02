#define _REENTRANT

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

// The maximum number of customer threads.
#define MAX_CUSTOMERS 10

// Define the semaphores.
sem_t waitingRoom;
sem_t barberChair;
sem_t barberPillow;
sem_t seatBelt;

// Flag to stop the barber thread when all customers have been serviced.
int allDone = 0;
int DEBUG = 0;

static void *customer(void *number) {
    int num = *(int *)number;

    if(DEBUG) printf("Customer %d leaving for barber shop.\n", num);
    if(DEBUG) printf("Customer %d arrived at barber shop.\n", num);

    sem_wait(&waitingRoom);

    if(DEBUG) printf("Customer %d entering waiting room.\n", num);

    sem_wait(&barberChair);

    if(DEBUG) printf("Customer %d waking the barber.\n", num);
    sem_post(&barberPillow);

    sem_wait(&seatBelt);

    sem_post(&barberChair);
    if(DEBUG) printf("Customer %d leaving barber shop.\n", num);
    return NULL;
}

static void *barber(void *junk) {
    while (!allDone) {
        if(DEBUG) printf("The barber is sleeping\n");
        sem_wait(&barberPillow);

        if (!allDone) {
            if(DEBUG) printf("The barber is cutting hair\n");
            if(DEBUG) printf("The barber has finished cutting hair.\n");
            sem_post(&seatBelt);
        }
        else {
            if(DEBUG) printf("The barber is going home for the day.\n");
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if(argc != 5){
        printf("Usage: %s numCustomers numChairs RandSeed DEBUG\n", argv[0]);
        return 1;
    }

    pthread_t btid;
    pthread_t tid[MAX_CUSTOMERS];
    int i, numCustomers, numChairs;
    long RandSeed;
    int Number[MAX_CUSTOMERS];

    numCustomers = atoi(argv[1]);
    numChairs = atoi(argv[2]);
    RandSeed = atol(argv[3]);
    DEBUG = atoi(argv[4]);

    if (numCustomers > MAX_CUSTOMERS) {
        printf("The maximum number of Customers is %d.\n", MAX_CUSTOMERS);
        exit(-1);
    }

    srand48(RandSeed);

    for (i=0; i<MAX_CUSTOMERS; i++) {
        Number[i] = i;
    }

    sem_init(&waitingRoom, 0, numChairs);
    sem_init(&barberChair, 0, 1);
    sem_init(&barberPillow, 0, 0);
    sem_init(&seatBelt, 0, 0);

    pthread_create(&btid, NULL, barber, NULL);

    for (i=0; i<numCustomers; i++) {
        pthread_create(&tid[i], NULL, customer, (void *)&Number[i]);
    }

    for (i=0; i<numCustomers; i++) {
        pthread_join(tid[i],NULL);
    }

    allDone = 1;
    sem_post(&barberPillow);
    pthread_join(btid,NULL);
}
