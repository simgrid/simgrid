/* spawn - demo of the gras_agent_spawn function                            */

/* The server process wants to compute all prime numbers between 0 and maxint.
   For that, it spawns amount of child processes and communicate with them
    through 2 queues (one for the things to do, and another for the things done).
   Beware, using sockets to speak between main thread and spawned agent
     is unreliable because they share the same incoming listener thread. */

/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"
#include "xbt/strbuff.h"
#include "gras.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(Spawn, "Messages specific to this example");

/* defines an interval to be searched */
typedef struct {
  int min, max;
  xbt_dynar_t primes;
} s_work_chunk_t,*work_chunk_t;
xbt_queue_t todo; /* The queue in which the server puts the work to do */
xbt_queue_t done; /* the queue in which the workers puts the result of their work; */

int worker(int argc, char *argv[]);
int worker(int argc, char *argv[]) {
  work_chunk_t chunk;
  int moretodo = 1;
  while (moretodo) {
    xbt_ex_t e;
    TRY {
      xbt_queue_shift_timed(todo,&chunk,0);
    } CATCH(e) {
      if (e.category != timeout_error) {
        RETHROW;
      }
      moretodo = 0;
    }
    if (!moretodo)
      break; // Do not break from within the CATCH, exceptions don't like it.

    INFO2("Got [%d;%d] to do",chunk->min,chunk->max);
    GRAS_BENCH_ALWAYS_BEGIN();
    int i;
    for (i=chunk->min;i<chunk->max;i++) {
      int j;
      for (j=2;j<i;j++) {
        if (i%j == 0) // not prime: j divides i perfectly
          break;
      }
      if (j==i) // no divisor found: that's prime
        xbt_dynar_push(chunk->primes,&i);
    }
    GRAS_BENCH_ALWAYS_END();
    xbt_queue_push(done,&chunk);
  }
  INFO0("No more work for me; bailing out");

  return 0;
}

/* ********************************************************************** */
/*  The server */
/* ********************************************************************** */
int server(int argc, char *argv[]);
int server(int argc, char *argv[])
{
  int maxint = 1000;
  int perchunk = 50;
  int child_amount = 5;
  char **worker_args;
  int i;
  work_chunk_t chunk;

  gras_init(&argc, argv);

  todo = xbt_queue_new(-1,sizeof(work_chunk_t));
  done = xbt_queue_new(-1,sizeof(work_chunk_t));


  INFO0("Prepare some work");
  for (i=0;i<maxint/perchunk;i++) {
    chunk = xbt_new0(s_work_chunk_t,1);
    chunk->min = i*perchunk;
    chunk->max = (i+1)*perchunk;
    chunk->primes = xbt_dynar_new(sizeof(int),NULL);
    xbt_queue_push(todo,&chunk);
  }

  INFO0("Spawn the kids");
  for (i = 0; i < child_amount; i++) {
    char *name = bprintf("child%d",i);
    worker_args = xbt_new0(char *, 2);
    worker_args[0] = xbt_strdup("child");
    worker_args[1] = NULL;
    gras_agent_spawn(name, worker, 1, worker_args, NULL);
    free(name);
  }

  INFO0("Fetch their answers");
  for (i=0;i<maxint/perchunk;i++) {
    work_chunk_t chunk;
    xbt_strbuff_t buff = xbt_strbuff_new();
    int first=1;
    unsigned int cursor;
    int data;
    xbt_queue_pop(done,&chunk);
    xbt_dynar_foreach(chunk->primes,cursor,data) {
      char number[100];
      sprintf(number,"%d",data);
      if (first)
        first = 0;
      else
        xbt_strbuff_append(buff,",");
      xbt_strbuff_append(buff,number);
    }
    INFO3("Primes in [%d,%d]: %s",chunk->min,chunk->max,buff->data);
    xbt_strbuff_free(buff);
  }
  gras_os_sleep(.1);/* Let the childs detect that there is nothing more to do */
  xbt_queue_free(&todo);
  xbt_queue_free(&done);

  gras_exit();

  return 0;
}
