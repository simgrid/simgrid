/* Copyright (c) 2023-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <pthread.h>

pthread_barrier_t barrier;

int main(int argc, char* argv[]) {
    pthread_barrier_init(&barrier, NULL, 2);
    pthread_barrier_wait(&barrier);
    return 0;
}

