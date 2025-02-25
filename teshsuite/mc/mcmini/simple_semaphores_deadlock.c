/* Copyright (c) 2023-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

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

