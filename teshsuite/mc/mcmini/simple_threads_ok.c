#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

static void * thread_doit(void *unused) {
    int len = (int) ((drand48() * 5) + 1);
    sleep(len);
    return NULL;
}

int main(int argc, char* argv[]) {
    if(argc < 2) {
        printf("Expected usage: %s THREAD_NUM\n", argv[0]);
        return -1;
    }

    int thread_num = atoi(argv[1]);

    pthread_t *threads = malloc(sizeof(pthread_t) * thread_num);

    for(int i = 0; i < thread_num; i++) {
        pthread_create(&threads[i], NULL, &thread_doit, NULL);
    }

    for(int i = 0; i < thread_num; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);

    return 0;
}
