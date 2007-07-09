/* $Id$ */

/* run_context -- stuff in which TESH runs a command                        */

/* Copyright (c) 2007 Martin Quinson.                                       */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "tesh.h"

#include <sys/types.h>
#include <sys/wait.h>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);

xbt_dynar_t bg_jobs = NULL;
rctx_t armageddon_initiator = NULL;
xbt_os_mutex_t armageddon_mutex = NULL;

/* 
 * Module management
 */

static void kill_it(void*r) {  
  rctx_t rctx = *(rctx_t*)r;

  VERB1("Join thread %p which were running a background cmd",rctx->runner);
  xbt_os_thread_join(rctx->runner,NULL);
  rctx_free(rctx);
}

void rctx_init(void) {
  bg_jobs = xbt_dynar_new(sizeof(rctx_t),kill_it);
  armageddon_mutex = xbt_os_mutex_init();
  armageddon_initiator = NULL;
}

void rctx_exit(void) {
  if (bg_jobs)
    xbt_dynar_free(&bg_jobs);
  xbt_os_mutex_destroy(armageddon_mutex);
}

void rctx_wait_bg(void) {
  xbt_dynar_free(&bg_jobs);
  bg_jobs = xbt_dynar_new(sizeof(rctx_t),kill_it);
}

void rctx_armageddon(rctx_t initiator, int exitcode) {
  rctx_t rctx;
  int cpt;

  xbt_os_mutex_lock(armageddon_mutex);
  if (armageddon_initiator != NULL) {
    VERB0("Armageddon already started. Let it go");
    return;
  }
  armageddon_initiator = initiator;
  xbt_os_mutex_unlock(armageddon_mutex);


  /* Kill any background commands */
  xbt_dynar_foreach(bg_jobs,cpt,rctx) {
    if (rctx != initiator) {
      xbt_os_mutex_lock(rctx->interruption);
      rctx->interrupted = 1;
      INFO2("Kill <%s> because <%s> failed",rctx->filepos,initiator->filepos);
      if (!rctx->reader_done) {
	kill(rctx->pid,SIGTERM);
	usleep(100);
	kill(rctx->pid,SIGKILL);          
      }
      xbt_os_mutex_unlock(rctx->interruption);
    }
  }

  /* Remove myself from the tasks */
  if (xbt_dynar_member(bg_jobs, &initiator)) {
    int mypos = xbt_dynar_search(bg_jobs, &initiator);
    rctx_t myself;
    xbt_dynar_remove_at(bg_jobs,mypos,&myself);
    //    rctx_free(myself);
  } 

  /* Cleanup the place */
  //  xbt_dynar_free(&bg_jobs);

  exit(exitcode);
}

/*
 * Memory management
 */

void rctx_empty(rctx_t rc) {
  if (rc->cmd)
    free(rc->cmd);
  rc->cmd = NULL;
  if (rc->filepos)
    free(rc->filepos);
  rc->filepos = NULL;
  rc->is_empty = 1;
  rc->is_background = 0;
  rc->is_stoppable = 0;
  rc->output = e_output_check;
  rc->brokenpipe = 0;
  rc->timeout = 0;
  rc->interrupted = 0;
  buff_empty(rc->input);
  buff_empty(rc->output_wanted);
  buff_empty(rc->output_got);
}

rctx_t rctx_new() {
  rctx_t res = xbt_new0(s_rctx_t,1);

  res->input=buff_new();
  res->output_wanted=buff_new();
  res->output_got=buff_new();
  res->interruption = xbt_os_mutex_init();
  rctx_empty(res);
  return res;
}

void rctx_free(rctx_t rctx) {
  DEBUG1("RCTX: Free %p", rctx);
  rctx_dump(rctx,"free");
  if (!rctx)
    return;

  if (rctx->cmd)
    free(rctx->cmd);
  if (rctx->filepos)
    free(rctx->filepos);
  xbt_os_mutex_destroy(rctx->interruption);
  buff_free(rctx->input);
  buff_free(rctx->output_got);
  buff_free(rctx->output_wanted);
  free(rctx);
}

void rctx_dump(rctx_t rctx, const char *str) {
  DEBUG9("%s RCTX %p={in%p={%d,%10s}, want={%d,%10s}, out={%d,%10s}}",
	 str, rctx,
 	 rctx->input,              rctx->input->used,        rctx->input->data,
	 rctx->output_wanted->used,rctx->output_wanted->data,
	 rctx->output_got->used,   rctx->output_got->data);
  DEBUG5("%s RCTX %p=[cmd%p=%10s, pid=%d]",
	 str,rctx,rctx->cmd,rctx->cmd,rctx->pid);

}

/*
 * Getting instructions from the file
 */

void rctx_pushline(const char* filepos, char kind, char *line) {
  
  switch (kind) {
  case '$':
  case '&':
    if (rctx->cmd) {
      if (!rctx->is_empty) {
	ERROR2("[%s] More than one command in this chunk of lines (previous: %s).\n"
	       " Dunno which input/output belongs to which command.",
	       filepos,rctx->cmd);
	ERROR1("Test suite `%s': NOK (syntax error)",testsuite_name);
	rctx_armageddon(rctx,1);
      }
      rctx_start();
      VERB1("[%s] More than one command in this chunk of lines",filepos);
    }
    if (kind == '&')
      rctx->is_background = 1;
    else
      rctx->is_background = 0;
      
    rctx->cmd = xbt_strdup(line);
    rctx->filepos = xbt_strdup(filepos);
    INFO3("[%s] %s%s",filepos,rctx->cmd,
	  ((rctx->is_background)?" (background command)":""));

    break;
    
  case '<':
    rctx->is_empty = 0;
    buff_append(rctx->input,line);
    buff_append(rctx->input,"\n");
    break;

  case '>':
    rctx->is_empty = 0;
    buff_append(rctx->output_wanted,line);
    buff_append(rctx->output_wanted,"\n");
    break;

  case '!':
    if (rctx->cmd)
      rctx_start();

    if (!strncmp(line,"timeout no",strlen("timeout no"))) {
      VERB1("[%s] (disable timeout)", filepos);
      timeout_value = -1;
    } else if (!strncmp(line,"timeout ",strlen("timeout "))) {
      timeout_value=atoi(line+strlen("timeout"));
      VERB2("[%s] (new timeout value: %d)",
	     filepos,timeout_value);

    } else if (!strncmp(line,"expect signal ",strlen("expect signal "))) {
      rctx->expected_signal = strdup(line + strlen("expect signal "));
      xbt_str_trim(rctx->expected_signal," \n");
	   VERB2("[%s] (next command must raise signal %s)", 
		 filepos, rctx->expected_signal);

    } else if (!strncmp(line,"expect return ",strlen("expect return "))) {
      rctx->expected_return = atoi(line+strlen("expect return "));
      VERB2("[%s] (next command must return code %d)",
	    filepos, rctx->expected_return);

    } else if (!strncmp(line,"output ignore",strlen("output ignore"))) {
      rctx->output = e_output_ignore;
      VERB1("[%s] (ignore output of next command)", filepos);
       
    } else if (!strncmp(line,"output display",strlen("output display"))) {
      rctx->output = e_output_display;
      VERB1("[%s] (ignore output of next command)", filepos);
       
    } else {
      ERROR2("%s: Malformed metacommand: %s",filepos,line);
      ERROR1("Test suite `%s': NOK (syntax error)",testsuite_name);
      rctx_armageddon(rctx,1);
    }
    break;
  }
}

/* 
 * Actually doing the job
 */

/* The IO of the childs are handled by the two following threads
   (one pair per child) */

static void* thread_writer(void *r) {
  int posw;
  rctx_t rctx = (rctx_t)r;
  for (posw=0; posw<rctx->input->used && !rctx->brokenpipe; ) {
    int got;
    DEBUG1("Still %d chars to write",rctx->input->used-posw);
    got=write(rctx->child_to,rctx->input->data+posw,rctx->input->used-posw);
    if (got>0)
      posw+=got;
    if (got<0) {
      if (errno == EPIPE) {
	rctx->brokenpipe = 1;
      } else if (errno!=EINTR && errno!=EAGAIN && errno!=EPIPE) {
	perror("Error while writing input to child");
	ERROR1("Test suite `%s': NOK (system error)",testsuite_name);
	rctx_armageddon(rctx,4);
      }
    }
    DEBUG1("written %d chars so far",posw);

    if (got <= 0)
      usleep(100);
  }
  rctx->input->data[0]='\0';
  rctx->input->used=0;
  close(rctx->child_to);

  return NULL;
}
static void *thread_reader(void *r) {
  rctx_t rctx = (rctx_t)r;
  char *buffout=malloc(4096);
  int posr, got_pid;

  do {
    posr=read(rctx->child_from,buffout,4095);
    if (posr<0 && errno!=EINTR && errno!=EAGAIN) {
      perror("Error while reading output of child");
      ERROR1("Test suite `%s': NOK (system error)", testsuite_name);
      rctx_armageddon(rctx,4);
    }
    if (posr>0) {
      buffout[posr]='\0';
      buff_append(rctx->output_got,buffout);
    } else {
      usleep(100);
    }
  } while (!rctx->timeout && posr!=0);
  free(buffout);

  /* let this thread wait for the child so that the main thread can detect the timeout without blocking on the wait */
  got_pid = waitpid(rctx->pid,&rctx->status,0);
  if (got_pid != rctx->pid) {
    perror(bprintf("Cannot wait for the child %s",rctx->cmd));
    ERROR1("Test suite `%s': NOK (system error)", testsuite_name);
    rctx_armageddon(rctx,4);
  }
   
  rctx->reader_done = 1;
  return NULL;
} 

/* Start a new child, plug the pipes as expected and fire up the 
   helping threads. Is also waits for the child to end if this is a 
   foreground job, or fire up a thread to wait otherwise. */

void rctx_start(void) {
  int child_in[2];
  int child_out[2];

  VERB2("Start %s %s",rctx->cmd,(rctx->is_background?"(background job)":""));
  if (pipe(child_in) || pipe(child_out)) {
    perror("Cannot open the pipes");
    ERROR1("Test suite `%s': NOK (system error)", testsuite_name);
    rctx_armageddon(rctx,4);
  }

  rctx->pid=fork();
  if (rctx->pid<0) {
    perror("Cannot fork the command");
    ERROR1("Test suite `%s': NOK (system error)", testsuite_name);
    rctx_armageddon(rctx,4);
  }

  if (rctx->pid) { /* father */
    close(child_in[0]);
    rctx->child_to = child_in[1];

    close(child_out[1]);
    rctx->child_from = child_out[0];

    if (timeout_value > 0)
       rctx->end_time = time(NULL) + timeout_value;
    else 
       rctx->end_time = -1;

    rctx->reader_done = 0;
    rctx->reader = xbt_os_thread_create(thread_reader,(void*)rctx);
    rctx->writer = xbt_os_thread_create(thread_writer,(void*)rctx);

  } else { /* child */

    close(child_in[1]);
    dup2(child_in[0],0);
    close(child_in[0]);

    close(child_out[0]);
    dup2(child_out[1],1);
    dup2(child_out[1],2);
    close(child_out[1]);

    execlp ("/bin/sh", "sh", "-c", rctx->cmd, NULL);
  }

  rctx->is_stoppable = 1;

  if (!rctx->is_background) {
    rctx_wait(rctx);
  } else {
    /* Damn. Copy the rctx and launch a thread to handle it */
    rctx_t old = rctx;
    xbt_os_thread_t runner;

    rctx = rctx_new();
    DEBUG2("RCTX: new bg=%p, new fg=%p",old,rctx);

    DEBUG2("Launch a thread to wait for %s %d",old->cmd,old->pid);
    runner = xbt_os_thread_create(rctx_wait,(void*)old);
    old->runner = runner;
    VERB3("Launched thread %p to wait for %s %d",
	  runner,old->cmd, old->pid);
    xbt_dynar_push(bg_jobs,&old);
  }
}

/* Waits for the child to end (or to timeout), and check its 
   ending conditions. This is launched from rctx_start but either in main
   thread (for foreground jobs) or in a separate one for background jobs. 
   That explains the prototype, forced by xbt_os_thread_create. */

void *rctx_wait(void* r) {
  rctx_t rctx = (rctx_t)r;
  int errcode = 0;
  int now = time(NULL);
    
  rctx_dump(rctx,"wait");

  if (!rctx->is_stoppable) 
    THROW1(unknown_error,0,"Cmd '%s' not started yet. Cannot wait it",
	   rctx->cmd);

  /* Wait for the child to die or the timeout to happen (or an armageddon to happen) */
  while (!rctx->interrupted && !rctx->reader_done && (rctx->end_time <0 ||rctx->end_time >= now)) {
    usleep(100);
    now = time(NULL);
  }
   
  xbt_os_mutex_lock(rctx->interruption);

  if (!rctx->interrupted && rctx->end_time > 0 && rctx->end_time < now) {    
    INFO1("<%s> timeouted. Kill the process.",rctx->filepos);
    rctx->timeout = 1;
    kill(rctx->pid,SIGTERM);
    usleep(100);
    kill(rctx->pid,SIGKILL);    
    rctx->reader_done = 1;
  }
   
  /* Make sure helper threads die.
     Cannot block since they wait for the child we just killed
     if not already dead. */
  xbt_os_thread_join(rctx->writer,NULL);
  xbt_os_thread_join(rctx->reader,NULL);

  /*  xbt_os_mutex_unlock(rctx->interruption);
  if (rctx->interrupted)
    return NULL;
    xbt_os_mutex_lock(rctx->interruption);*/
 
  buff_chomp(rctx->output_got);
  buff_chomp(rctx->output_wanted);
  buff_trim(rctx->output_got);
  buff_trim(rctx->output_wanted);

  /* Check for broken pipe */
  if (rctx->brokenpipe)
    VERB0("Warning: Child did not consume all its input (I got broken pipe)");

  /* Check for timeouts */
  if (rctx->timeout) {
    if (rctx->output_got->data[0])
      INFO2("<%s> Output on timeout:\n%s",
	    rctx->filepos,rctx->output_got->data);
    else
      INFO1("<%s> No output before timeout",
	    rctx->filepos);
    ERROR3("Test suite `%s': NOK (<%s> timeout after %d sec)", 
	   testsuite_name,rctx->filepos,timeout_value);
    if (!rctx->interrupted)
      rctx_armageddon(rctx, 3);
  }
      
  DEBUG2("RCTX=%p (pid=%d)",rctx,rctx->pid);
  DEBUG3("Status(%s|%d)=%d",rctx->cmd,rctx->pid,rctx->status);

  if (!rctx->interrupted) {
    if (WIFSIGNALED(rctx->status) && !rctx->expected_signal) {
      ERROR3("Test suite `%s': NOK (<%s> got signal %s)", 
	     testsuite_name, rctx->filepos,
	     signal_name(WTERMSIG(rctx->status),NULL));
      errcode = WTERMSIG(rctx->status)+4;	
    }
    
    if (WIFSIGNALED(rctx->status) && rctx->expected_signal &&
	strcmp(signal_name(WTERMSIG(rctx->status),rctx->expected_signal),
	       rctx->expected_signal)) {
      ERROR4("Test suite `%s': NOK (%s got signal %s instead of %s)", 
	     testsuite_name, rctx->filepos,
	     signal_name(WTERMSIG(rctx->status),rctx->expected_signal),
	     rctx->expected_signal);
      errcode = WTERMSIG(rctx->status)+4;	
    }
    
    if (!WIFSIGNALED(rctx->status) && rctx->expected_signal) {
      ERROR3("Test suite `%s': NOK (child %s expected signal %s)", 
	     testsuite_name, rctx->filepos,
	     rctx->expected_signal);
      errcode = 5;
    }
    
    if (WIFEXITED(rctx->status) && WEXITSTATUS(rctx->status) != rctx->expected_return ) {
      if (rctx->expected_return) 
	ERROR4("Test suite `%s': NOK (<%s> returned code %d instead of %d)",
	       testsuite_name, rctx->filepos,
	       WEXITSTATUS(rctx->status), rctx->expected_return);
      else
	ERROR3("Test suite `%s': NOK (<%s> returned code %d)",
	       testsuite_name, rctx->filepos, WEXITSTATUS(rctx->status));
      errcode = 40+WEXITSTATUS(rctx->status);
      
    }
    rctx->expected_return = 0;
  
    if(rctx->expected_signal){
      free(rctx->expected_signal);
      rctx->expected_signal = NULL;
    }
  }

  if (   rctx->output == e_output_check
      && (    rctx->output_got->used != rctx->output_wanted->used
	   || strcmp(rctx->output_got->data, rctx->output_wanted->data))) {
    if (XBT_LOG_ISENABLED(tesh,xbt_log_priority_info)) {
       char *diff= xbt_str_diff(rctx->output_wanted->data,rctx->output_got->data);	  
       ERROR2("Output of <%s> mismatch:\n%s",rctx->filepos,diff);
       free(diff);
    }     
    ERROR2("Test suite `%s': NOK (<%s> output mismatch)", 
	   testsuite_name,rctx->filepos);
     
    errcode=2;
  } else if (rctx->output == e_output_ignore) {
    INFO1("(ignoring the output of <%s> as requested)",rctx->filepos);
  } else if (rctx->output == e_output_display) {
    xbt_dynar_t a = xbt_str_split(rctx->output_got->data, "\n");
    char *out = xbt_str_join(a,"\n||");
    xbt_dynar_free(&a);
    INFO1("Here is the (ignored) command output: \n||%s",out);
    free(out);
  } else if (errcode || rctx->interrupted) {
    /* checking output, and matching */
    xbt_dynar_t a = xbt_str_split(rctx->output_got->data, "\n");
    char *out = xbt_str_join(a,"\n||");
    xbt_dynar_free(&a);
    INFO2("Output of <%s> so far: \n||%s",rctx->filepos,out);
    free(out);    
  }

  if (!rctx->is_background) {
    rctx_empty(rctx);
  }
  if (errcode) {
    if (!rctx->interrupted)
      rctx_armageddon(rctx, errcode);
  }
  xbt_os_mutex_unlock(rctx->interruption);

  return NULL;
}

