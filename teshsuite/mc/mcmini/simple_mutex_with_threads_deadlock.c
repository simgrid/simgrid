#include <pthread.h>

pthread_mutex_t mutex1;
pthread_mutex_t mutex2;
pthread_t thread1, thread2;

static void * thread_doit1(void *forks_arg) {
    pthread_mutex_lock(&mutex2);
    pthread_mutex_lock(&mutex1);
    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
    return NULL;
}

static void * thread_doit2(void *forks_arg) {
    pthread_mutex_lock(&mutex1);
    pthread_mutex_lock(&mutex2);
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    return NULL;
}

int main(int argc, char* argv[]) {
    pthread_mutex_init(&mutex1, NULL);
    pthread_mutex_init(&mutex2, NULL);

    pthread_create(&thread1, NULL, &thread_doit1, NULL);
    pthread_create(&thread2, NULL, &thread_doit2, NULL);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    return 0;
}

