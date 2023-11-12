#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>

sem_t sem;

int main(int argc, char* argv[]) {
    if(argc < 2) {
        printf("Expected usage: %s START_NUM\n", argv[0]);
        return -1;
    }

    int start_num = atoi(argv[1]);

    sem_init(&sem, 0, start_num);

    for(int i = 0; i < start_num; i++) {
        sem_wait(&sem);
    }

    sem_post(&sem);
    sem_post(&sem);
    sem_wait(&sem);
    sem_wait(&sem);

    sem_destroy(&sem);

    return 0;
}
