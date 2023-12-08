#include <pthread.h>

pthread_barrier_t barrier;

int main(int argc, char* argv[]) {
    pthread_barrier_init(&barrier, NULL, 2);
    pthread_barrier_wait(&barrier);
    return 0;
}

