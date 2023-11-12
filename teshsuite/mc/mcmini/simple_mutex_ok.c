#include <pthread.h>
#include <stdio.h>

pthread_mutex_t mutex1;
pthread_mutex_t mutex2;

int main(int argc, char* argv[]) {

    if(argc != 1) {
        printf("Expected usage: %s \n", argv[0]);
        return -1;
    }

    pthread_mutex_init(&mutex1, NULL);
    pthread_mutex_init(&mutex2, NULL);

    pthread_mutex_lock(&mutex1);
    pthread_mutex_lock(&mutex2);
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);

    pthread_mutex_destroy(&mutex1);
    pthread_mutex_destroy(&mutex2);

    return 0;
}