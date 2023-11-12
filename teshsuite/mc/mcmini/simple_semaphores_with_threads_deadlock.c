#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>

int START_NUM;
int DEBUG = 0;

sem_t sem1, sem2;
pthread_t thread1, thread2;

static void * thread1_doit(void *forks_arg) {
    sem_wait(&sem2);
    sem_wait(&sem2);
    if(DEBUG) printf("Thread 1: Posted to sem1\n");
    sem_post(&sem1);
    return NULL;
}

static void * thread2_doit(void *forks_arg) {
    for( int i = 0; i < START_NUM+1; i++) {
        if(DEBUG) printf("Thread 2: Waiting for sem1\n");
        sem_wait(&sem1);
    }
    if(DEBUG) printf("Thread 2: Posted to sem2\n");
    sem_post(&sem2);
    return NULL;
}

int main(int argc, char* argv[]) {
    if(argc != 3){
        printf("Usage: %s START_NUM DEBUG_FLAG\n", argv[0]);
        return 1;
    }

    START_NUM = atoi(argv[1]);
    DEBUG = atoi(argv[2]);

    sem_init(&sem1, 0, START_NUM);
    sem_init(&sem2, 0, 1);

    pthread_create(&thread1, NULL, &thread1_doit, NULL);
    pthread_create(&thread2, NULL, &thread2_doit, NULL);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    return 0;
}
