#include <pthread.h>
#include <semaphore.h>

sem_t sem;

int main(int argc, char* argv[]) {
    sem_init(&sem, 0, 0);

    sem_post(&sem);
    sem_post(&sem);
    sem_wait(&sem);
    sem_wait(&sem);
    sem_wait(&sem);
    return 0;
}

