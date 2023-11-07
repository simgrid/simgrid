#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#define MaxBufferSize 5

sem_t empty;
sem_t full;
int in = 0;
int out = 0;
int buffer[MaxBufferSize];
pthread_mutex_t mutex;
int BufferSize;
int ItemCount;
int DEBUG;

static void *producer(void *pno)
{
    int item;
    for(int i = 0; i < ItemCount; i++) {
        item = rand(); // Produce an random item
        sem_wait(&empty);
        pthread_mutex_lock(&mutex);
        buffer[in] = item;
        if (DEBUG) {
          printf("Producer %d: Insert Item %d at %d\n",
                 *((int *)pno),buffer[in],in);
        }
        in = (in+1)%BufferSize;
        pthread_mutex_unlock(&mutex);
        sem_post(&full);
    }
    return NULL;
}

static void *consumer(void *cno)
{
    for(int i = 0; i < ItemCount; i++) {
        sem_wait(&full);
        pthread_mutex_lock(&mutex);
        int item = buffer[out];
        if (DEBUG) {
          printf("Consumer %d: Remove Item %d from %d\n",
                 *((int *)cno),item, out);
        }
        out = (out+1)%BufferSize;
        pthread_mutex_unlock(&mutex);
        sem_post(&empty);
    }
    return NULL;
}

int main(int argc, char* argv[]) 
{
    if (argc < 6) {
      printf("Usage: %s <NUM_PRODUCERS> <NUM_CONSUMERS> <ITEM_COUNT> <BUFFER_SIZE> <DEBUG>\n", argv[0]);
      return 1;
    }
  
    int NUM_PRODUCERS = atoi(argv[1]);
    int NUM_CONSUMERS = atoi(argv[2]);
    ItemCount = atoi(argv[3]);
    BufferSize = atoi(argv[4]);
    DEBUG = atoi(argv[5]);

    pthread_t pro[NUM_PRODUCERS],con[NUM_CONSUMERS];

    pthread_mutex_init(&mutex, NULL);
    sem_init(&empty,0,BufferSize);
    sem_init(&full,0,0);

    int a[NUM_PRODUCERS > NUM_CONSUMERS ? NUM_PRODUCERS : NUM_CONSUMERS];

    for(int i = 0; i < NUM_PRODUCERS; i++) {
        a[i] = i+1;
        pthread_create(&pro[i], NULL, producer, (void *)&a[i]);
    }
    for(int i = 0; i < NUM_CONSUMERS; i++) {
        a[i] = i+1;
        pthread_create(&con[i], NULL, consumer, (void *)&a[i]);
    }

    for(int i = 0; i < NUM_PRODUCERS; i++) {
        pthread_join(pro[i], NULL);
    }
    for(int i = 0; i < NUM_CONSUMERS; i++) {
        pthread_join(con[i], NULL);
    }

    return 0;
}
