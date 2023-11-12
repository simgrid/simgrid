#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>

sem_t sem1, sem2;
pthread_t thread1, thread2;

static void * thread1_doit(void *unused) {
    sem_wait(&sem2);
    sem_post(&sem1);
    sem_wait(&sem2);
    return NULL;
}

static void * thread2_doit(void *sem_count) {
    int start_num = *((int*)sem_count);
    for(int i = 0; i < start_num + 1; i++) {
        sem_wait(&sem1);
    }
    sem_post(&sem2);
    return NULL;
}

int main(int argc, char* argv[]) {
    if(argc < 2) {
        printf("Expected usage: %s START_NUM\n", argv[0]);
        return -1;
    }

    int start_num = atoi(argv[1]);

    sem_init(&sem1, 0, start_num);
    sem_init(&sem2, 0, 1);

    pthread_create(&thread1, NULL, &thread1_doit, NULL);
    pthread_create(&thread2, NULL, &thread2_doit, &start_num);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    sem_destroy(&sem1);
    sem_destroy(&sem2);

    return 0;
}
