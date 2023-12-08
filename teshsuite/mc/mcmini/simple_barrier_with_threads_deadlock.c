#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

int THREAD_NUM; 
int DEBUG = 0;

pthread_barrier_t barrier;
pthread_t *thread;

static void * thread_doit(void *unused)
{
    if(DEBUG) printf("Thread %lu: Waiting at barrier\n", (unsigned long)pthread_self());
    pthread_barrier_wait(&barrier);
    if(DEBUG) printf("Thread %lu: Passed the barrier\n", (unsigned long)pthread_self());
    return NULL;
}

int main(int argc, char* argv[]) {
    if(argc != 3){
        printf("Usage: %s THREAD_NUM DEBUG_FLAG\n", argv[0]);
        return 1;
    }

    THREAD_NUM = atoi(argv[1]);
    DEBUG = atoi(argv[2]);

    thread = (pthread_t*) malloc(THREAD_NUM * sizeof(pthread_t));

    pthread_barrier_init(&barrier, NULL, THREAD_NUM);
    for(int i = 0; i < THREAD_NUM; i++) {
        pthread_create(&thread[i], NULL, &thread_doit, NULL);
    }

    pthread_barrier_wait(&barrier);

    for(int i = 0; i < THREAD_NUM; i++) {
        pthread_join(thread[i], NULL);
    }

    free(thread);
    return 0;
}
