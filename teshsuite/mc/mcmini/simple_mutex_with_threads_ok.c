#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

pthread_mutex_t mutex1;
pthread_mutex_t mutex2;

static void * thread_doit(void *unused) {
    pthread_mutex_lock(&mutex1);
    pthread_mutex_lock(&mutex2);
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    return NULL;
}

int main(int argc, char* argv[]) {

    if(argc < 2) {
        printf("Expected usage: %s THREAD_NUM\n", argv[0]);
        return -1;
    }

    int THREAD_NUM = atoi(argv[1]);

    if(THREAD_NUM < 2) {
        printf("At least 2 threads are required\n");
        return -1;
    }

    pthread_t *threads = malloc(sizeof(pthread_t) * THREAD_NUM);

    pthread_mutex_init(&mutex1, NULL);
    pthread_mutex_init(&mutex2, NULL);

    for(int i = 0; i < THREAD_NUM; i++) {
        pthread_create(&threads[i], NULL, &thread_doit, NULL);
    }

    for(int i = 0; i < THREAD_NUM; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    pthread_mutex_destroy(&mutex1);
    pthread_mutex_destroy(&mutex2);

    return 0;
}
