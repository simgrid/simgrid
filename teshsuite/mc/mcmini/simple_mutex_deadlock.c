#include <pthread.h>

pthread_mutex_t mutex1;
pthread_mutex_t mutex2;

int main(int argc, char* argv[]) {
    pthread_mutex_init(&mutex1, NULL);
    pthread_mutex_init(&mutex2, NULL);

    pthread_mutex_lock(&mutex1);
    pthread_mutex_lock(&mutex2);
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_lock(&mutex1);
    pthread_mutex_unlock(&mutex1);

    return 0;
}

