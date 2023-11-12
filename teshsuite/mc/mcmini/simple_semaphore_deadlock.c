#include <pthread.h>
#include <semaphore.h>

sem_t sem1;
sem_t sem2;
pthread_t thread1, thread2;

static void * thread_doit1(void *forks_arg) {
    sem_wait(&sem2);
    sem_wait(&sem1);
    sem_post(&sem1);
    sem_post(&sem2);
    return NULL;
}

static void * thread_doit2(void *forks_arg) {
    sem_wait(&sem1);
    sem_wait(&sem2);
    sem_post(&sem2);
    sem_post(&sem1);
    return NULL;
}

int main(int argc, char* argv[]) {
    sem_init(&sem1, 0, 1);
    sem_init(&sem2, 0, 1);

    pthread_create(&thread1, NULL, &thread_doit1, NULL);
    pthread_create(&thread2, NULL, &thread_doit2, NULL);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    return 0;
}

