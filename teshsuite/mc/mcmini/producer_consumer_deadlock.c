#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

int MaxItems; // Maximum items a producer can produce or a consumer can consume
int BufferSize; // Size of the buffer

sem_t empty;
sem_t full;
int in = 0;
int out = 0;
int *buffer;
pthread_mutex_t mutex;

int DEBUG = 0; // Debug flag

static void *producer(void *pno)
{
    int item;
    for(int i = 0; i < MaxItems; i++) {
        item = rand(); // Produce a random item
        pthread_mutex_lock(&mutex);
        sem_wait(&empty);
        buffer[in] = item;
        if(DEBUG) printf("Producer %d: Insert Item %d at %d\n", *((int *)pno),buffer[in],in);
        in = (in+1)%BufferSize;
        pthread_mutex_unlock(&mutex);
        sem_post(&full);
    }
    return NULL;
}

static void *consumer(void *cno)
{
    for(int i = 0; i < MaxItems; i++) {
        pthread_mutex_lock(&mutex);
        sem_wait(&full);
        int item = buffer[out];
        if(DEBUG) printf("Consumer %d: Remove Item %d from %d\n",*((int *)cno),item, out);
        out = (out+1)%BufferSize;
        pthread_mutex_unlock(&mutex);
        sem_post(&empty);
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    if(argc != 4){
        printf("Usage: %s MAX_ITEMS BUFFER_SIZE DEBUG\n", argv[0]);
        return 1;
    }

    MaxItems = atoi(argv[1]);
    BufferSize = atoi(argv[2]);
    DEBUG = atoi(argv[3]);

    buffer = (int*) malloc(BufferSize * sizeof(int));

    pthread_t pro[5],con[5];
    pthread_mutex_init(&mutex, NULL);
    sem_init(&empty,0,BufferSize);
    sem_init(&full,0,0);

    int a[5] = {1,2,3,4,5}; //Just used for numbering the producer and consumer

    for(int i = 0; i < 5; i++) {
        pthread_create(&pro[i], NULL, producer, (void *)&a[i]);
    }
    for(int i = 0; i < 5; i++) {
        pthread_create(&con[i], NULL, consumer, (void *)&a[i]);
    }

    for(int i = 0; i < 5; i++) {
        pthread_join(pro[i], NULL);
    }
    for(int i = 0; i < 5; i++) {
        pthread_join(con[i], NULL);
    }

    free(buffer);
    return 0;
}
